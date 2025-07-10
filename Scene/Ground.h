//=============================================================================
//
// 地形および環境オブジェクト管理[Ground.h]
// Author : 
// フィールド・町・環境オブジェクトの描画と更新を管理する
// 
//=============================================================================
#pragma once
#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Scene/Environment.h"
#include "Town.h"
#include "Scene/Item/Sword.h"
#include "Effects/Skybox.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_SCENE_NUM				15

#define WORLD_MAX					XMFLOAT3(50000.0f, 50000.0f, 50000.0f)
#define WORLD_MIN					XMFLOAT3(-50000.0f, -50000.0f, -50000.0f)

// 読み込むモデル名
#define MODEL_BANYAN_PATH			"data/MODEL/Environment/Banyan/Big_Banyan_121.obj"
#define	MODEL_ENVIRONMENT_PATH		"data/MODEL/Environment/"
#define	MODEL_FIELD_PATH			"data/MODEL/Environment/Land.obj"
#define	MODEL_TOWN_PATH				"data/MODEL/Environment/Knight"
#define	MODEL_TREE_NAME				"Tree.fbx"
#define	MODEL_FIELD_NAME			"Land.fbx"
#define MODEL_TERRAIN_PATH			"data/MODEL/Environment/Terrain"
#define MODEL_TERRAIN_NAME			"terrain_prototype2.fbx"
//#define MODEL_TERRAIN_NAME			"spaceland2.fbx"
#define MODEL_BONFIRE_PATH			"data/MODEL/Environment/bonfire.obj"
#define MODEL_LAND_PATH				"data/MODEL/Environment/terrain_prototype.obj"


enum class SceneID : uint64_t
{
	NONE,
	SCENE_1,
	SCENE_2,
	SCENE_3,
};

// 前方宣言
class Ground;
// 各シーンに応じたモデルの生成処理などをまとめて呼び出す
typedef void (Ground::* SceneLoadFunc)();

class Ground :public IDebugUI
{
public:
	Ground(Skybox* skybox = nullptr);
	~Ground();
	void Update(void);
	void Draw(void);

	int GetTotalModelLoadCnt(void) const { return m_totalModelLoadCnt; }
	int GetCurrentModelLoadCnt(void) const { return m_currentModelLoadCnt; }

	void UnloadCurrentScene(void);
	void LoadScene(SceneID id);
	SceneID GetCurrentScene(void) { return m_currentScene; }

private:
	void InitTerrain(void);
	void CreateTerrainGO(GameObjectConfig config);
	void InitSceneLoadMap(void);
	// シーンロード関数
	void LoadScene1(void);
	void LoadScene2(void);

	void GenerateEnvironmentObj(void*);
	bool DeferredIsFieldReady(void*) { return m_fieldGO->GetModel(); }

	template<typename T>
	void UpdateGameObjectArray(const SimpleArray<T*>& array);
	template<typename T>
	void DrawGameObjectArray(const SimpleArray<T*>& array);

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Map Model Manager"; };

	int m_currentModelLoadCnt = 0;
	int m_totalModelLoadCnt = 0;
	int m_persistentModelLoadCnt = 0;

	SimpleArray<GameObject<ModelInstance>*>				m_staticGOs;
	SimpleArray<GameObject<ModelInstance>*>             m_persistentStaticGOs;
	SimpleArray<GameObject<SkinnedMeshModelInstance>*>	m_skinnedGOs;
	SimpleArray<GameObject<SkinnedMeshModelInstance>*>  m_persistentSkinnedGOs;

	GameObject<ModelInstance>* m_fieldGO = nullptr;
	bool m_drawBoundingBox = true;

	Town* m_town = nullptr;
	Environment* m_environment = nullptr;

	SceneID m_currentScene = SceneID::NONE;
	Scene& m_scene = Scene::get_instance();
	Renderer& m_renderer = Renderer::get_instance();

	HashMap<const char*, int, CharPtrHash, CharPtrEquals> m_modelLoadMap =
		HashMap<const char*, int, CharPtrHash, CharPtrEquals>(
			MAX_MODEL_TYPE,
			CharPtrHash(),
			CharPtrEquals()
		);

	HashMap<uint64_t, SceneLoadFunc, HashUInt64, EqualUInt64> m_sceneLoadMap = 
		HashMap<uint64_t, SceneLoadFunc, HashUInt64, EqualUInt64>(
			MAX_SCENE_NUM,
			HashUInt64(),
			EqualUInt64()
		);

	HashMap<uint64_t, SimpleArray<ISceneEntity*>, HashUInt64, EqualUInt64> m_sceneEntityMap =
		HashMap<uint64_t, SimpleArray<ISceneEntity*>, HashUInt64, EqualUInt64>(
			MAX_SCENE_NUM,
			HashUInt64(),
			EqualUInt64()
		);

	Item* m_sceneSword = nullptr;
	Skybox* m_skybox = nullptr;
};

template<typename T>
inline void Ground::UpdateGameObjectArray(const SimpleArray<T*>& array)
{
	UINT size = array.getSize();
	for (UINT i = 0; i < size; i++)
	{
		if (m_currentModelLoadCnt < m_totalModelLoadCnt)
		{
			const char* modelPath = array[i]->GetInstance()->modelName;
			if (m_modelLoadMap.contains(modelPath))
			{
				if (array[i]->GetLoad() && m_modelLoadMap[modelPath] == 0)
				{
					m_modelLoadMap[modelPath] = 1;
					m_currentModelLoadCnt++;
				}
			}
		}

		if (array[i]->GetUse() == false) continue;
		array[i]->Update();
	}
}

template<typename T>
inline void Ground::DrawGameObjectArray(const SimpleArray<T*>& array)
{
	UINT size = array.getSize();
	for (UINT i = 0; i < size; i++)
	{
		if (array[i]->GetUse() == false) continue;

		array[i]->Draw();
	}
}
