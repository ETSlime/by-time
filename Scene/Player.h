#pragma once
//=============================================================================
//
// プレイヤー操作＆主役ちゃん管理クラス [Player.h]
// Author : 
// プレイヤーの行動・移動・描画・エフェクトを一括で担当する主人公ちゃん管理クラスですっ！
// SigewinneやKleeたちの切り替えもここで制御して、アクションキューで動きを可愛く整列します
// 
//=============================================================================
#include "Scene/Character/Sigewinne.h"
#include "Scene/Character/Klee.h"
#include "Scene/Character/Lumine.h"
#include "Scene/Character/Hilichurl.h"
#include "Scene/Character/Mitachurl.h"
#include "Scene/Item/Item.h"
#include "Scene/Item/Interactable.h"
#include "Core/LightManager.h"
#include "Utility/InputManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	VALUE_MOVE			(6.5f)							// 移動量
#define VALUE_CLIMB			(4.0f)							// クライム移動量
#define VALUE_RUN_FACTOR	(1.5f)
#define	VALUE_ROTATE		(XM_PI * 0.02f)					// 回転量
#define ROTATION_SPEED				(0.18f)
#define FALLING_SPEED		(2.50f)
#define SPD_DECAY_RATE		(0.93f)
#define PLAYER_INIT_POS		XMFLOAT3(-5371.67f, -4775.0f, -1895.0f)

class Player : public IDebugUI
{
public:
	Player();
	~Player();
	void Update(void);
	void Draw(void);
	void DrawEffect(void);
	Transform GetTransform(void) const;

	void InitializePlayerStatus(void);

	int   GetTotalModelLoadCnt(void) const { return m_totalModelLoadCnt; }
	int GetCurrentModelLoadCnt(void) const { return m_currentModelLoadCnt; }

	void PickItem(Item* obj);

	void SetClimbMode(bool enable);

	static void OnClimbPromptTriggered(void* userData)
	{
		static_cast<Player*>(userData)->SetClimbMode(true);
	}

	bool GetMeleeMode(void) const { return m_playerAttr.isMeleeMode; }
	bool GetHasSeed(void) const { return m_playerAttr.hasSeed; }

	void LinkInteractable(Interactable* interactable);

private:
	void UpdateNormalMode(void);
	void UpdateClimbMode(void);
	void UpdateActionQueue(void);
	void HandlePlayerMove(Transform& transform);

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Player"; };

	Attributes m_attributes;
	Transform m_transform;
	PlayerAttributes m_playerAttr;
	Sigewinne* sigewinne;
	Klee* klee;
	Lumine* lumine;

	Hilichurl* hilichurl;
	Mitachurl* mitachurl;

	bool m_drawBoundingBox = true;
	int m_currentModelLoadCnt = 0;
	int m_totalModelLoadCnt = 1;
	
	GameObject<SkinnedMeshModelInstance>* m_playerGO;

	Light* m_light;

	Renderer& m_renderer = Renderer::get_instance();
	LightManager& m_lightMgr = LightManager::get_instance();

	DebugProc& m_debugProc = DebugProc::get_instance();
	InputManager& m_inputManager = InputManager::get_instance();

	Interactable*   m_linkedInteractable = nullptr;

	bool m_hasWeapon = false;
};