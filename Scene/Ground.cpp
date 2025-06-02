//=============================================================================
//
// Ground処理 [Ground.cpp]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Core/Graphics/Renderer.h"
#include "Core/Async//DeferredTaskAPI.h"
#include "Model/Model.h"
#include "Utility/InputManager.h"
#include "Utility/Debugproc.h"
#include "Scene/Ground.h"
#include "Collision/CollisionManager.h"
#include "Effects/EffectSystem.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************

#define MAX_TREE			(0)

#define TREE_SIZE			(350.0f)
#define FIELD_SIZE			(46850.0f)
#define BONFIRE_SIZE		(12.0f)

#define PLAYER_INIT_POS		XMFLOAT3(12582.0f, -2111.0f, -19485.0f)

//=============================================================================
// 初期化処理
//=============================================================================
Ground::Ground()
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // DEBUG

	Transform trans;
	trans.pos = XMFLOAT3(12937.0f, -2384.0f, -19485.0f);

	//for (int i = 0; i < MAX_TREE; i++)
	//{
	//	float radius = 20000.f;
	//	float r = GetRandFloat(2000.0f, radius);
	//	float angle = GetRandFloat(0.0f, 2.0f * 3.14159265359f);
	//	float x = trans.pos.x + r * cos(angle);
	//	float z = trans.pos.z + r * sin(angle);
	//	float size = GetRandFloat(TREE_SIZE * .8f, TREE_SIZE * 2.5f);
	//	GameObject<SkinnedMeshModelInstance>* treeGO = new GameObject<SkinnedMeshModelInstance>();
	//	treeGO->Instantiate(MODEL_ENVIRONMENT_PATH, MODEL_TREE_NAME, ModelType::Tree);
	//	treeGO->GetInstance()->pModel->SetBodyDiffuseTexture("data/MODEL/Environment/Bark.jpg");
	//	treeGO->SetPosition(XMFLOAT3(x, trans.pos.y, z));
	//	treeGO->SetRotation(XMFLOAT3(0.0f, 0.0f, 0.0f));
	//	treeGO->SetScale(XMFLOAT3(size, size, size));
	//	
	//	treeGO->GetSkinnedMeshModel()->SetBoundingBoxSize(XMFLOAT3(0.1f, 1.0f, 0.1f));
	//	treeGO->GetSkinnedMeshModel()->SetBoundingBoxLocationOffset(XMFLOAT3(-0.55f, 0.0f, 0.2f));
	//	treeGO->SetColliderOwner(treeGO);
	//	treeGO->SetColliderEnable(true);
	//	treeGO->SetColliderTag(ColliderTag::TREE);
	//	CollisionManager::get_instance().RegisterDynamicCollider(&treeGO->GetCollider());

	//	skinnedMeshGroundGO.push_back(treeGO);
	//}

	BOUNDING_BOX worldBB;
	worldBB.maxPoint = WORLD_MAX;
	worldBB.minPoint = WORLD_MIN;
	CollisionManager::get_instance().InitOctree(worldBB);

	//m_town = new Town();

	SkinnedGameObjectConfig config;
	config.modelPath = MODEL_ENVIRONMENT_PATH;
	config.modelName = MODEL_FIELD_NAME;
	config.modelType = SkinnedModelType::Field;
	config.scale = XMFLOAT3(FIELD_SIZE, FIELD_SIZE, FIELD_SIZE);
	config.position = XMFLOAT3(0.0f, -650.0f, 0.0f);
	config.rotation = XMFLOAT3(XM_PI / 2, XM_PI, 0.0f);
	config.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	GameObject<SkinnedMeshModelInstance>* fieldGO = new GameObject<SkinnedMeshModelInstance>();
	fieldGO->Instantiate(config);
	m_skinnedMeshGroundGO.push_back(fieldGO);
	m_fieldGO = fieldGO;

	//GameObject<ModelInstance>* fieldGO = new GameObject<ModelInstance>();
	//fieldGO->Instantiate(MODEL_FIELD_PATH);
	//fieldGO->SetScale(XMFLOAT3(FIELD_SIZE, FIELD_SIZE, FIELD_SIZE));
	//fieldGO->SetRotation(XMFLOAT3(0, 0, 0.0f));
	//fieldGO->SetPosition(XMFLOAT3(0.0f, -650.0f, 0.0f));
	//fieldGO->Update();
	//worldMatrix = fieldGO->GetWorldMatrix();
	//fieldGO->GetModel()->BuildTrianglesByWorldMatrix(worldMatrix, true);
	//fieldGO->GetModel()->BuildOctree();
	//fieldGO->GetModel()->SetDrawBoundingBox(false);
	//fieldGO->SetCastShadow(false);
	//groundGO.push_back(fieldGO);

	GameObject<ModelInstance>* bonfireGO = new GameObject<ModelInstance>();
	GameObjectConfig bonfireConfig{};
	bonfireConfig.modelPath = MODEL_BONFIRE_PATH;
	bonfireConfig.collisionType = ObjectCollisionType::OBJECT_BOUNDING_BOX;
	bonfireConfig.scale = XMFLOAT3(BONFIRE_SIZE, BONFIRE_SIZE, BONFIRE_SIZE);
	bonfireConfig.position = XMFLOAT3(12582.0f, -2051.0f, -19485.0f);
	bonfireConfig.drawWorldAABB = true;
	bonfireGO->Instantiate(bonfireConfig);
	m_groundGO.push_back(bonfireGO);

	//GameObject<ModelInstance>* banyanGO = new GameObject<ModelInstance>();
	//GameObjectConfig banyanConfig{};
	//banyanConfig.modelPath = MODEL_BANYAN_PATH;
	//banyanConfig.scale = XMFLOAT3(50.0f, 50.0f, 50.0f);
	//banyanConfig.position = XMFLOAT3(-12280.0f, -2254.0f, -4650.0f);
	//banyanGO->Instantiate(banyanConfig);
	//m_groundGO.push_back(banyanGO);

	ParticleEffectParams params;
	params.type = EffectType::Smoke;
	params.position = XMFLOAT3(12282.0f, -1990.0f, -19485.0f);
	params.scale = 250.0f;
	params.acceleration = XMFLOAT3(0.0f, 15.0f, 0.0f);
	params.spawnRateMin = 2.0f;
	params.spawnRateMax = 5.0f;
	params.lifeMin = 3.0f;
	params.lifeMax = 6.0f;
	params.startColor = XMFLOAT4(0.6f, 0.6f, 0.6f, 0.3f); // 煙色
	EffectSystem::get_instance().SpawnParticleEffect(params);

	FireBallEffectParams fireparams;
	fireparams.type = EffectType::FireBall;
	fireparams.position = XMFLOAT3(11882.0f, -1990.0f, -19485.0f);
	fireparams.scale = 43.0f;
	fireparams.lifeMin = 1.0f;
	fireparams.lifeMax = 1.0f;
	fireparams.spawnRateMin = 0.0f;
	fireparams.spawnRateMax = 10.0f;
	fireparams.acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
	fireparams.startColor = XMFLOAT4(1.0f, 1.0f, 0.3f, 1.0f);
	fireparams.endColor = XMFLOAT4(1.2f, 0.5f, 0.0f, 0.0f);
	fireparams.tilesX = 7;
	fireparams.tilesY = 7;
	fireparams.coneAngleDegree = 25.0f;
	fireparams.coneRadius = 0.6f;
	fireparams.coneLength = 5.0f;
	fireparams.frameLerpCurve = 1.0f;
	fireparams.rotationSpeed = 0.0f;
	fireparams.startSpeedMin = 2.0f;
	fireparams.startSpeedMax = 3.0f;
	EffectSystem::get_instance().SpawnParticleEffect(fireparams);
	
	DeferredTaskOptions options{};
	options.async = false; // 同期実行
	options.debugName = "Ground_GenerateEnvironmentObj"; // デバッグ用の名前
	// タスクを DeferredTaskSystem に登録
	DeferredTaskBinder<Ground>::Add(
		this,
		nullptr,
		&Ground::GenerateEnvironmentObj,
		&Ground::DeferredIsFieldReady,
		options
	);
}

//=============================================================================
// 終了処理
//=============================================================================
Ground::~Ground()
{
	int size = m_groundGO.getSize();
	for (int i = 0; i < size; i++)
	{
		if (m_groundGO[i])
			SAFE_DELETE(m_groundGO[i]);
	}
}

//=============================================================================
// 更新処理
//=============================================================================
void Ground::Update(void)
{
	int size = m_groundGO.getSize();
	for (int i = 0; i < size; i++)
	{
		if (m_groundGO[i]->GetUse() == false) continue;
		m_groundGO[i]->Update();
	}

	size = m_skinnedMeshGroundGO.getSize();
	for (int i = 0; i < size; i++)
	{
		if (m_skinnedMeshGroundGO[i]->GetUse() == false) continue;
		m_skinnedMeshGroundGO[i]->Update();

		//if (skinnedMeshGroundGO[i]->GetCollider().type == ColliderTag::TREE)
		//{
		//	if (skinnedMeshGroundGO[i]->GetAttributes().isGrounded == false)
		//	{
		//		Transform transform = skinnedMeshGroundGO[i]->GetTransform();
		//		transform.pos.y -= 25;
		//		skinnedMeshGroundGO[i]->SetTransform(transform);
		//	}
		//}
	}

	if (m_town)
		m_town->Update();

	if (m_environment)
		m_environment->Update();
}

//=============================================================================
// 描画処理
//=============================================================================
void Ground::Draw(void)
{
	m_renderer.SetLightModeBuffer(1);

	if (m_town)
		m_town->Draw();

	if (m_renderer.GetRenderMode() == RenderMode::INSTANCE)
	{
		if (m_environment)
			m_environment->Draw();
	}
	else if (m_renderer.GetRenderMode() == RenderMode::OBJ)
	{
		UINT size = m_groundGO.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_groundGO[i]->GetUse() == false) continue;

			m_groundGO[i]->Draw();
		}
	}
	else if (m_renderer.GetRenderMode() == RenderMode::SKINNED_MESH)
	{
		UINT size = m_skinnedMeshGroundGO.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_skinnedMeshGroundGO[i]->GetUse() == false)
				continue;

			m_skinnedMeshGroundGO[i]->Draw();
		}
	}

	m_renderer.SetLightModeBuffer(0);
}

void Ground::GenerateEnvironmentObj(void*)
{
	InstanceParams instanceParams;
	instanceParams.type = EnvironmentObjectType::Bush_2;
	instanceParams.transformArray.push_back(InstanceTransformInfo(
		12882.0f,
		-18385.0f,
		0.5 * XM_PI,
		0.3f
	));

	instanceParams.transformArray.push_back(InstanceTransformInfo(
		12482.0f,
		-17385.0f,
		0.5,
		0.3f
	));

	// 環境オブジェクト管理クラスのインスタンス生成
	m_environment = new Environment();
	const char* environmentModelPath = Environment::GetModelPathByType(instanceParams.type);

	// モデルが既にロード完了している場合は、即座にインスタンス生成を実行
	if (Model::GetModel(environmentModelPath))
	{
		m_environment->GenerateInstanceByParams(
			instanceParams,
			m_fieldGO->GetSkinnedMeshModel(),
			m_fieldGO->GetBoundingBoxWorld());
	}
	else // モデルがロードされていない場合は非同期でロードしてから実行
	{
		// 遅延用コンテキスト構築
		auto* ctx = new GenerateInstanceContext{
			m_environment,
			instanceParams,
			m_fieldGO,
			environmentModelPath
		};
		DeferredTaskOptions options{};
		options.async = false; // 非同期実行
		options.debugName = "Ground_GenerateInstanceByParams"; // デバッグ用の名前

		RawDeferredTask::Add(
			ctx,
			[](void* ptr)
			{
				auto* ctx = static_cast<GenerateInstanceContext*>(ptr);
				GameObject<SkinnedMeshModelInstance>* fieldGO = dynamic_cast<GameObject<SkinnedMeshModelInstance>*>(ctx->fieldGO);
				if (!fieldGO) return;
				// 環境オブジェクトのインスタンスを生成
				ctx->environment->GenerateInstanceByParams(
					ctx->instanceParams,
					fieldGO->GetSkinnedMeshModel(),
					ctx->fieldGO->GetBoundingBoxWorld()
				);
			},
			[](void* ptr)
			{
				// モデルのロード要求を発行（非同期ロードを開始）
				auto* ctx = static_cast<GenerateInstanceContext*>(ptr);
				Model::StoreModel(ctx->modelPath);
				return Model::GetModel(ctx->modelPath) != nullptr;
			},
			options
		);
	}
	//environment->GenerateRandomInstances(EnvironmentObjectType::Shrubbery_1, fieldGO->GetSkinnedMeshModel(), 15);
}

void Ground::RenderImGui(void)
{
	if (ImGui::Checkbox("Draw Scene Objct Bounding Box", &m_drawBoundingBox)) 
	{
		UINT size = m_skinnedMeshGroundGO.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_skinnedMeshGroundGO[i]->GetUse() == false) continue;
			m_groundGO[i]->GetModel()->SetDrawBoundingBox(m_drawBoundingBox);
		}
	}
	if(m_environment)
		m_environment->SetDrawBoundingBox(m_drawBoundingBox);
}

