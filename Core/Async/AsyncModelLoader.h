#pragma once
//=============================================================================
//
// モデルちゃんの順番管理もバッチリ～非同期読み込み受付嬢　[AsyncModelLoader.h]
// Author : 
// .fbx や .obj モデルを非同期で読み込み、リクエストを優先度つきで並べて処理してくれる超有能ローダーちゃんっ
// ロード完了チェックや失敗検知、GPUリソースへの反映もすべてお任せ！！モデル界の受付嬢なの～っ！
//
//=============================================================================
#include "main.h"
#include "Utility/SingletonBase.h"
#include "Utility/HashMap.h"
#include "Model/Model.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_MODEL_NUM (512) // 最大モデル数

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct ModelLoadTask 
{
    char* modelPath = nullptr;                      // モデルパス（読み取り専用）
    bool isDone = false;                            // パース完了フラグ
    bool hasFailed = false;                         // 失敗フラグ
    StaticModelData* parsedData = nullptr;          // パース済みデータ（CreateGPUResources に渡す）

    unsigned int pendingTextureCount = 0;       // 要求テクスチャ数
    unsigned int completedTextureCount = 0;     // 完了テクスチャ数
    bool texturesReady = false;                 // テクスチャ読み込み完了

}; // モデル非同期読み込みタスク構造体

struct DeferredRequestInfo
{
    int priority = 0;               // 優先度（数値が大きいほど優先）
    char* requestedBy = nullptr;    // 要求元
    float requestTime = 0.0f;       // 登録された時間（表示やデバッグ用）
};

struct DeferredEntry 
{
    const char* modelPath = nullptr;
    DeferredRequestInfo info;
};

class AsyncModelLoader : public SingletonBase<AsyncModelLoader>
{
public:
    AsyncModelLoader() {};
    ~AsyncModelLoader();

    // リクエストが未登録なら登録
    void RequestLoadIfNotQueued(const char* modelPath, int priority = 0, const char* source = nullptr);

    void EnqueueLoad(const char* modelPath);              // 非同期ロード要求を追加
    void Update(void);                                    // 毎フレーム完了チェック
    bool IsLoaded(const char* modelPath) const;           // ロード完了か判定
    bool HasFailed(const char* modelPath) const;          // ロード失敗か判定
    Model* CreateModelIfReady(const char* modelPath);     // GPUリソース作成（主スレッド）

private:
    void AsyncModelLoader_LoadModel(void* param);
    bool AsyncModelLoader_CanStart(void* ptr) { return true; }
    void RequestModelTexture(const char* path, ID3D11ShaderResourceView** out, ModelLoadTask* task);

    SimpleArray<DeferredEntry> m_cachedSortArray;
    HashMap<const char*, ModelLoadTask*, CharPtrHash, CharPtrEquals> m_taskMap =
		HashMap<const char*, ModelLoadTask*, CharPtrHash, CharPtrEquals>(MAX_MODEL_NUM, CharPtrHash(), CharPtrEquals());
    HashMap<const char*, DeferredRequestInfo, CharPtrHash, CharPtrEquals> m_deferredRequests =
        HashMap<const char*, DeferredRequestInfo, CharPtrHash, CharPtrEquals>(MAX_MODEL_NUM, CharPtrHash(), CharPtrEquals());

};