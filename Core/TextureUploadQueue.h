#pragma once
//=============================================================================
//
//  [TextureUploadQueue.h]
// Author :
// 
//=============================================================================
#include "main.h"
#include "Core/TextureMgr.h"
#include "Core/Graphics/Renderer.h"
#include "Core/Async/ThreadPool.h"
#include "Core/Async/SpinLock.h"
#include "Utility/HashMap.h"
#include "Utility/SingletonBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_TEXTURE_PATH            (256)
#define MAX_TEXTURE_UPLOAD_SIZE     (256)

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct TextureUploadTask 
{
    char* path; // テクスチャパス
    BYTE* fileBuffer; // 読み込まれたファイルバッファ
    unsigned int bufferSize; // バッファサイズ

    ID3D11ShaderResourceView** outSRV; // 出力先ポインタ（モデルが保持）

    void (*onFinish)(bool success, void* userData); // コールバック関数
    void* userData; // コールバックに渡すデータ

    HANDLE finishEvent = nullptr; // 主スレッド通知用（主スレッドからWaitForSingleObject可能）
}; // アップロードタスク構造体


class TextureUploadQueue : public SingletonBase<TextureUploadQueue>
{
public:

    void Init(void)
    {
        m_taskMap = HashMap<const char*, TextureUploadTask*, CharPtrHash, CharPtrEquals>(
            MAX_TEXTURE_UPLOAD_SIZE,
            CharPtrHash(),
            CharPtrEquals());
    }

    bool TextureUploadQueue::HasTask(const char* path) 
    {
        m_lock.Lock();
        bool exists = m_taskMap.search(path) != nullptr;
        m_lock.Unlock();
        return exists;
    }

    void Push(TextureUploadTask* task)
    {
        m_lock.Lock();

        if (!m_taskMap.search(task->path)) 
        {
            m_taskMap.insert(task->path, task);
        }

        m_lock.Unlock();
    }

    TextureUploadTask* GetTask(const char* path) 
    {
        m_lock.Lock();
        TextureUploadTask* result = m_taskMap[path];
        m_lock.Unlock();
        return result;
    }

    void ShutDown(void)
    {
        for (auto& it : m_taskMap)
        {
            TextureUploadTask* task = it.value;
            if (!task) continue;

            // バッファ解放
            SAFE_DELETE_ARRAY(task->fileBuffer);

            // イベント解放
            if (task->finishEvent)
            {
                ThreadPool::get_instance().ReleaseFinishEvent(task->finishEvent);
                task->finishEvent = nullptr;
            }

            // 失敗通知
            if (task->onFinish)
            {
                task->onFinish(false, task->userData);
            }

            SAFE_DELETE(task);
        }

        m_taskMap.clear();
    }

    void Update() 
    {
        m_lock.Lock();

        for (auto& it = m_taskMap.begin(); it != m_taskMap.end();)
        {
            TextureUploadTask* task = (*it).value;

            if (task->finishEvent && WaitForSingleObject(task->finishEvent, 0) != WAIT_OBJECT_0)
            {
                ++it;
                continue; // スキップ（次フレーム再確認）
            }


            // GPU アップロード処理
            ID3D11ShaderResourceView* srv = nullptr;
            HRESULT hr = D3DX11CreateShaderResourceViewFromMemory(
                m_renderer.GetDevice(),
                task->fileBuffer,
                task->bufferSize,
                nullptr,
                nullptr,
                &srv,
                nullptr);

            *(task->outSRV) = srv;
            if (task->onFinish)
                task->onFinish(SUCCEEDED(hr), task->userData);

            if (SUCCEEDED(hr))
                m_textureMgr.RegisterSRV(task->path, srv);

            SAFE_DELETE_ARRAY(task->fileBuffer);

            // イベントを完了状態に設定
            if (task->finishEvent)
                ThreadPool::get_instance().ReleaseFinishEvent(task->finishEvent);

            // タスク削除
            it = m_taskMap.erase(it);
        }

        m_lock.Unlock();
    }

private:
    HashMap<const char*, TextureUploadTask*, CharPtrHash, CharPtrEquals> m_taskMap;
    SpinLock m_lock;
    Renderer& m_renderer = Renderer::get_instance();
    TextureMgr& m_textureMgr = TextureMgr::get_instance();
};