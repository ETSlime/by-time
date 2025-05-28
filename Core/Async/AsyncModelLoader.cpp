//=============================================================================
//
// [AsyncModelLoader.cpp]
// Author : 
//
//
//=============================================================================
#include "Core/Timer.h"
#include "Core/Async/AsyncModelLoader.h"
#include "Core/Async/DeferredTaskSystem.h"
#include "Core/Async/DeferredTaskAPI.h"
#include "Model/FBXLoader.h"
#include "Model/ModelCacheLoader.h"
#include "Model/ObJLoader.h"

AsyncModelLoader::~AsyncModelLoader()
{
    for (auto& tasks : m_taskMap) 
    {
        if (tasks.value)
        {
            SAFE_DELETE(tasks.value->parsedData);
        }

        SAFE_DELETE(tasks.value);
    }
    m_taskMap.clear();
}

void AsyncModelLoader::EnqueueLoad(const char* modelPath)
{
	// すでにロード中またはロード済みなら無視
    if (m_taskMap.count(modelPath)) return;

    ModelLoadTask* task = new ModelLoadTask();
    task->modelPath = const_cast<char*>(modelPath);

	DeferredTaskOptions options{};
	options.async = true; // 非同期実行
	options.debugName = "AsyncModelLoader_LoadModel"; // デバッグ用の名前
	// タスクを DeferredTaskSystem に登録
    DeferredTaskBinder<AsyncModelLoader>::Add(
        this,
        task,
        &AsyncModelLoader::AsyncModelLoader_LoadModel,
        &AsyncModelLoader::AsyncModelLoader_CanStart,
        options
    );

	// タスクをマップに登録
    m_taskMap[modelPath] = task;
}

void AsyncModelLoader::Update()
{
    // m_cachedSortArray を再利用（clear して再利用）
    m_cachedSortArray.clear();

    // 優先度順に DeferredRequest をソートして実行
    if (!m_deferredRequests.empty())
    {
        for (auto& it : m_deferredRequests)
        {
            DeferredEntry entry;
            entry.modelPath = it.key;
            entry.info = it.value;
            m_cachedSortArray.push_back(entry);
        }

        // 原地ソート（バブルソート：優先度の高い順）
        UINT n = m_cachedSortArray.getSize();
        for (UINT i = 0; i < n - 1; ++i)
        {
            for (UINT j = 0; j < n - i - 1; ++j)
            {
                if (m_cachedSortArray[j].info.priority < m_cachedSortArray[j + 1].info.priority)
                {
                    // swap
                    DeferredEntry temp = m_cachedSortArray[j];
                    m_cachedSortArray[j] = m_cachedSortArray[j + 1];
                    m_cachedSortArray[j + 1] = temp;
                }
            }
        }

        // 優先度順に Enqueue
        for (UINT i = 0; i < m_cachedSortArray.getSize(); ++i) 
        {
            EnqueueLoad(m_cachedSortArray[i].modelPath);
        }

        // 保留リストをクリア（次のフレーム用）
        m_deferredRequests.clear();
    }
}

bool AsyncModelLoader::IsLoaded(const char* modelPath) const
{
    if (!m_taskMap.contains(modelPath)) return false;

    ModelLoadTask* task = m_taskMap.at(modelPath);
    return task->isDone && task->parsedData;
}

bool AsyncModelLoader::HasFailed(const char* modelPath) const
{
    if (!m_taskMap.contains(modelPath)) return false;

    return m_taskMap.at(modelPath)->hasFailed;
}

void AsyncModelLoader::RequestLoadIfNotQueued(const char* modelPath, int priority, const char* source)
{
    // すでにロード中またはロード済みなら無視
    if (m_taskMap.count(modelPath)) return;
    if (m_deferredRequests.count(modelPath)) return;

    // 登録する
    DeferredRequestInfo info;
    info.priority = priority;
    info.requestedBy = source ? source : "unknown";
    info.requestTime = Timer::get_instance().GetElapsedTime(); // ← 必要ならゲーム内経過時間を返す関数を使ってね！

    m_deferredRequests[modelPath] = info;
}

Model* AsyncModelLoader::CreateModelIfReady(const char* modelPath)
{
    auto it = m_taskMap.find(modelPath);
    if (it == m_taskMap.end()) return nullptr;

    ModelLoadTask* task = it->value;
    if (!task->isDone || !task->parsedData || !task->texturesReady) return nullptr;

    // Model クラスに所有権を移譲
    Model* model = new Model(modelPath, task->parsedData);
    model->CreateGPUResources(task->parsedData); // 所有権を移動
    task->parsedData = nullptr; // ここで null にする（もう使わない）
	SAFE_DELETE(task);
	m_taskMap.remove(modelPath); // タスクを削除

    return model;
}

void AsyncModelLoader::AsyncModelLoader_LoadModel(void* param)
{
    ModelLoadTask* task = (ModelLoadTask*)param;
    task->parsedData = new StaticModelData();

    // キャッシュ読み込みを試みる
    if (!ModelCacheLoader::LoadFromCache(task->modelPath, task->parsedData))
    {
        bool success = false;

		success = OBJLoader::get_instance().LoadObjModel(task->modelPath, task->parsedData);
		if (success)
		{
			ModelCacheLoader::SaveToCache(task->modelPath, task->parsedData);
		}
		else
		{
			// パース失敗 → 解放して終了
			SAFE_DELETE(task->parsedData);
			task->hasFailed = true;
		}
    }

    // テクスチャ非同期登録（OBJのみ対象）
    task->texturesReady = false;
    task->pendingTextureCount = 0;
    task->completedTextureCount = 0;

    if (task->parsedData && task->parsedData->SubsetArray)
    {
        const unsigned int subsetNum = task->parsedData->SubsetNum;
        for (unsigned int i = 0; i < subsetNum; ++i)
        {
            SUBSET& modelSubset = task->parsedData->SubsetArray[i];
            MODEL_MATERIAL& mat = modelSubset.Material;

            RequestModelTexture(mat.DiffuseTextureName, &modelSubset.diffuseTexture, task);
            RequestModelTexture(mat.NormalTextureName, &modelSubset.normalTexture, task);
            RequestModelTexture(mat.BumpTextureName, &modelSubset.bumpTexture, task);
            RequestModelTexture(mat.OpacityTextureName, &modelSubset.opacityTexture, task);
            RequestModelTexture(mat.ReflectTextureName, &modelSubset.reflectTexture, task);
            RequestModelTexture(mat.TranslucencyTextureName, &modelSubset.translucencyTexture, task);
        }
    }


    task->isDone = true;
}

void AsyncModelLoader::RequestModelTexture(const char* path, ID3D11ShaderResourceView** out, ModelLoadTask* task)
{
    if (!path || path[0] == '\0') return;

    ++task->pendingTextureCount;

    TextureMgr::get_instance().RequestTextureAsync
    (
        path, out,
        [](bool success, void* userData) 
        {
            ModelLoadTask* task = static_cast<ModelLoadTask*>(userData);
            if (success)
            {
                task->completedTextureCount++;
                if (task->completedTextureCount == task->pendingTextureCount)
                    task->texturesReady = true;
            }

        },
        task
    );
}
