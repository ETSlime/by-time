//=============================================================================
//
// GameObject処理 [GameObject.cpp]
// Author : 
//
//=============================================================================
#include "Scene/GameObject.h"

template <>
GameObject<SkinnedMeshModelInstance>::~GameObject()
{
	if (instance.pModel)
	{
		instance.load = false;
		SkinnedMeshModelPool* modelPool = SkinnedMeshModel::GetModel(instance.modelFullPath);
		if (modelPool)
		{
			modelPool->count--;
			if (modelPool->count == 0)
			{
				delete modelPool->pModel;
				Model::RemoveModel(instance.modelFullPath);
			}
		}
	}

	Scene::get_instance().UnregisterGameObject(this);
}

template <>
void GameObject<SkinnedMeshModelInstance>::Instantiate(char* modelPath, char* modelName, 
	SkinnedModelType modelType, AnimClipName clipName)
{
	char* modelFullPath = new char[MODEL_NAME_LENGTH] {};

	strcat(modelFullPath, modelPath);

	size_t modelPathLength = strlen(modelPath);
	if (modelPathLength > 0 && modelPath[modelPathLength - 1] != '/' && modelPath[modelPathLength - 1] != '\\')
	{
		strcat(modelFullPath, "/");
	}

	strcat(modelFullPath, modelName);

	instance.modelFullPath = modelFullPath;
	instance.modelPath = modelPath;
	instance.modelName = modelName;
	instance.pModel = SkinnedMeshModel::StoreModel(modelPath, modelName, modelFullPath, modelType, clipName);
	if (instance.pModel)
	{
		instance.load = true;
		instance.use = true;
	}
}

template<>
void GameObject<SkinnedMeshModelInstance>::Update()
{
	// ワールド行列の構築
	XMMATRIX mtxWorld = XMMatrixIdentity();
	mtxWorld = XMMatrixMultiply(mtxWorld, XMMatrixScaling(instance.transform.scl.x, instance.transform.scl.y, instance.transform.scl.z)); // スケーリング
	mtxWorld = XMMatrixMultiply(mtxWorld, XMMatrixRotationRollPitchYaw(instance.transform.rot.x, instance.transform.rot.y + XM_PI, instance.transform.rot.z)); // 回転
	mtxWorld = XMMatrixMultiply(mtxWorld, XMMatrixTranslation(instance.transform.pos.x, instance.transform.pos.y, instance.transform.pos.z)); // 平行移動
	instance.transform.mtxWorld = mtxWorld;

	// ボーン変形行列（スキニングの最終変換）
	XMMATRIX boneTransform = XMMatrixIdentity(); // ※実装に応じてルートボーンまたは全体ボーン補正

	// ローカル空間AABBの取得
	XMFLOAT3 localMin = instance.pModel->GetBoundingBox().minPoint;
	XMFLOAT3 localMax = instance.pModel->GetBoundingBox().maxPoint;

	// 8つのコーナー頂点を作成（ローカル空間）
	XMVECTOR localCorners[8] = {
		XMVectorSet(localMin.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMax.z, 1.0f),
	};

	// ワールド空間AABBの初期化
	XMVECTOR worldMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
	XMVECTOR worldMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);

	// 全ての角をスキン変換→ワールド変換→AABB更新
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR skinnedPt = XMVector3Transform(localCorners[i], boneTransform);    // ボーン変換
		XMVECTOR worldPt = XMVector3Transform(skinnedPt, mtxWorld);              // ワールド変換

		worldMin = XMVectorMin(worldMin, worldPt);
		worldMax = XMVectorMax(worldMax, worldPt);
	}

	// AABBをコライダーに格納（ワールド空間）
	XMStoreFloat3(&instance.collider.aabb.minPoint, worldMin);
	XMStoreFloat3(&instance.collider.aabb.maxPoint, worldMax);
}