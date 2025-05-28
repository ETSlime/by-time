#include "Core/TextureMgr.h"
#include "Core/TextureUploadQueue.h"
#include "Core/Async/ThreadPool.h"
#include "Core/Async/DeferredTaskAPI.h"


void TextureMgr::Shutdown(void)
{
    for (auto& srv : m_TextureSRVMap)
    {
        //SAFE_DELETE(srv.value);
    }

    m_TextureSRVMap.clear();
}

void TextureMgr::RequestTextureAsync(const char* filename, ID3D11ShaderResourceView** outSRV, void(*onFinish)(bool success, void* userData), void* userData)
{
    if (!filename || !outSRV) return;

    // キャッシュ済みなら即時完了
    SRV_POOL** existing = m_TextureSRVMap.search(filename);
    if (existing) 
    {
        *outSRV = (*existing)->srv;
        if (onFinish) onFinish(true, userData);
        return;
    }

    // すでに同じファイルパスのタスクが存在するなら重複登録しない
    if (TextureUploadQueue::get_instance().HasTask(filename))
    {
        // すでにロード中、コールバックを即時呼び出し（成功を仮定）
        if (onFinish)
            onFinish(true, userData); // 成功と仮定で呼び出し
        return;
    }


    // GPU Upload タスク生成（メインスレッドで実行）
    TextureUploadTask* task = new TextureUploadTask();
    task->path = const_cast<char*>(filename);
    task->onFinish = onFinish;
    task->outSRV = outSRV;
    task->userData = userData;
    task->finishEvent = ThreadPool::get_instance().AcquireFinishEvent();
    task->fileBuffer = nullptr;
    task->bufferSize = 0;

    // 登録（主スレッド）
    TextureUploadQueue::get_instance().Push(task);

    // Deferred 登録（スレッドプールでファイル読み込み）
    DeferredTaskOptions opt;
    opt.async = true;
    opt.debugName = "TextureFileIO";

    RawDeferredTask::Add(
        task->path,
        [](void* param) 
        {
            char* path = static_cast<char*>(param);
            TextureUploadTask* task = TextureUploadQueue::get_instance().GetTask(path);
            if (!task) return;

            FILE* fp = fopen(task->path, "rb");
            if (!fp) 
            {
                if (task->onFinish) task->onFinish(false, task->userData);
                SetEvent(task->finishEvent);
                return;
            }

            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            BYTE* buffer = new BYTE[size];
            fread(buffer, 1, size, fp);
            fclose(fp);

            // 情報書き込み
            task->fileBuffer = buffer;
            task->bufferSize = static_cast<unsigned int>(size);
            // 完了を通知
            SetEvent(task->finishEvent);
        },
        nullptr, // 条件関数なし
        opt
    );
}

ID3D11ShaderResourceView* TextureMgr::CreateTexture(char* filename)
{
	if (filename == nullptr)
		return nullptr;

	SRV_POOL** ppSrvPool = m_TextureSRVMap.search(filename);
	
	if (ppSrvPool == nullptr)
	{
		SRV_POOL* pSrvPool = new SRV_POOL();
		ppSrvPool = &pSrvPool;
		D3DX11CreateShaderResourceViewFromFile(m_renderer.GetDevice(),
			filename,
			NULL,
			NULL,
			&pSrvPool->srv,
			NULL);
        pSrvPool->count = 1;
		m_TextureSRVMap.insert(filename, *ppSrvPool);
	}

	return (*ppSrvPool)->srv;
}

void TextureMgr::RegisterSRV(const char* path, ID3D11ShaderResourceView* srv)
{
    m_lock.Lock();

    if (m_TextureSRVMap.contains(path)) 
    {
        SafeRelease(&srv);
        return;
    }

    SRV_POOL* newPool = new SRV_POOL();
    newPool->srv = srv;
    newPool->count = 1;

    m_TextureSRVMap.insert(path, newPool);

    m_lock.Unlock();
}


