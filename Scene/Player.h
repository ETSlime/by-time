#pragma once
//=============================================================================
//
// プレイヤー操作＆主役ちゃん管理クラス [Player.h]
// Author : 
// プレイヤーの行動・移動・描画・エフェクトを一括で担当する主人公ちゃん管理クラスですっ！
// SigewinneやKleeたちの切り替えもここで制御して、アクションキューで動きを可愛く整列します
// 
//=============================================================================
#include "Scene/Character/Lumine.h"
#include "Scene/Item/ItemManager.h"
#include "Core/LightManager.h"
#include "Utility/InputManager.h"
#include "Effects/ProjectilePreview/ProjectilePreviewRenderer.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	VALUE_PLAYER_MOVE			(6.5f)							// 移動量
#define VALUE_PLAYER_CLIMB			(4.0f)							// クライム移動量
#define VALUE_PLAYEER_RUN_FACTOR	(1.5f)
#define	VALUE_PLAYER_ROTATE			(XM_PI * 0.02f)					// 回転量

#define PLAYER_INIT_POS				XMFLOAT3(4024.0f, 6270.0f, 4453.0f)

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

	int GetTotalModelLoadCnt(void) const { return m_totalModelLoadCnt; }
	int GetCurrentModelLoadCnt(void) const { return m_currentModelLoadCnt; }

	void PickItem(Item* obj);

	void SetClimbMode(PlayerState climbState);
	void PlaceItem(ItemType type);

	static void OnClimbPromptTriggered(void* userData);

	static void OnPlaceItemPromptTriggered(void* userData);

	bool GetMeleeMode(void) const { return m_playerAttr.isMeleeMode; }
	bool HasSeed(void) const { return m_playerAttr.seedCount > 0; }
	PlayerAttributes& GetPlayerAttributes(void) { return m_playerAttr; }
	const BOUNDING_BOX& GetBoundingBox(void) const { return m_playerGO->GetCollider().aabb; }

private:
	void UpdateNormalMode(void);
	void UpdateClimbMode(void);
	void UpdateActionQueue(void);
	void UpdatePlayerStatus(void);
	void UpdatePlayerMove(void);
	void UpdatePlayerAttack(void);
	void UpdateSwitchCharacter(void);
	void HandleInput(void);
	void HandlePlayerMove(Transform& transform);
	void AlignPlayerYawToCamera(void); // プレイヤーのYawをカメラのYawに補間して合わせる関数
	void ConfirmThrow(void);
	void CancelThrow(void);

	virtual void RenderImGui(void) override;
	virtual const char* GetPanelName(void) const override { return "Player"; };

	Attributes* m_attributes;
	Transform m_transform;
	PlayerAttributes m_playerAttr;

	Lumine* lumine;


	bool m_drawBoundingBox = true;
	int m_currentModelLoadCnt = 0;
	int m_totalModelLoadCnt = 1;
	bool m_hasWeapon = false;

	GameObject<SkinnedMeshModelInstance>* m_playerGO;

	Light* m_light;

	ProjectilePreviewRenderer* m_previewRenderer = nullptr;
	ProjectilePredictor m_predictor;

	Renderer& m_renderer = Renderer::get_instance();
	LightManager& m_lightMgr = LightManager::get_instance();
	Camera& m_camera = Camera::get_instance();
	DebugProc& m_debugProc = DebugProc::get_instance();
	InputManager& m_inputManager = InputManager::get_instance();
	ItemManager& m_itemManager = ItemManager::get_instance();
};