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
//*****************************************************************************
// マクロ定義
//*****************************************************************************
// 読み込むモデル名
#define MODEL_BANYAN_PATH			"data/MODEL/Environment/Banyan/Big_Banyan_121.obj"
#define	MODEL_ENVIRONMENT_PATH		"data/MODEL/Environment/"
#define	MODEL_FIELD_PATH			"data/MODEL/Environment/Land.obj"
#define	MODEL_TOWN_PATH				"data/MODEL/Environment/Knight"
#define	MODEL_TREE_NAME				"Tree.fbx"
#define	MODEL_FIELD_NAME			"Land.fbx"
#define MODEL_BONFIRE_PATH			"data/MODEL/Environment/bonfire.obj"

#define WORLD_MAX			(XMFLOAT3(50000.0f, 50000.0f, 50000.0f))
#define WORLD_MIN			(XMFLOAT3(-50000.0f, -50000.0f, -50000.0f))
//*****************************************************************************
// 構造体定義
//*****************************************************************************

class Ground :public IDebugUI
{
public:
	Ground();
	~Ground();
	void Update(void);
	void Draw(void);
	void CollectStaticShadowMeshes(SimpleArray<StaticRenderData>& outMeshes);

private:

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Map Model Manager"; };

	SimpleArray<GameObject<SkinnedMeshModelInstance>*> m_skinnedMeshGroundGO;
	SimpleArray<GameObject<ModelInstance>*>	m_groundGO;

	bool m_drawBoundingBox = true;

	Town* m_town = nullptr;
	Environment* m_environment = nullptr;
	Renderer& m_renderer = Renderer::get_instance();
};