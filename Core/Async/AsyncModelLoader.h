#pragma once
//=============================================================================
//
// [AsyncModelLoader.h]
// Author : 
//
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
#define MAX_MODEL_PATH (256) // パスの最大長

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct ModelLoadTask 
{
    char modelPath[MAX_MODEL_PATH]{};         // .fbx / .obj 等
    bool isDone = false;
    bool hasFailed = false;
    StaticModelData* parsedData = nullptr;
};

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
    static void AsyncModelLoader_LoadModel(void* param);
    static bool AsyncModelLoader_CanStart(void* ptr) { return true; }

    SimpleArray<DeferredEntry> m_cachedSortArray;
    HashMap<const char*, ModelLoadTask*, CharPtrHash, CharPtrEquals> m_taskMap =
		HashMap<const char*, ModelLoadTask*, CharPtrHash, CharPtrEquals>(MAX_MODEL_NUM, CharPtrHash(), CharPtrEquals());
    HashMap<const char*, DeferredRequestInfo, CharPtrHash, CharPtrEquals> m_deferredRequests =
        HashMap<const char*, DeferredRequestInfo, CharPtrHash, CharPtrEquals>(MAX_MODEL_NUM, CharPtrHash(), CharPtrEquals());

};