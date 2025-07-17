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
#include "Scene/Item.h"
#include "Core/LightManager.h"
#include "Utility/InputManager.h"
#include "Scene/HPBarUI.h"
#include "UI/Base/UIManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define ACTION_QUEUE_SIZE		(4)
#define ACTION_QUEUE_CLEAR_WAIT	(40)
#define PLAYER_HIT_WINDOW		(90)
#define Action(action)			static_cast<UINT>(action)

//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct PlayerAction
{
	ActionEnum	actionType;
	float		liveTime;
};

struct PlayerAttributes
{
	UINT			actionQueueClearTime;
	PlayerAction	actionQueue[ACTION_QUEUE_SIZE];
	UINT			actionQueueStart;
	UINT			actionQueueEnd;
};


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

	float GetCurrentHP() const { return m_currentHP; }
	float GetMaxHP() const { return m_maxHP; }

	void TakeDamage(int damage);
	void Heal(int amount);

	void InitHPBar(UIManager* uiManager);
	void SetHP(float hp);

private:

	void UpdateActionQueue(void);
	void HandlePlayerMove(Transform& transform);

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Player"; };

	Attributes m_attributes;
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

	bool m_hasWeapon = false;

	float m_currentHP = 100;    // 現在HP
	float m_maxHP = 100;        // 最大HP

	HPBarUI* m_hpBar = nullptr;

	float m_hpDrainTimer = 0.0f; // 自動HP減少タイマー
	bool m_autoHPDrain = true;   // 自動でHPを減らすかどうか（デバッグ用）

};