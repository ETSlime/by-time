#pragma once
//=============================================================================
//
// [GameMenuModal.h]
// Author : 
// 
//=============================================================================
#include "UI/Base/UIElement.h"
#include "UI/Base/UIButton.h"
#include "UI/Base/UIManager.h"
#include "UI/Base/TweenManager.h"
#include "Scene/Ground.h"
#include "Core/TextureMgr.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MENU_BACKGROUND_TEXTURE_PATH		"data/TEXTURE/UI/menu_bg.png"
#define MENU_BACKGROUND_TEXTURE_POS_X		SCREEN_CENTER_X
#define MENU_BACKGROUND_TEXTURE_POS_Y		SCREEN_CENTER_Y * 0.9f // 画面中央の少し上に配置
#define MENU_BACKGROUND_TEXTURE_SIZE_X		1937.0f
#define MENU_BACKGROUND_TEXTURE_SIZE_Y		1150.0f

#define MENU_BTN_RESUME_TEXTURE_PATH		"data/TEXTURE/UI/back.png"
#define MENU_BTN_RESUME_TEXTURE_POS_X		MENU_BACKGROUND_TEXTURE_POS_X - MENU_BACKGROUND_TEXTURE_SIZE_X * 0.25f
#define MENU_BTN_RESUME_TEXTURE_POS_Y		MENU_BACKGROUND_TEXTURE_POS_Y - MENU_BACKGROUND_TEXTURE_SIZE_Y * 0.25f
#define MENU_BTN_RESUME_TEXTURE_SIZE_X		413.0f
#define MENU_BTN_RESUME_TEXTURE_SIZE_Y		172.0f

#define MENU_BTN_SAVE_TEXTURE_PATH			"data/TEXTURE/UI/save.png"
#define MENU_BTN_SAVE_TEXTURE_POS_X			MENU_BACKGROUND_TEXTURE_POS_X + MENU_BACKGROUND_TEXTURE_SIZE_X * 0.25f
#define MENU_BTN_SAVE_TEXTURE_POS_Y			MENU_BACKGROUND_TEXTURE_POS_Y + MENU_BACKGROUND_TEXTURE_SIZE_Y * 0.2f
#define MENU_BTN_SAVE_TEXTURE_SIZE_X		581.0f
#define MENU_BTN_SAVE_TEXTURE_SIZE_Y		175.0f

#define MENU_BTN_QUIT_TEXTURE_PATH			"data/TEXTURE/UI/quitgame.png"
#define MENU_BTN_QUIT_TEXTURE_POS_X			MENU_BACKGROUND_TEXTURE_POS_X + MENU_BACKGROUND_TEXTURE_SIZE_X * 0.25f
#define MENU_BTN_QUIT_TEXTURE_POS_Y			MENU_BACKGROUND_TEXTURE_POS_Y + MENU_BACKGROUND_TEXTURE_SIZE_Y * 0.3f
#define MENU_BTN_QUIT_TEXTURE_SIZE_X		580.0f
#define MENU_BTN_QUIT_TEXTURE_SIZE_Y		174.0f

#define MENU_BTN_TIME_CONTROL_TEXTURE_PATH			"data/TEXTURE/UI/833743121e34e7c0.png"
#define MENU_BTN_TIME_CONTROL_TEXTURE_POS_X			MENU_BACKGROUND_TEXTURE_POS_X - MENU_BACKGROUND_TEXTURE_SIZE_X * 0.205f
#define MENU_BTN_TIME_CONTROL_TEXTURE_POS_Y			MENU_BACKGROUND_TEXTURE_POS_Y + MENU_BACKGROUND_TEXTURE_SIZE_Y * 0.126f
#define MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_X		73.0f
#define MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_Y		73.0f
#define MENU_BTN_TIME_CONTROL_NUM					8 // 時間制御ボタンの数
#define MENU_BTN_TIME_CONTROL_SPACING_X				39.5f // ボタン間のX方向の間隔

class GameSystem;

class GameMenuModal : public UIElement
{
public:
	GameMenuModal(GameSystem* gameSystem);
	~GameMenuModal();

	void OnAppear(void);

	void Update(void) override;
	void Draw(void) override;

	void CollectTweenTargets(SimpleArray<ISpriteTransformable*>& outTargets) const override;

private:
	// ボタンのクリックイベントハンドラー
	static void OnResumeClicked(void* userData);
	static void OnExitClicked(void* userData);
	static void OnSceneSwitchClicked(void* userData);

	void ApplyBounceTween(ISpriteTransformable* target, const XMFLOAT2& targetPos, float offset = 300.0f);

	// クリックロックの管理
	void LockClick(void) { m_clickLocked = true; }
	void UnlockClick(void) { m_clickLocked = false; }
	bool IsClickLocked(void) const { return m_clickLocked; }

	bool m_clickLocked = false; // クリックロック（多重起動防止）

	GameSystem* m_gameSystem = nullptr;
    UIButton* m_btnResume = nullptr;
    UIButton* m_btnSave = nullptr;
    UIButton* m_btnQuit = nullptr;
	UIButton* m_btnTimeControl[MENU_BTN_TIME_CONTROL_NUM] = { nullptr }; // 時間制御ボタンの配列

	UISprite* m_background = nullptr; // メニュー背景スプライト

    SimpleArray<UIElement*> m_elements;

	TweenManager* m_TweenManager = &TweenManager::get_instance();
};