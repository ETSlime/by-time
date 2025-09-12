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
#include "Collision/CollisionManager.h"
#include "Effects/EffectSystem.h"
#include "Effects/SpecialEffects/LakeSurfaceRenderer.h"
#include "Effects/SpecialEffects/FireBallEffectRenderer.h"
#include "Scene/Ground.h"
#include "Scene/Item/ItemManager.h"
#include "Scene/Item/Interactable/AutumnDoor.h"
#include "Scene/Item/Interactable/Gate.h"
#include "Core/GameSystem.h"
#include "UI/sound.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define _USE_MATH_DEFINES

#define TREE_SIZE			(350.0f)
#define KEY_SIZE			(5.0f)
#define SEED_SIZE			(0.2f)
#define SEEDSOIL_SIZE       (10)
#define AUTUMNDOOR_SIZE       (13)
#define SOIL_SIZE			(100.0f)
#define FIELD_SIZE			(46850.0f)
#define TERRAIN_SIZE		(12.0f)
#define BONFIRE_SIZE		(12.0f)

#define TERRAIN_POS			XMFLOAT3(5371.67f, 5775.0f, 5895.0f)

#define SOIL_POS			XMFLOAT3(8833.67f, 6229.0f, 8084.0f)
#define SOIL_SCALE			XMFLOAT3(48.0f, 31.0f, 46.0f)
#define SOIL_ROT_Y			(-0.55f)

#define BOSSFIGHT_ROOM_POS				XMFLOAT3(8725.67f, 6718.0f, 2411.0f)
#define BOSSFIGHT_ROOM_SCALE			XMFLOAT3(398.0f, 120.0f, 380.0f)
#define BOSSFIGHT_ROOM_ROT_Y			(0.0f)

#define IVY_POS				XMFLOAT3(8881.0f, 7338.0f, 8112.0f)
#define IVY_SCALE			XMFLOAT3(148.0f, 265.0f, 58.0f)
#define IVY_ROT_Y			(1.2f)

#define GATE_SIZE			(13)

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

	LakeSurfaceRenderer* lakeSurface = new LakeSurfaceRenderer(XMFLOAT3(4500.0f, 1500.0f, 4000.0f));
	if (lakeSurface->Initialize(m_renderer.GetDevice(), m_renderer.GetDeviceContext()))
	{
		lakeSurface->SetPosition(XMFLOAT3(7954.0f, 6250.0f, 1996.0f)); // 湖の位置を調整
		EffectSystem::get_instance().AddEffect(lakeSurface);
	}
	else
		SAFE_DELETE(lakeSurface);


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

	if (m_scenesADoorL == nullptr && m_scenesADoorR == nullptr) {

		Transform trans;
		trans.pos = XMFLOAT3(4800.0f, 9245.0f, 11100.0f);//XMFLOAT3(5336.0f, 4814.0f, 5034.0f); // 
		trans.rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
		trans.scl = XMFLOAT3(AUTUMNDOOR_SIZE, AUTUMNDOOR_SIZE, AUTUMNDOOR_SIZE);

		AutumnDoor* leftDoor = new AutumnDoor(DoorSide::Left, trans);

		//trans.pos = XMFLOAT3(5436.0f, 4814.0f, 5034.0f);
		trans.pos.x -= 63 * AUTUMNDOOR_SIZE;

		AutumnDoor* rightDoor = new AutumnDoor(DoorSide::Right, trans);

		leftDoor->SetLinkedDoor(rightDoor);
		rightDoor->SetLinkedDoor(leftDoor);

		// いつでもある
		m_scenesADoorL = ItemManager::get_instance().SpawnItem(leftDoor);
		m_scenesADoorR = ItemManager::get_instance().SpawnItem(rightDoor);
	}
	else {
	
		m_scenesADoorL->ResetStatus();
		m_scenesADoorR->ResetStatus();

	}

	if (m_scenesGate == nullptr) {
	
		Transform trans;
		trans.pos = XMFLOAT3(1170.0f, 6414.0f, -1650.0f);
		trans.rot = XMFLOAT3(0.0f, 1.5f, 0.0f);
		trans.scl = XMFLOAT3(GATE_SIZE, GATE_SIZE, GATE_SIZE);

		m_scenesGate = ItemManager::get_instance().SpawnItem(new Gate(trans));
	
	}

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

	UpdateGameObjectArray<GameObject<ModelInstance>>(m_tempGOs);

	if (m_town)
		m_town->Update();

	if (m_environment)
		m_environment->Update();

	/// INTERACTABLE-IMPLEMENTATION
	/*if (GetCurrentScene() == SceneID::SCENE_1) {
	
		if (m_sceneSoil != nullptr) {
		
			if (m_sceneSoil->GetItemAttributes().used && m_scenePSeed == nullptr)
			{
				Transform trans;
				trans.pos = m_sceneSoil->GetTransform().pos;
				//trans.pos.y -= 100;
				trans.rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
				trans.scl = XMFLOAT3(SEED_SIZE, SEED_SIZE, SEED_SIZE);

				m_scenePSeed = ItemManager::get_instance().SpawnItem(ItemType::IntPSeed, trans);
			}

		}
	
	}*/
#ifdef _DEBUG
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
#endif // _DEBUG
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
		DrawGameObjectArray<GameObject<ModelInstance>>(m_tempGOs);
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
	if (!m_sceneEntityMap.contains(TO_UINT64(m_currentSceneID)))
		return;

	auto& sceneData = m_sceneEntityMap[TO_UINT64(m_currentSceneID)];

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

	m_currentSceneID = SceneID::NONE;
}

void Ground::LoadScene(SceneID sceneID)
{
	if (!m_initialized) return;

	// 現在のシーンと同じ場合は何もしない
	if (m_currentSceneID == sceneID)
		return;

	UnloadCurrentScene(); // 現在のシーンを破棄

	// シーンに対応するロード関数を呼び出す
	auto it = m_sceneLoadMap.find(TO_UINT64(sceneID));
	if (it != m_sceneLoadMap.end())
	{
		SceneLoadFunc func = it->value;
		(this->*func)(); // 登録されたロード関数を呼び出す
		m_currentSceneID = sceneID;
	}

	// "ALL" シーンのロード関数も呼び出す
	auto all = m_sceneLoadMap.find(TO_UINT64(SceneID::ALL));
	if (all != m_sceneLoadMap.end())
	{
		SceneLoadFunc func = all->value;
		(this->*func)(); // 登録されたロード関数を呼び出す
	}
}

void Ground::SetEnableBossfightRoom(bool enable)
{
	if (enable)
	{
		GameObjectConfig airwallConfig{};
		airwallConfig.modelPath = "data/MODEL/Environment/Cube.obj";
		airwallConfig.castShadow = false;
		airwallConfig.drawWorldAABB = false;
		airwallConfig.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
		airwallConfig.colliderTag = ColliderTag::AIR_WALL;
		airwallConfig.position = XMFLOAT3(5180.0f, 6488.0f, 2400.0f);
		airwallConfig.scale = XMFLOAT3(23.0f, 25.0f, 410.0f);
		CreateSceneGO(airwallConfig, SceneID::NONE);

		airwallConfig.position = XMFLOAT3(12637.0f, 6488.0f, 2400.0f);
		airwallConfig.scale = XMFLOAT3(23.0f, 25.0f, 410.0f);
		CreateSceneGO(airwallConfig, SceneID::NONE);

		airwallConfig.position = XMFLOAT3(9407.0f, 6488.0f, 6378.0f);
		airwallConfig.scale = XMFLOAT3(400.0f, 25.0f, 33.0f);
		CreateSceneGO(airwallConfig, SceneID::NONE);

		airwallConfig.position = XMFLOAT3(9407.0f, 6488.0f, -962.0f);
		airwallConfig.scale = XMFLOAT3(400.0f, 25.0f, 36.0f);
		CreateSceneGO(airwallConfig, SceneID::NONE);
	}
	else
	{
		auto& sceneData = m_sceneEntityMap[TO_UINT64(SceneID::NONE)];

		m_tempGOs.clear();

		// まずはシーンから登録解除
		for (ISceneEntity* entity : sceneData)
			m_scene.UnregisterEntity(entity);


		for (ISceneEntity* entity : sceneData)
			entity->Destroy();

		m_scene.DestroyAllMarkedGameObjects();

		sceneData.clear();
	}
}

void Ground::ReinitializeSceneObjects()
{

	if (m_sceneSeed != nullptr) 
		m_sceneSeed->ResetStatus();
	

	if (m_scenesADoorL != nullptr)
		m_scenesADoorL->ResetStatus();

	if (m_scenesADoorR != nullptr)
		m_scenesADoorR->ResetStatus();

	if (m_sceneKey != nullptr)
		m_sceneKey->ResetStatus();

	if (m_sceneKey2 != nullptr)
		m_sceneKey2->ResetStatus();

}

void Ground::InitTerrain(void)
{
	GameObject<ModelInstance>* terrainGO = new GameObject<ModelInstance>();
	GameObjectConfig terrainBuildConfig{};
	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/ground_new.obj";
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

	/*terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/door_l.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/door_r.obj";
	CreateTerrainGO(terrainBuildConfig);*/

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/fence1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/fence2.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/house_new.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/lines.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain1.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain2.obj";
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Level/terrain/mountain3.obj";
	CreateTerrainGO(terrainBuildConfig);

	//terrainBuildConfig.modelPath = "data/MODEL/Level/spring/tree.obj";
	//CreateTerrainGO(terrainBuildConfig);


	// SteleTips - Haru
	terrainBuildConfig.modelPath = "data/MODEL/Level/steletips/Stele_tips_haru.obj";
	terrainBuildConfig.position = XMFLOAT3(8700.0f, 6480.0f, 8700.0f);
	terrainBuildConfig.rotation = XMFLOAT3(0, XM_PI*0.8f, 0);
	CreateTerrainGO(terrainBuildConfig);

	// SteleTips - Aki
	terrainBuildConfig.modelPath = "data/MODEL/Level/steletips/Stele_tips_aki2.obj";
	terrainBuildConfig.position = XMFLOAT3(3750.0f, 9360.0f, 10780.0f);
	terrainBuildConfig.rotation = XMFLOAT3(0, XM_PI/2, 0);
	CreateTerrainGO(terrainBuildConfig);

	// SteleTips - Fuyu
	terrainBuildConfig.modelPath = "data/MODEL/Level/steletips/Stele_tips_fuyu2.obj";
	terrainBuildConfig.position = XMFLOAT3(5236.0f, 6480.0f, 4734.0f);
	terrainBuildConfig.rotation = XMFLOAT3(0, XM_PI, 0);
	CreateTerrainGO(terrainBuildConfig);

	//terrainBuildConfig.modelPath = "data/MODEL/Environment/Cube.obj";
	//terrainBuildConfig.castShadow = false;
	//terrainBuildConfig.drawWorldAABB = false;
	//terrainBuildConfig.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	//terrainBuildConfig.colliderTag = ColliderTag::AIR_WALL;
	//terrainBuildConfig.position = XMFLOAT3(9731.0f, 4142.0f, 2443.0f);
	//terrainBuildConfig.scale = XMFLOAT3(277.0f, 269.0f, 370.0f);
	//terrainBuildConfig.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//CreateTerrainGO(terrainBuildConfig);

	//terrainBuildConfig.position = XMFLOAT3(6608.0f, 4353.0f, 2173.0f);
	//terrainBuildConfig.scale = XMFLOAT3(32.0f, 269.0f, 344.0f);
	//CreateTerrainGO(terrainBuildConfig);

	//terrainBuildConfig.position = XMFLOAT3(6017.0f, 4353.0f, 2169.0f);
	//terrainBuildConfig.scale = XMFLOAT3(32.0f, 287.0f, 280.0f);
	//CreateTerrainGO(terrainBuildConfig);

	//terrainBuildConfig.position = XMFLOAT3(5426.0f, 4353.0f, 2450.0f);
	//terrainBuildConfig.scale = XMFLOAT3(33.0f, 287.0f, 185.0f);
	//CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.modelPath = "data/MODEL/Environment/Cube.obj";
	terrainBuildConfig.castShadow = false;
	terrainBuildConfig.drawWorldAABB = false;
	terrainBuildConfig.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	terrainBuildConfig.colliderTag = ColliderTag::AIR_WALL;
	terrainBuildConfig.position = XMFLOAT3(-2933.0f, 4353.0f, 4277.0f);
	terrainBuildConfig.scale = XMFLOAT3(25.0f, 234.0f, 1000.0f);
	terrainBuildConfig.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(4013.0f, 4072.0f, -2390.0f);
	terrainBuildConfig.scale = XMFLOAT3(1000.0f, 234.0f, 25.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(13539.0f, 4072.0f, 4868.0f);
	terrainBuildConfig.scale = XMFLOAT3(25.0f, 234.0f, 1000.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(6655.0f, 4072.0f, 14119.0f);
	terrainBuildConfig.scale = XMFLOAT3(1000.0f, 234.0f, 25.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(9002.0f, 8708.0f, 11350.0f);
	terrainBuildConfig.scale = XMFLOAT3(8.6f, 16.3f, 8.7f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(8546.0f, 8708.0f, 10529.0f);
	terrainBuildConfig.scale = XMFLOAT3(8.6f, 16.3f, 8.7f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7864.0f, 8708.0f, 10244.0f);
	terrainBuildConfig.scale = XMFLOAT3(5.8f, 16.3f, 5.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7763.0f, 8889.0f, 9911.0f);
	terrainBuildConfig.scale = XMFLOAT3(5.8f, 16.3f, 5.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7434.0f, 8889.0f, 9863.0f);
	terrainBuildConfig.scale = XMFLOAT3(5.8f, 16.3f, 5.0f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7737.0f, 8095.0f, 11407.0f);
	terrainBuildConfig.scale = XMFLOAT3(5.8f, 61.7f, 143.1f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7887.0f, 8177.0f, 11838.0f);
	terrainBuildConfig.scale = XMFLOAT3(12.4f, 6.1f, 12.3f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(7174.1f, 7885.4f, 10801.6f);
	terrainBuildConfig.scale = XMFLOAT3(68.8f, 58.6f, 95.1f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(5778.3f, 6297.6f, 11385.0f);
	terrainBuildConfig.scale = XMFLOAT3(220.9f, 51.2f, 158.7f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(9926.3f, 6297.6f, 11320.0f);
	terrainBuildConfig.scale = XMFLOAT3(56.1f, 50.5f, 60.6f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(11870.2f, 6297.6f, 6913.2f);
	terrainBuildConfig.scale = XMFLOAT3(61.6f, 50.5f, 62.8f);
	CreateTerrainGO(terrainBuildConfig);

	terrainBuildConfig.position = XMFLOAT3(4652.5f, 7690.8f, 11416.0f);
	terrainBuildConfig.scale = XMFLOAT3(109.3f, 79.8f, 161.0f);
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

GameObject<ModelInstance>* Ground::CreateSceneGO(GameObjectConfig config, SceneID sceneID)
{
	GameObject<ModelInstance>* staticGO = new GameObject<ModelInstance>();
	staticGO->Instantiate(config);
	if (sceneID == SceneID::NONE)
		m_tempGOs.push_back(staticGO);
	else
		m_staticGOs.push_back(staticGO);

	RegisterModelLoadMap(config.modelPath, false, false);
	if (sceneID != SceneID::ALL)
		m_sceneEntityMap[TO_UINT64(sceneID)].push_back(staticGO);

	return staticGO;
}

void Ground::InitSceneLoadMap(void)
{
	m_sceneLoadMap[TO_UINT64(SceneID::Spring_Day)] = &Ground::LoadSceneSpringDay;
	m_sceneLoadMap[TO_UINT64(SceneID::Spring_Night)] = &Ground::LoadSceneSpringNight;
	m_sceneLoadMap[TO_UINT64(SceneID::Summer_Day)] = &Ground::LoadSceneSummerDay;
	m_sceneLoadMap[TO_UINT64(SceneID::Summer_Night)] = &Ground::LoadSceneSummerNight;
	m_sceneLoadMap[TO_UINT64(SceneID::Autumn_Day)] = &Ground::LoadSceneAutumnDay;
	m_sceneLoadMap[TO_UINT64(SceneID::Autumn_Night)] = &Ground::LoadSceneAutumnNight;
	m_sceneLoadMap[TO_UINT64(SceneID::Winter_Day)] = &Ground::LoadSceneWinterDay;
	m_sceneLoadMap[TO_UINT64(SceneID::Winter_Night)] = &Ground::LoadSceneWinterNight;

	m_sceneLoadMap[TO_UINT64(SceneID::ALL)] = &Ground::LoadSceneAll;
}

void Ground::LoadSceneSpringDay(void)
{
	if(m_sceneKey != nullptr)
		m_sceneKey->SetActive(false);

	if (m_sceneKey2 != nullptr)
		m_sceneKey2->SetActive(false);

	/// INTERACTABLE-IMPLEMENTATION
	/*if (m_sceneVines != nullptr)
		m_sceneVines->SetActive(false);

	if (m_skybox != nullptr)
		m_skybox->SetCurrentTime(DAY_TIME);*/

	m_skybox.SetSeason(SeasonID::SPRING);
	m_skybox.SetCurrentTime(DAY_TIME);

	//ParticleEffectParams params;
	//params.type = EffectType::Smoke;
	//params.position = XMFLOAT3(12282.0f, -1990.0f, -19485.0f);
	//params.scale = 250.0f;
	//params.acceleration = XMFLOAT3(0.0f, 15.0f, 0.0f);
	//params.spawnRateMin = 2.0f;
	//params.spawnRateMax = 5.0f;
	//params.lifeMin = 3.0f;
	//params.lifeMax = 6.0f;
	//params.startColor = XMFLOAT4(0.6f, 0.6f, 0.6f, 0.3f); // 煙色
	//IEffectRenderer* smoke = EffectSystem::get_instance().SpawnParticleEffect(params);
	//m_sceneEntityMap[TO_UINT64(SceneID::SCENE_1)].push_back(smoke);

	BOUNDING_BOX triggerBox;
	triggerBox.minPoint = XMFLOAT3(8547.6f, 6394.0f , 7774.6f);
	triggerBox.maxPoint = XMFLOAT3(8848.2f, 6490.0f, 8133.0f);

	RegisterEventTrigger(triggerBox, EventCallbackAction::PlaceItem, SceneID::Spring_Day);


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
		trans.rot = XMFLOAT3(1.0f, 1.0f, 10.0f);
		trans.scl = XMFLOAT3(SEED_SIZE, SEED_SIZE, SEED_SIZE);

		m_sceneSeed = ItemManager::get_instance().SpawnItem(new Item(ItemType::Seed, trans));

	}
	else if (!m_sceneSeed->GetPickedUp())
		m_sceneSeed->SetActive(true);

	if (m_sceneSoil == nullptr)
	{

		Transform trans;
		trans.pos = XMFLOAT3(-1000.0f, -5050.0f, 800.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
		trans.scl = XMFLOAT3(SEEDSOIL_SIZE, SEEDSOIL_SIZE, SEEDSOIL_SIZE);

		m_sceneSoil = ItemManager::get_instance().SpawnItem(new Item(ItemType::IntSoil, trans));

	}
	else {
		if (!m_sceneSoil->GetUsed())
			m_sceneSoil->SetActive(true);
		else {

			if (m_scenePSeed != nullptr)
				m_scenePSeed->SetActive(true);

		}
	}

	LoadLakeAirWalls(SceneID::Spring_Day);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/spring/flowers_big.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	springConfig.modelPath = "data/MODEL/Level/spring/flowers_small.obj";
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	springConfig.modelPath = "data/MODEL/Level/spring/trees_new.obj";
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	m_pendingBGM = SOUND_LABEL_BGM_spring;
}

void Ground::LoadSceneSpringNight(void)
{
	if (m_sceneKey == nullptr) 
	{

		Transform trans;
		trans.pos = XMFLOAT3(10000.0f, 9950.0f, 8770.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = XMFLOAT3(KEY_SIZE, KEY_SIZE, KEY_SIZE);
	
		m_sceneKey = ItemManager::get_instance().SpawnItem(new Item(ItemType::Key, trans));

	}
	else if (!m_sceneKey->GetPickedUp())
		m_sceneKey->SetActive(true);

	if (m_sceneKey2 != nullptr)
		m_sceneKey2->SetActive(false);

	/*if (m_scenePSeed != nullptr)
	{

		if(m_sceneVines == nullptr){

			Transform trans = m_scenePSeed->GetTransform();

			trans.pos.y -= 200;
			trans.rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
			trans.scl = XMFLOAT3(120, 120, 120);

			m_sceneVines = ItemManager::get_instance().SpawnItem(new Item(ItemType::IntVines, trans));
		}
		else if (!m_sceneVines->GetUsed())
			m_sceneVines->SetActive(true);
		

	}*/

	if (m_sceneSoil != nullptr)
		m_sceneSoil->SetActive(false);
	
	if (m_scenePSeed != nullptr)
		m_scenePSeed->SetActive(false);

	if (m_sceneSeed != nullptr)
		m_sceneSeed->SetActive(false);

	m_skybox.SetSeason(SeasonID::SPRING);
	m_skybox.SetCurrentTime(NIGHT_TIME);
	

	//FireBallEffectParams fireparams;
	//fireparams.type = EffectType::FireBall;
	//fireparams.position = XMFLOAT3(11882.0f, -1990.0f, -19485.0f);
	//fireparams.scale = 43.0f;
	//fireparams.lifeMin = 1.0f;
	//fireparams.lifeMax = 1.0f;
	//fireparams.spawnRateMin = 0.0f;
	//fireparams.spawnRateMax = 10.0f;
	//fireparams.acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//fireparams.startColor = XMFLOAT4(1.0f, 1.0f, 0.3f, 1.0f);
	//fireparams.endColor = XMFLOAT4(1.2f, 0.5f, 0.0f, 0.0f);
	//fireparams.tilesX = FIREBALL_ANIM_TILE_X;
	//fireparams.tilesY = FIREBALL_ANIM_TILE_Y;
	//fireparams.coneAngleDegree = 25.0f;
	//fireparams.coneRadius = 0.6f;
	//fireparams.coneLength = 5.0f;
	//fireparams.frameLerpCurve = 1.0f;
	//fireparams.rotationSpeed = 0.0f;
	//fireparams.startSpeedMin = 2.0f;
	//fireparams.startSpeedMax = 3.0f;
	//IEffectRenderer* effect = EffectSystem::get_instance().SpawnParticleEffect(fireparams);
	//m_sceneEntityMap[TO_UINT64(SceneID::SCENE_2)].push_back(effect);

	Player* player = GameSystem::get_instance().GetPlayer();
	

	if (player->GetPlayerAttributes().plantedSeed)
	{

		//if (m_sceneVines == nullptr) {

			GameObjectConfig IvyConfig{};
			IvyConfig.modelPath = "data/MODEL/Environment/Ivy.obj";
			IvyConfig.collisionType = ObjectCollisionType::NONE;
			IvyConfig.colliderTag = ColliderTag::STATIC_OBJECT;
			IvyConfig.scale = IVY_SCALE;
			IvyConfig.position = IVY_POS;
			IvyConfig.rotation = XMFLOAT3(0.0f, IVY_ROT_Y, 0.0f);
			IvyConfig.drawWorldAABB = false;
			IvyConfig.castShadow = true;
			IvyConfig.loadAsync = false;
			GameObject<ModelInstance>* ivyGO = new GameObject<ModelInstance>();
			ivyGO->Instantiate(IvyConfig);
			m_staticGOs.push_back(ivyGO);
			m_sceneEntityMap[TO_UINT64(SceneID::Spring_Night)].push_back(ivyGO);

			if (!m_modelLoadMap.contains(IvyConfig.modelPath))
			{
				m_modelLoadMap[IvyConfig.modelPath] = 0;
				m_totalModelLoadCnt++;
			}


			BOUNDING_BOX triggerBox;
			triggerBox.minPoint = XMFLOAT3(8550.6f, 6367.0f, 7835.6f);
			triggerBox.maxPoint = XMFLOAT3(8814.2f, 6404.0f, 8136.0f);

			RegisterEventTrigger(triggerBox, EventCallbackAction::StartClimbUp, SceneID::Spring_Night);

			triggerBox.minPoint = XMFLOAT3(8625.6f, 9717.0f, 7986.6f);
			triggerBox.maxPoint = XMFLOAT3(8964.2f, 9830.0f, 8399.0f);

			RegisterEventTrigger(triggerBox, EventCallbackAction::StopClimbUp, SceneID::Spring_Night);
		//}
	}

	LoadLakeAirWalls(SceneID::Spring_Night);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/spring/flowers_big.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	springConfig.modelPath = "data/MODEL/Level/spring/flowers_small.obj";
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	springConfig.modelPath = "data/MODEL/Level/spring/trees_new.obj";
	CreateSceneGO(springConfig, SceneID::Spring_Day);

	m_pendingBGM = SOUND_LABEL_BGM_spring;
}

void Ground::LoadSceneSummerDay(void)
{
	m_skybox.SetCurrentTime(DAY_TIME);

	LoadLakeAirWalls(SceneID::Summer_Day);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/summer/flowers_small_summer.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Summer_Day);

	springConfig.modelPath = "data/MODEL/Level/summer/trees_summer.obj";
	CreateSceneGO(springConfig, SceneID::Summer_Day);

	if (m_sceneKey2 != nullptr)
		m_sceneKey2->SetActive(false);

	m_skybox.SetSeason(SeasonID::SUMMER);
	m_skybox.SetCurrentTime(DAY_TIME);

	m_pendingBGM = SOUND_LABEL_BGM_summer;
}

void Ground::LoadSceneSummerNight(void)
{
	m_skybox.SetCurrentTime(NIGHT_TIME);

	LoadLakeAirWalls(SceneID::Summer_Day);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/summer/flowers_small_summer.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Summer_Day);

	springConfig.modelPath = "data/MODEL/Level/summer/trees_summer.obj";
	CreateSceneGO(springConfig, SceneID::Summer_Day);
	if (m_sceneKey2 != nullptr)
		m_sceneKey2->SetActive(false);

	m_skybox.SetSeason(SeasonID::SUMMER);
	m_skybox.SetCurrentTime(NIGHT_TIME);

	m_pendingBGM = SOUND_LABEL_BGM_summer;
}

void Ground::LoadSceneAutumnDay(void)
{
	m_skybox.SetCurrentTime(DAY_TIME);

	LoadLakeAirWalls(SceneID::Autumn_Day);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/autumn/trees_autumn.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Autumn_Day);

	springConfig.modelPath = "data/MODEL/Level/autumn/flowers_small_autumn.obj";
	CreateSceneGO(springConfig, SceneID::Autumn_Day);
	if (m_sceneKey2 != nullptr)
		m_sceneKey2->SetActive(false);

	m_skybox.SetSeason(SeasonID::AUTUMN);
	m_skybox.SetCurrentTime(DAY_TIME);

	m_pendingBGM = SOUND_LABEL_BGM_autumn;
}

void Ground::LoadSceneAutumnNight(void)
{
	m_skybox.SetCurrentTime(NIGHT_TIME);

	LoadLakeAirWalls(SceneID::Autumn_Night);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/autumn/trees_autumn.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Autumn_Night);

	springConfig.modelPath = "data/MODEL/Level/autumn/flowers_small_autumn.obj";
	CreateSceneGO(springConfig, SceneID::Autumn_Night);
	if (m_sceneKey2 == nullptr)
	{

		Transform trans;
		trans.pos = XMFLOAT3(4436.0f, 9414.0f, 11834.0f);
		trans.rot = XMFLOAT3(0.0f, 0.0f, -1.0f);
		trans.scl = XMFLOAT3(KEY_SIZE, KEY_SIZE, KEY_SIZE);

		m_sceneKey2 = ItemManager::get_instance().SpawnItem(new Item(ItemType::Key, trans));

	}
	else if (!m_sceneKey2->GetPickedUp())
		m_sceneKey2->SetActive(true);

	m_skybox.SetSeason(SeasonID::AUTUMN);
	m_skybox.SetCurrentTime(NIGHT_TIME);

	m_pendingBGM = SOUND_LABEL_BGM_autumn;
}

void Ground::LoadSceneWinterDay(void)
{
	m_skybox.SetCurrentTime(DAY_TIME);
	m_skybox.SetSeason(SeasonID::WINTER);

	GameObjectConfig airwallConfig{};
	airwallConfig.modelPath = "data/MODEL/Environment/Cube.obj";
	airwallConfig.castShadow = false;
	airwallConfig.drawWorldAABB = false;
	airwallConfig.collisionType = ObjectCollisionType::OBJECT_BOUNDING_BOX;
	airwallConfig.colliderTag = ColliderTag::ICE_WALL;
	airwallConfig.position = XMFLOAT3(8645.0f, 6000.0f, 2481.0f);
	airwallConfig.scale = XMFLOAT3(382.0f, 25.0f, 410.0f);
	airwallConfig.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	airwallConfig.trackTriangles = true;
	CreateSceneGO(airwallConfig, SceneID::Winter_Day);

	debugGOPosX = airwallConfig.position.x;
	debugGOPosY = airwallConfig.position.y;
	debugGOPosZ = airwallConfig.position.z;
	debugGOScaleX = airwallConfig.scale.x;
	debugGOScaleY = airwallConfig.scale.y;
	debugGOScaleZ = airwallConfig.scale.z;
	debugGORotY = airwallConfig.rotation.y;


	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/winter/trees_winter.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Winter_Day);

	springConfig.modelPath = "data/MODEL/Level/winter/flowers_small_winter.obj";
	CreateSceneGO(springConfig, SceneID::Winter_Day);

	m_pendingBGM = SOUND_LABEL_BGM_winter;
}

void Ground::LoadSceneWinterNight(void)
{
	m_skybox.SetCurrentTime(NIGHT_TIME);
	m_skybox.SetSeason(SeasonID::WINTER);

	GameObjectConfig airwallConfig{};
	airwallConfig.modelPath = "data/MODEL/Environment/Cube.obj";
	airwallConfig.castShadow = false;
	airwallConfig.drawWorldAABB = false;
	airwallConfig.collisionType = ObjectCollisionType::OBJECT_BOUNDING_BOX;
	airwallConfig.colliderTag = ColliderTag::ICE_WALL;
	airwallConfig.position = XMFLOAT3(8645.0f, 6000.0f, 2481.0f);
	airwallConfig.scale = XMFLOAT3(382.0f, 25.0f, 410.0f);
	airwallConfig.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	airwallConfig.trackTriangles = true;
	CreateSceneGO(airwallConfig, SceneID::Winter_Night);

	GameObjectConfig springConfig{};
	springConfig.modelPath = "data/MODEL/Level/winter/trees_winter.obj";
	springConfig.castShadow = false;
	springConfig.drawWorldAABB = false;
	springConfig.collisionType = ObjectCollisionType::NONE;
	springConfig.scale = XMFLOAT3(TERRAIN_SIZE, TERRAIN_SIZE, TERRAIN_SIZE);
	springConfig.position = TERRAIN_POS;
	springConfig.flip = AxisFlip::FlipX;
	springConfig.trackTriangles = true;
	CreateSceneGO(springConfig, SceneID::Winter_Night);

	springConfig.modelPath = "data/MODEL/Level/winter/flowers_small_winter.obj";
	CreateSceneGO(springConfig, SceneID::Winter_Night);

	m_pendingBGM = SOUND_LABEL_BGM_winter;
}

void Ground::LoadSceneAll(void)
{
	BOUNDING_BOX triggerBox;
	triggerBox.minPoint = XMFLOAT3(-27774.0f, 5753.0f, -27774.0f);
	triggerBox.maxPoint = XMFLOAT3(28133.0f, 6113.0f, 28133.0f);

	RegisterEventTrigger(triggerBox, EventCallbackAction::ResetPosition, SceneID::ALL);
	RegisterEventTrigger(triggerBox, EventCallbackAction::SuddenDeath, SceneID::ALL);

	//debugTrigger = eventTrigger; // デバッグ用トリガー
	//debugTriggerMinX = triggerBox.minPoint.x;
	//debugTriggerMinY = triggerBox.minPoint.y;
	//debugTriggerMinZ = triggerBox.minPoint.z;
	//debugTriggerMaxX = triggerBox.maxPoint.x;
	//debugTriggerMaxY = triggerBox.maxPoint.y;
	//debugTriggerMaxZ = triggerBox.maxPoint.z;
}

void Ground::LoadLakeAirWalls(SceneID sceneID)
{
	GameObjectConfig airwallConfig{};
	airwallConfig.modelPath = "data/MODEL/Environment/Cube.obj";
	airwallConfig.castShadow = false;
	airwallConfig.drawWorldAABB = false;
	airwallConfig.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	airwallConfig.colliderTag = ColliderTag::AIR_WALL;
	airwallConfig.position = XMFLOAT3(9731.0f, 4142.0f, 2443.0f);
	airwallConfig.scale = XMFLOAT3(277.0f, 269.0f, 370.0f);
	airwallConfig.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	CreateSceneGO(airwallConfig, sceneID);

	airwallConfig.position = XMFLOAT3(6608.0f, 4353.0f, 2173.0f);
	airwallConfig.scale = XMFLOAT3(32.0f, 269.0f, 344.0f);
	CreateSceneGO(airwallConfig, sceneID);

	airwallConfig.position = XMFLOAT3(6017.0f, 4353.0f, 2169.0f);
	airwallConfig.scale = XMFLOAT3(32.0f, 287.0f, 280.0f);
	CreateSceneGO(airwallConfig, sceneID);

	airwallConfig.position = XMFLOAT3(5426.0f, 4353.0f, 2450.0f);
	airwallConfig.scale = XMFLOAT3(33.0f, 287.0f, 185.0f);
	CreateSceneGO(airwallConfig, sceneID);
}

//=============================================================================
// 季節ごとのBGMラベルを返す
//=============================================================================
int Ground::GetSeasonBGM() const
{
	return m_pendingBGM; // 各 LoadSceneX() で季節ごとにセットされた BGM ラベルを返す
}

void Ground::GenerateEnvironmentObj(void*)
{
	InstanceParams instanceParams;
	instanceParams.type = EnvironmentObjectType::Grass_1;
	instanceParams.transformArray.push_back(InstanceTransformInfo(
		3024.0f,		// posX
		3453.0f,		// posZ
		0.5 * XM_PI,	// rotY
		20.0f,			// scl
		25.0f			// offsetY
	));

	instanceParams.transformArray.push_back(InstanceTransformInfo(
		2453.0f,		// posX
		2453.0f,		// posZ
		0.5,			// rotY
		20.0f,			// scl
		25.0f			// offsetY
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

EventTrigger* Ground::RegisterEventTrigger(const BOUNDING_BOX& triggerBox, EventCallbackAction action, SceneID sceneID)
{
	EventTrigger* eventTriger = new EventTrigger();
	eventTriger->SetBoundingBox(triggerBox);

	// イベントの内容を設定
	if (action < EventCallbackAction::EnemyEventCallback)
	{
		// プレイヤー用
		auto* behavior = new PlayerEnterExitBehavior();
		behavior->SetAction(action);
		eventTriger->SetEventBehavior(behavior);
		eventTriger->Init();
	}
	else
	{
		// 敵用
		auto* behavior = new EnemyEnterExitBehavior();
		behavior->SetAction(action);
		eventTriger->SetEventBehavior(behavior);
		eventTriger->Init();
	}


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
		ImGui::SliderFloat("PosX: ", &debugGOPosX, 1131.0f, 13718.0f, "%.1f");
		ImGui::SliderFloat("PosY: ", &debugGOPosY, 5131.0f, 13718.0f, "%.1f");
		ImGui::SliderFloat("PosZ: ", &debugGOPosZ, -1131.0f, 13718.0f, "%.1f");
		ImGui::SliderFloat("ScaleX: ", &debugGOScaleX, 5.0f, 399.0f, "%.1f");
		ImGui::SliderFloat("ScaleY: ", &debugGOScaleY, 5.0f, 99.0f, "%.1f");
		ImGui::SliderFloat("ScaleZ: ", &debugGOScaleZ, 5.0f, 399.0f, "%.1f");
		ImGui::SliderFloat("RotY: ", &debugGORotY, -3, 3, "%.2f");
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