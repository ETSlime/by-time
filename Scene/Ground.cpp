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
#include "Effects/FireBallEffectRenderer.h"
#include "Scene/ItemManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_TREE			(0)

#define TREE_SIZE			(350.0f)
#define FIELD_SIZE			(46850.0f)
#define TERRAIN_SIZE		(15.0f)
#define BONFIRE_SIZE		(12.0f)

#define PLAYER_INIT_POS		XMFLOAT3(12582.0f, -2111.0f, -19485.0f)
#define TERRAIN_POS			XMFLOAT3(-5371.67f, -5775.0f, -1895.0f)
//=============================================================================
// 初期化処理
//=============================================================================
Ground::Ground(Skybox* skybox)
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // DEBUG

	InitSceneLoadMap();

	BOUNDING_BOX worldBB;
	worldBB.maxPoint = WORLD_MAX;
	worldBB.minPoint = WORLD_MIN;
	CollisionManager::get_instance().InitOctree(worldBB);

	//m_town = new Town();

	//SkinnedGameObjectConfig config;
	//config.modelPath = MODEL_ENVIRONMENT_PATH;
	//config.modelName = MODEL_FIELD_NAME;
	//config.modelType = SkinnedModelType::Field;
	//config.scale = XMFLOAT3(FIELD_SIZE, FIELD_SIZE, FIELD_SIZE);
	//config.position = XMFLOAT3(0.0f, -650.0f, 0.0f);
	//config.rotation = XMFLOAT3(XM_PI / 2, XM_PI, 0.0f);
	//config.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	//GameObject<SkinnedMeshModelInstance>* fieldGO = new GameObject<SkinnedMeshModelInstance>();
	//fieldGO->Instantiate(config);
	//m_persistentSkinnedGOs.push_back(fieldGO);
	//m_fieldGO = fieldGO;

	//if (!m_modelLoadMap.contains(config.modelName))
	//{
	//	m_modelLoadMap[config.modelName] = 0;
	//	m_totalModelLoadCnt++;
	//	m_persistentModelLoadCnt++;
	//}

	InitTerrain();

	//SkinnedGameObjectConfig config;
	//config.modelPath = MODEL_TERRAIN_PATH;
	//config.modelName = MODEL_TERRAIN_NAME;
	//config.modelType = SkinnedModelType::Field;
	//config.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	//config.position = XMFLOAT3(0.0f, -650.0f, 0.0f);
	//config.rotation = XMFLOAT3(XM_PI / 2, XM_PI, 0.0f);
	//config.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	//GameObject<SkinnedMeshModelInstance>* fieldGO = new GameObject<SkinnedMeshModelInstance>();

	//fieldGO->SetPostInitCallback([](void* userData) {
	//	// 遅延用コンテキスト構築
	//	auto* go = static_cast<GameObject<SkinnedMeshModelInstance>*>(userData);
	//	auto* ctx = new InstantiateTaskContext<SkinnedMeshModelInstance>{ go };

	//	DeferredTaskOptions options{};
	//	options.async = true; // 非同期実行
	//	options.debugName = "AsyncFbxModelLoader_InitializeAnim_Ground"; // デバッグ用の名前
	//	// タスクを DeferredTaskSystem に登録
	//	RawDeferredTask::Add(
	//		ctx,
	//		[](void* ptr)
	//		{
	//			auto* ctx = static_cast<InstantiateTaskContext<SkinnedMeshModelInstance>*>(ptr);

	//			ctx->owner->GetSkinnedMeshModel()->SetTerrainDiffuseTexture("data/MODEL/Environment/Terrain/holographic_bright_white_panel_blurry_ultra_realistic.jpg");
	//			ctx->owner->GetSkinnedMeshModel()->SetTerrainDiffuseTexture2("data/MODEL/Environment/Terrain/12.jpg");
	//			ctx->owner->SetPostInit(true);
	//		},
	//		nullptr,
	//		options
	//	);
	//	}, fieldGO);

	//fieldGO->Instantiate(config);
	//m_persistentSkinnedGOs.push_back(fieldGO);
	//m_fieldGO = fieldGO; 


	//if (!m_modelLoadMap.contains(config.modelName))
	//{
	//	m_modelLoadMap[config.modelName] = 0;
	//	m_totalModelLoadCnt++;
	//	m_persistentModelLoadCnt++;
	//}

	//GameObjectConfig config{};
	//config.modelPath = MODEL_LAND_PATH;
	//config.scale = XMFLOAT3(FIELD_SIZE, FIELD_SIZE, FIELD_SIZE);
	//config.position = XMFLOAT3(0.0f, -650.0f, 0.0f);
	//config.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	//GameObject<ModelInstance>* fieldGO = new GameObject<ModelInstance>();
	//fieldGO->Instantiate(config);
	//m_groundGO.push_back(fieldGO);
	//m_fieldGO = fieldGO;

	//GameObject<ModelInstance>* banyanGO = new GameObject<ModelInstance>();
	//GameObjectConfig banyanConfig{};
	//banyanConfig.modelPath = MODEL_BANYAN_PATH;
	//banyanConfig.scale = XMFLOAT3(50.0f, 50.0f, 50.0f);
	//banyanConfig.position = XMFLOAT3(-12280.0f, -2254.0f, -4650.0f);
	//banyanGO->Instantiate(banyanConfig);
	//m_groundGO.push_back(banyanGO);

	
	//DeferredTaskOptions options{};
	//options.async = false; // 同期実行
	//options.debugName = "Ground_GenerateEnvironmentObj"; // デバッグ用の名前
	//// タスクを DeferredTaskSystem に登録
	//DeferredTaskBinder<Ground>::Add(
	//	this,
	//	nullptr,
	//	&Ground::GenerateEnvironmentObj,
	//	&Ground::DeferredIsFieldReady,
	//	options
	//);

	m_skybox = skybox;
}

//=============================================================================
// 終了処理
//=============================================================================
Ground::~Ground()
{
}

//=============================================================================
// 更新処理
//=============================================================================
void Ground::Update(void)
{
	UpdateGameObjectArray<GameObject<ModelInstance>>(m_staticGOs);
	UpdateGameObjectArray<GameObject<ModelInstance>>(m_persistentStaticGOs);
	UpdateGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_skinnedGOs);
	UpdateGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_persistentSkinnedGOs);

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
		DrawGameObjectArray<GameObject<ModelInstance>>(m_staticGOs);
		DrawGameObjectArray<GameObject<ModelInstance>>(m_persistentStaticGOs);
	}
	else if (m_renderer.GetRenderMode() == RenderMode::SKINNED_MESH)
	{
		DrawGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_skinnedGOs);
		DrawGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_persistentSkinnedGOs);
	}

	m_renderer.SetLightModeBuffer(0);
}

void Ground::UnloadCurrentScene(void)
{
	for (auto it = m_modelLoadMap.begin(); it != m_modelLoadMap.end();)
	{
		if ((*it).value == 1)
			it = m_modelLoadMap.erase(it);
		else
			++it;
	}

	m_staticGOs.clear();
	m_skinnedGOs.clear();
	m_totalModelLoadCnt = m_persistentModelLoadCnt;
	m_currentModelLoadCnt = 0;

	if (!m_sceneEntityMap.contains(TO_UINT64(m_currentScene)))
		return;

	auto& sceneData = m_sceneEntityMap[TO_UINT64(m_currentScene)];

	// まずはシーンから登録解除
	for (ISceneEntity* entity : sceneData)
		m_scene.UnregisterEntity(entity);

	// オブジェクトを破棄（Destroy）
	// Unregister() 中で m_renderableObjects や m_entities を操作するため、
	// その後 Destroy() で同じリストを操作するとメモリ破壊やクラッシュ
	for (ISceneEntity* entity : sceneData)
		entity->Destroy();

	// GameObject の破棄は Scene が一括で処理する
	// - GameObject は ShadowRenderer などで使われるため、描画中に削除されると危険！
	// - 安全のため、事前に Unregister してから delete する必要があるのっ！
	// - だから Scene::DestroyAllMarkedGameObjects() で一括破棄を行うよ～！
	m_scene.DestroyAllMarkedGameObjects();

	sceneData.clear();

	m_currentScene = SceneID::NONE;
}

void Ground::LoadScene(SceneID sceneID)
{
	if (m_currentScene == sceneID)
		return;

	UnloadCurrentScene(); // 現在のシーンを破棄

	auto it = m_sceneLoadMap.find(TO_UINT64(sceneID));
	if (it != m_sceneLoadMap.end())
	{
		SceneLoadFunc func = it->value;
		(this->*func)(); // 登録されたロード関数を呼び出す
		m_currentScene = sceneID;
	}
}

void Ground::InitTerrain(void)
{
	GameObject<ModelInstance>* terrainGO = new GameObject<ModelInstance>();
	GameObjectConfig terrainBuildConfig{};
	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/ground.obj";
	terrainBuildConfig.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	terrainBuildConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	terrainBuildConfig.position = TERRAIN_POS;
	terrainBuildConfig.drawWorldAABB = false;
	terrainBuildConfig.flip = AxisFlip::FlipX;
	terrainBuildConfig.loadAsync = false;
	terrainGO->Instantiate(terrainBuildConfig);
	m_persistentStaticGOs.push_back(terrainGO);
	m_fieldGO = terrainGO;

	if (!m_modelLoadMap.contains(terrainBuildConfig.modelPath))
	{
		m_modelLoadMap[terrainBuildConfig.modelPath] = 0;
		m_totalModelLoadCnt++;
		m_persistentModelLoadCnt++;
	}

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/house.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/bridge1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/broken_bridge1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/broken_bridge2.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/CD1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/cubes.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/lines.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain2.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain3.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/tree.obj";
	CreateTerrainGO(terrainBuildConfig);
}

void Ground::CreateTerrainGO(GameObjectConfig config)
{
	GameObject<ModelInstance>* terrainGO = new GameObject<ModelInstance>();
	terrainGO->Instantiate(config);
	m_persistentStaticGOs.push_back(terrainGO);

	if (!m_modelLoadMap.contains(config.modelPath))
	{
		m_modelLoadMap[config.modelPath] = 0;
		m_totalModelLoadCnt++;
		m_persistentModelLoadCnt++;
	}
}

void Ground::InitSceneLoadMap(void)
{
	m_sceneLoadMap[TO_UINT64(SceneID::SCENE_1)] = &Ground::LoadScene1;
	m_sceneLoadMap[TO_UINT64(SceneID::SCENE_2)] = &Ground::LoadScene2;
}

void Ground::LoadScene1(void)
{
	if(m_sceneSword != nullptr)
		m_sceneSword->SetActive(false);

	if (m_skybox != nullptr)
		m_skybox->SetCurrentTime(DAY_TIME);

	if (m_p1SeedCont == nullptr) {

		Transform trans;
		trans.pos = XMFLOAT3(-4371.0f, -4770.0f, -1595.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = SWORD_SIZE;

		m_p1SeedCont = ItemManager::get_instance().SpawnItem(ItemType::Interactable, trans);

		m_p1SeedCont->Initialize();

	}

	if(m_p1SeedCont->GetLoad())
		m_p1SeedCont->SetActive(true);

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
	IEffectRenderer* effect = EffectSystem::get_instance().SpawnParticleEffect(params);
	m_sceneEntityMap[TO_UINT64(SceneID::SCENE_1)].push_back(effect);

	//GameObject<ModelInstance>* bonfireGO = new GameObject<ModelInstance>();
	//GameObjectConfig bonfireConfig{};
	////bonfireConfig.modelPath = MODEL_BONFIRE_PATH;
	//bonfireConfig.modelPath = "data/MODEL/Level/terrain_prototype3.obj";
	//bonfireConfig.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	//bonfireConfig.scale = XMFLOAT3(BONFIRE_SIZE * 1, BONFIRE_SIZE * 1, BONFIRE_SIZE * 1);
	//bonfireConfig.position = XMFLOAT3(-5371.67f, -5775.0f, -1895.0f);
	//bonfireConfig.drawWorldAABB = false;
	//bonfireGO->Instantiate(bonfireConfig);
	//m_staticGOs.push_back(bonfireGO);
	//m_sceneEntityMap[TO_UINT64(SceneID::SCENE_1)].push_back(bonfireGO);

	//if (!m_modelLoadMap.contains(bonfireConfig.modelPath))
	//{
	//	m_modelLoadMap[bonfireConfig.modelPath] = 0;
	//	m_totalModelLoadCnt++;
	//}
}

void Ground::LoadScene2(void)
{
	if (m_sceneSword == nullptr) {

		Transform trans;
		trans.pos = XMFLOAT3(-5371.0f, -4770.0f, -1295.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = SWORD_SIZE;
	
		m_sceneSword = ItemManager::get_instance().SpawnItem(ItemType::Weapon, trans);

		m_sceneSword->Initialize();

	}

	m_sceneSword->SetActive(true);

	if (m_p1SeedCont != nullptr)
		m_p1SeedCont->SetActive(false);

	if (m_skybox != nullptr) 
		m_skybox->SetCurrentTime(NIGHT_TIME);
	

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
	fireparams.tilesX = FIREBALL_ANIM_TILE_X;
	fireparams.tilesY = FIREBALL_ANIM_TILE_Y;
	fireparams.coneAngleDegree = 25.0f;
	fireparams.coneRadius = 0.6f;
	fireparams.coneLength = 5.0f;
	fireparams.frameLerpCurve = 1.0f;
	fireparams.rotationSpeed = 0.0f;
	fireparams.startSpeedMin = 2.0f;
	fireparams.startSpeedMax = 3.0f;
	IEffectRenderer* effect = EffectSystem::get_instance().SpawnParticleEffect(fireparams);
	m_sceneEntityMap[TO_UINT64(SceneID::SCENE_2)].push_back(effect);
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
			m_fieldGO->GetModel(),
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
				GameObject<ModelInstance>* fieldGO = dynamic_cast<GameObject<ModelInstance>*>(ctx->fieldGO);
				if (!fieldGO) return;
				// 環境オブジェクトのインスタンスを生成
				ctx->environment->GenerateInstanceByParams(
					ctx->instanceParams,
					fieldGO->GetModel(),
					ctx->fieldGO->GetBoundingBoxWorld()
				);
			},
			[](void* ptr)
			{
				// モデルのロード要求を発行（非同期ロードを開始）
				auto* ctx = static_cast<GenerateInstanceContext*>(ptr);
				GameObject<ModelInstance>* fieldGO = dynamic_cast<GameObject<ModelInstance>*>(ctx->fieldGO);
				Model* instanceModel = Model::StoreModel(ctx->modelPath);
				return instanceModel != nullptr && fieldGO->GetCollider().isInitialized;
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
		UINT size = m_staticGOs.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_staticGOs[i]->GetUse() == false) continue;
			m_staticGOs[i]->GetModel()->SetDrawBoundingBox(m_drawBoundingBox);
		}

		size = m_persistentStaticGOs.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_persistentStaticGOs[i]->GetUse() == false) continue;
			m_persistentStaticGOs[i]->GetModel()->SetDrawBoundingBox(m_drawBoundingBox);
		}

		size = m_skinnedGOs.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_skinnedGOs[i]->GetUse() == false) continue;
			m_skinnedGOs[i]->GetSkinnedMeshModel()->SetDrawBoundingBox(m_drawBoundingBox);
		}
		
		size = m_persistentSkinnedGOs.getSize();
		for (UINT i = 0; i < size; i++)
		{
			if (m_persistentSkinnedGOs[i]->GetUse() == false) continue;
			m_persistentSkinnedGOs[i]->GetSkinnedMeshModel()->SetDrawBoundingBox(m_drawBoundingBox);
		}
	}

	if(m_environment)
		m_environment->SetDrawBoundingBox(m_drawBoundingBox);
}

