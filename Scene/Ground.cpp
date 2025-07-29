//=============================================================================
//
// Gound処理 [Ground.cpp]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Core/Graphics/Renderer.h"
#include "Core/Async//DeferredTaskAPI.h"
#include "Model/Model.h"
#include "Utility/InputManager.h"
#include "Utility/Debug/Debugproc.h"
#include "Scene/Ground.h"
#include "Collision/CollisionManager.h"
#include "Effects/EffectSystem.h"
#include "Effects/SpecialEffects/FireBallEffectRenderer.h"
#include "Scene/Item/ItemManager.h"

//*****************************************************************************
// マクロ定義r
//*****************************************************************************
#define MAX_TREE			(0)

#define TREE_SIZE			(350.0f)
#define KEY_SIZE			(15.0f)
#define SEED_SIZE			(0.2f)
#define FIELD_SIZE			(46850.0f)
#define TERRAIN_SIZE		(12.0f)
#define BONFIRE_SIZE		(12.0f)

#define TERRAIN_POS			XMFLOAT3(5371.67f, 5775.0f, 5895.0f)

#define SOIL_POS			XMFLOAT3(8833.67f, 6229.0f, 8084.0f)
#define SOIL_SCALE			XMFLOAT3(48.0f, 31.0f, 46.0f)
#define SOIL_ROT_Y			(-0.55f)

#define IVY_POS				XMFLOAT3(8881.0f, 7338.0f, 8112.0f)
#define IVY_SCALE			XMFLOAT3(148.0f, 265.0f, 58.0f)
#define IVY_ROT_Y			(1.2f)
//=============================================================================
// 初期化処理
//=============================================================================

void Ground::Initialize(void)
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // _DEBUG

	InitSceneLoadMap();

	BOUNDING_BOX worldBB;
	worldBB.maxPoint = WORLD_MAX;
	worldBB.minPoint = WORLD_MIN;
	CollisionManager::get_instance().InitOctree(worldBB);

	InitTerrain();

	GameObjectConfig soilConfig{};
	soilConfig.modelPath = "data/MODEL/Environment/soil.obj";
	soilConfig.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	soilConfig.scale = SOIL_SCALE;
	soilConfig.position = SOIL_POS;
	soilConfig.rotation = XMFLOAT3(0.0f, SOIL_ROT_Y, 0.0f);
	soilConfig.drawWorldAABB = false;
	soilConfig.loadAsync = false;
	soilConfig.castShadow = false;
	CreateTerrainGO(soilConfig);



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

	m_initialized = true;
}

//=============================================================================
// 更新処理
//=============================================================================
void Ground::Update(void)
{
	if (!m_initialized) return;

	UpdateGameObjectArray<GameObject<ModelInstance>>(m_staticGOs);
	UpdateGameObjectArray<GameObject<ModelInstance>>(m_persistentStaticGOs);
	UpdateGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_skinnedGOs);
	UpdateGameObjectArray<GameObject<SkinnedMeshModelInstance>>(m_persistentSkinnedGOs);

	//if (InputManager::get_instance().GetKeyboardPress(DIK_K))
	//{
	//	if (m_waterFluid && VoxelBufferUploader::get_instance().IsBuffersUploaded())
	//		m_waterFluid->EmitWater(35.0f, 1);
	//}

	if (m_town)
		m_town->Update();

	if (m_environment)
		m_environment->Update();

	if (debugGO)
	{
		debugGO->SetPosition(XMFLOAT3(debugGOPosX, debugGOPosY, debugGOPosZ));
		debugGO->SetScale(XMFLOAT3(debugGOScaleX, debugGOScaleY, debugGOScaleZ));
		debugGO->SetRotation(XMFLOAT3(0.0f, debugGORotY, 0.0f));
	}

	if (debugTrigger)
	{
		BOUNDING_BOX triggerBox;
		triggerBox.minPoint = XMFLOAT3(debugTriggerMinX, debugTriggerMinY, debugTriggerMinZ);
		triggerBox.maxPoint = XMFLOAT3(debugTriggerMaxX, debugTriggerMaxY, debugTriggerMaxZ);
		debugTrigger->SetBoundingBox(triggerBox);
	}

	if (debugItem)
	{
		debugItem->SetPosition(XMFLOAT3(debugItemPosX, debugItemPosY, debugItemPosZ));
		debugItem->SetScale(XMFLOAT3(debugItemScaleX, debugItemScaleY, debugItemScaleZ));
		debugItem->SetRotation(XMFLOAT3(debugItemRotX, debugItemRotY, debugItemRotZ));
	}
}

//=============================================================================
// 描画処理
//=============================================================================
void Ground::Draw(void)
{
	if (!m_initialized) return;

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
	if (!m_initialized) return;

	for (auto it = m_modelLoadMap.begin(); it != m_modelLoadMap.end();)
	{
		// モデルロード済みのものは削除
		if ((*it).value == 1)
			it = m_modelLoadMap.erase(it);
		else
			++it;
	}

	// 現在のシーンの GameObject をクリア
	m_staticGOs.clear();
	m_skinnedGOs.clear();

	// 現在のシーンのモデルロードカウントをリセット
	m_totalModelLoadCnt = m_persistentModelLoadCnt;
	m_totalTerrainModelLoadCnt = m_persistentTerrainModelLoadCnt;
	m_currentModelLoadCnt = 0;
	m_currentTerrainModelLoadCnt = 0;

	// 現在のシーンが存在しない場合は何もしない
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
	if (!m_initialized) return;

	// 現在のシーンと同じ場合は何もしない
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

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain2.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain3.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/tree.obj";
	CreateTerrainGO(terrainBuildConfig);
}

GameObject<ModelInstance>* Ground::CreateTerrainGO(GameObjectConfig config)
{
	GameObject<ModelInstance>* terrainGO = new GameObject<ModelInstance>();
	terrainGO->Instantiate(config);
	m_persistentStaticGOs.push_back(terrainGO);

	RegisterModelLoadMap(config.modelPath, true, true);

	return terrainGO;
}

void Ground::InitSceneLoadMap(void)
{
	m_sceneLoadMap[TO_UINT64(SceneID::SCENE_1)] = &Ground::LoadScene1;
	m_sceneLoadMap[TO_UINT64(SceneID::SCENE_2)] = &Ground::LoadScene2;
}

void Ground::LoadScene1(void)
{
	if(m_sceneKey != nullptr)
		m_sceneKey->SetActive(false);

	m_skybox.SetCurrentTime(DAY_TIME);

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
	//IEffectRenderer* smoke = EffectSystem::get_instance().SpawnParticleEffect(params);
	//m_sceneEntityMap[TO_UINT64(SceneID::SCENE_1)].push_back(smoke);

	WaterFluidParams waterParams;
	waterParams.type = EffectType::WaterFluid;
	waterParams.scale = 15.0f; // 粒子の基本スケール（sizeの初期値）
	waterParams.position = XMFLOAT3(11882.0f, -1790.0f, -19485.0f); // 初期生成位置
	waterParams.acceleration = XMFLOAT3(0.0f, -65.0f, 0.0f); // 重力方向（Y軸マイナス）
	waterParams.numParticles = 5000; // 最大粒子数
	waterParams.startColor = XMFLOAT4(0.2f, 0.5f, 1.0f, 0.5f); // 水色
	waterParams.endColor = XMFLOAT4(0.0f, 0.3f, 0.7f, 1.0f);   // 寿命で暗く

	// エミット制御（あまり使わないが、baseクラス必須）
	waterParams.spawnRateMin = 0;
	waterParams.spawnRateMax = 0;
	waterParams.lifeMin = 9999.0f; // 実質無限寿命（蒸発未実装なら）
	waterParams.lifeMax = 9999.0f; // 実質無限寿命（蒸発未実装なら）

	// 水の物理パラメータ
	waterParams.supportRadius = 15.0f;		// サポート半径（粒子影響範囲）
	waterParams.restDensity = 50.0f;		// 静止密度（流体の基準密度）
	waterParams.particleMass = 15.0f;		// 粒子質量
	waterParams.stiffness = 4000.0f;		// 拘束強度（大きいと固くなる）
	waterParams.friction = 0.05f;			// 速度減衰（地面との摩擦的な）
	waterParams.viscosity = 5.0f;			// 粘性係数（流体の粘り気）
	waterParams.restitution = 0.0f;			// 反発係数（衝突時の挙動制御用）

	//IEffectRenderer* waterFluid =  EffectSystem::get_instance().SpawnParticleEffect(waterParams);
	//m_sceneEntityMap[TO_UINT64(SceneID::SCENE_2)].push_back(waterFluid);
	//m_waterFluid = dynamic_cast<WaterFluidParticleRenderer*>(waterFluid);

	BOUNDING_BOX triggerBox;
	triggerBox.minPoint = XMFLOAT3(8547.6f, 6394.0f , 7774.6f);
	triggerBox.maxPoint = XMFLOAT3(8848.2f, 6490.0f, 8133.0f);

	RegisterEventTrigger(triggerBox, PlayerCallbackAction::PlaceItem, SceneID::SCENE_1);


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

	if (m_sceneSeed == nullptr) 
	{

		Transform trans;
		trans.pos = XMFLOAT3(5336.0f, 6414.0f, 6734.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = XMFLOAT3(SEED_SIZE, SEED_SIZE, SEED_SIZE);

		m_sceneSeed = ItemManager::get_instance().SpawnItem(ItemType::Seed, trans);

	}
	else if (!m_sceneSeed->GetPickedUp())
		m_sceneSeed->SetActive(true);
}

void Ground::LoadScene2(void)
{
	if (m_sceneKey == nullptr) 
	{

		Transform trans;
		trans.pos = XMFLOAT3(-5371.0f, -4670.0f, -1295.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = XMFLOAT3(KEY_SIZE, KEY_SIZE, KEY_SIZE);
	
		m_sceneKey = ItemManager::get_instance().SpawnItem(ItemType::Key, trans);

	}
	else if (!m_sceneKey->GetPickedUp())
		m_sceneKey->SetActive(true);
	

	m_skybox.SetCurrentTime(NIGHT_TIME);
	

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

	GameObjectConfig IvyConfig{};
	IvyConfig.modelPath = "data/MODEL/Environment/Ivy.obj";
	IvyConfig.collisionType = ObjectCollisionType::MESH_BOUNDING_BOX;
	IvyConfig.scale = IVY_SCALE;
	IvyConfig.position = IVY_POS;
	IvyConfig.rotation = XMFLOAT3(0.0f, IVY_ROT_Y, 0.0f);
	IvyConfig.drawWorldAABB = false;
	IvyConfig.castShadow = true;
	IvyConfig.loadAsync = false;
	GameObject<ModelInstance>* ivyGO = new GameObject<ModelInstance>();
	ivyGO->Instantiate(IvyConfig);
	m_staticGOs.push_back(ivyGO);
	m_sceneEntityMap[TO_UINT64(SceneID::SCENE_2)].push_back(ivyGO);

	if (!m_modelLoadMap.contains(IvyConfig.modelPath))
	{
		m_modelLoadMap[IvyConfig.modelPath] = 0;
		m_totalModelLoadCnt++;
	}


	BOUNDING_BOX triggerBox;
	triggerBox.minPoint = XMFLOAT3(8550.6f, 6367.0f, 7835.6f);
	triggerBox.maxPoint = XMFLOAT3(8814.2f, 6404.0f, 8136.0f);

	EventTrigger* eventTrigger = RegisterEventTrigger(triggerBox, PlayerCallbackAction::StartClimbUp, SceneID::SCENE_2);

	
	debugTrigger = eventTrigger; // デバッグ用トリガー
	debugTriggerMinX = triggerBox.minPoint.x;
	debugTriggerMinY = triggerBox.minPoint.y;
	debugTriggerMinZ = triggerBox.minPoint.z;
	debugTriggerMaxX = triggerBox.maxPoint.x;
	debugTriggerMaxY = triggerBox.maxPoint.y;
	debugTriggerMaxZ = triggerBox.maxPoint.z;

	triggerBox.minPoint = XMFLOAT3(8625.6f, 9717.0f, 7986.6f);
	triggerBox.maxPoint = XMFLOAT3(8964.2f, 9830.0f, 8399.0f);

	RegisterEventTrigger(triggerBox, PlayerCallbackAction::StopClimbUp, SceneID::SCENE_2);


}

void Ground::GenerateEnvironmentObj(void*)
{
	InstanceParams instanceParams;
	instanceParams.type = EnvironmentObjectType::Bush_2;
	instanceParams.transformArray.push_back(InstanceTransformInfo(
		3024.0f,
		3453.0f,
		0.5 * XM_PI,
		0.3f
	));

	instanceParams.transformArray.push_back(InstanceTransformInfo(
		2453.0f,
		2453.0f,
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

	//m_environment->GenerateRandomInstances(EnvironmentObjectType::Shrubbery_1, 15);
}

void Ground::RegisterModelLoadMap(const char* modelPath, bool isPersistent, bool isTerrain)
{
	if (!m_modelLoadMap.contains(modelPath))
	{
		m_modelLoadMap[modelPath] = 0; // 初期値は0
		m_totalModelLoadCnt++; // 全体のロード数を更新
		if (isPersistent)
		{
			m_persistentModelLoadCnt++; // 永続的なモデルロード数も更新
			if (isTerrain)
				m_persistentTerrainModelLoadCnt++; // 永続的な地形モデルのロード数も更新
		}
		if (isTerrain)
			m_totalTerrainModelLoadCnt++; // 地形モデルのロード数も更新
	}
}

EventTrigger* Ground::RegisterEventTrigger(const BOUNDING_BOX& triggerBox, PlayerCallbackAction action, SceneID sceneID)
{
	EventTrigger* eventTriger = new EventTrigger();
	eventTriger->SetBoundingBox(triggerBox);

	// イベントの内容を設定
	auto* behavior = new PlayerEnterExitBehavior();
	behavior->SetAction(action);
	eventTriger->SetEventBehavior(behavior);
	eventTriger->Init();

	// 登録
	m_sceneEntityMap[TO_UINT64(sceneID)].push_back(eventTriger);
	CollisionManager::get_instance().RegisterEventCollider(eventTriger->GetCollider());

	return eventTriger;
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

	if (m_environment)
		m_environment->SetDrawBoundingBox(m_drawBoundingBox);

	if (ImGui::CollapsingHeader("Debug GameObject"))
	{
		ImGui::SliderFloat("PosX: ", &debugGOPosX, 1133.67f, 9833.67f, "%.1f");
		ImGui::SliderFloat("PosY: ", &debugGOPosY, 1131.0f, 8131.0f, "%.1f");
		ImGui::SliderFloat("PosZ: ", &debugGOPosZ, 1418.0f, 8718.0f, "%.1f");
		ImGui::SliderFloat("ScaleX: ", &debugGOScaleX, 25.0f, 255.0f, "%.1f");
		ImGui::SliderFloat("ScaleY: ", &debugGOScaleY, 25.0f, 355.0f, "%.1f");
		ImGui::SliderFloat("ScaleZ: ", &debugGOScaleZ, 25.0f, 255.0f, "%.1f");
		ImGui::SliderFloat("RotY: ", &debugGORotY, -3, 3, "%.1f");
	}

	if (ImGui::CollapsingHeader("Debug EventTrigger"))
	{
		ImGui::SliderFloat("MinX: ", &debugTriggerMinX, 1133.67f, 11111.0f, "%.1f");
		ImGui::SliderFloat("MinY: ", &debugTriggerMinY, 1133.67f, 11111.0f, "%.1f");
		ImGui::SliderFloat("MinZ: ", &debugTriggerMinZ, 1133.67f, 11111.0f, "%.1f");
		ImGui::SliderFloat("MaxX: ", &debugTriggerMaxX, 1133.67f, 11111.0f, "%.1f");
		ImGui::SliderFloat("MaxY: ", &debugTriggerMaxY, 1133.67f, 11111.0f, "%.1f");
		ImGui::SliderFloat("MaxZ: ", &debugTriggerMaxZ, 1133.67f, 11111.0f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Debug Item"))
	{
		ImGui::SliderFloat("PosX: ", &debugItemPosX, 1133.67f, 11111.67f, "%.1f");
		ImGui::SliderFloat("PosY: ", &debugItemPosY, 1131.0f, 11111.0f, "%.1f");
		ImGui::SliderFloat("PosZ: ", &debugItemPosZ, 1418.0f, 11111.0f, "%.1f");
		ImGui::SliderFloat("ScaleX: ", &debugItemScaleX, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("ScaleY: ", &debugItemScaleY, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("ScaleZ: ", &debugItemScaleZ, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("RotX: ", &debugItemRotX, -3, 3, "%.1f");
		ImGui::SliderFloat("RotY: ", &debugItemRotX, -3, 3, "%.1f");
		ImGui::SliderFloat("RotZ: ", &debugItemRotZ, -3, 3, "%.1f");
	}
}