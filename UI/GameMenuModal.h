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
#include "Core/TextureMgr.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MENU_BACKGROUND_TEXTURE_PATH		"data/TEXTURE/UI/menu_bg.png"
#define MENU_BACKGROUND_TEXTURE_POS_X		SCREEN_CENTER_X
#define MENU_BACKGROUND_TEXTURE_POS_Y		SCREEN_CENTER_Y
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
#define MENU_BTN_TIME_CONTROL_TEXTURE_POS_Y			MENU_BACKGROUND_TEXTURE_POS_Y + MENU_BACKGROUND_TEXTURE_SIZE_Y * 0.13f
#define MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_X		73.0f
#define MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_Y		73.0f
#define MENU_BTN_TIME_CONTROL_NUM					8 // 時間制御ボタンの数
#define MENU_BTN_TIME_CONTROL_SPACING_X				38.0f // ボタン間のX方向の間隔

class GameMenuModal : public UIElement
{
public:
	GameMenuModal(GameSystem* gameSystem) : m_gameSystem(gameSystem)
	{
		m_background = new UISprite(
			UIManager::get_instance().GetSpriteVertexBuffer(), 
			TextureMgr::get_instance().CreateTexture(MENU_BACKGROUND_TEXTURE_PATH));
		m_background->SetSize(MENU_BACKGROUND_TEXTURE_SIZE_X, MENU_BACKGROUND_TEXTURE_SIZE_Y);
		m_background->SetScale(0.5f, 0.5f);
		m_background->SetDrawMode(SpriteDrawMode::Center);

		m_btnResume = new UIButton(
			{ MENU_BTN_RESUME_TEXTURE_POS_X, MENU_BTN_RESUME_TEXTURE_POS_Y }, 
			{ MENU_BTN_RESUME_TEXTURE_SIZE_X, MENU_BTN_RESUME_TEXTURE_SIZE_Y }, "Resume");
		m_btnResume->InitWithSingleTexture(
			UIManager::get_instance().GetSpriteVertexBuffer(),
			MENU_BTN_RESUME_TEXTURE_PATH,
			XMFLOAT4(1, 1, 1, 1), // Normal
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1), // Hover
			XMFLOAT4(0.6f, 0.6f, 0.6f, 1), // Pressed
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1) // Disabled
		);
		m_btnResume->SetScale(0.5f, 0.5f);
		m_elements.push_back(m_btnResume);

		m_btnSave = new UIButton(
			{ MENU_BTN_SAVE_TEXTURE_POS_X, MENU_BTN_SAVE_TEXTURE_POS_Y },
			{ MENU_BTN_SAVE_TEXTURE_SIZE_X, MENU_BTN_SAVE_TEXTURE_SIZE_Y }, "Save");
		m_btnSave->InitWithSingleTexture(
			UIManager::get_instance().GetSpriteVertexBuffer(),
			MENU_BTN_SAVE_TEXTURE_PATH,
			XMFLOAT4(1, 1, 1, 1), // Normal
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1), // Hover
			XMFLOAT4(0.6f, 0.6f, 0.6f, 1), // Pressed
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1) // Disabled
		);
		m_btnSave->SetScale(0.5f, 0.5f);
		m_elements.push_back(m_btnSave);

		m_btnQuit = new UIButton(
			{ MENU_BTN_QUIT_TEXTURE_POS_X, MENU_BTN_QUIT_TEXTURE_POS_Y },
			{ MENU_BTN_QUIT_TEXTURE_SIZE_X, MENU_BTN_QUIT_TEXTURE_SIZE_Y }, "Quit to Title");
		m_btnQuit->InitWithSingleTexture(
			UIManager::get_instance().GetSpriteVertexBuffer(),
			MENU_BTN_QUIT_TEXTURE_PATH,
			XMFLOAT4(1, 1, 1, 1), // Normal
			XMFLOAT4(0.8f, 0.8f, 0.8f, 1), // Hover
			XMFLOAT4(0.6f, 0.6f, 0.6f, 1), // Pressed
			XMFLOAT4(0.5f, 0.5f, 0.5f, 1) // Disabled
		);
		m_btnQuit->SetScale(0.5f, 0.5f);
		m_elements.push_back(m_btnQuit);

		for (int i = 0; i < MENU_BTN_TIME_CONTROL_NUM; ++i)
		{
			m_btnTimeControl[i] = new UIButton(
				{ MENU_BTN_TIME_CONTROL_TEXTURE_POS_X + i * (MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_X + MENU_BTN_TIME_CONTROL_SPACING_X),
				  MENU_BTN_TIME_CONTROL_TEXTURE_POS_Y },
				{ MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_X, MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_Y }, "TimeControl");
			m_btnTimeControl[i]->InitWithSingleTexture(
				UIManager::get_instance().GetSpriteVertexBuffer(),
				MENU_BTN_TIME_CONTROL_TEXTURE_PATH,
				XMFLOAT4(1, 1, 1, 1), // Normal
				XMFLOAT4(0.8f, 0.8f, 0.8f, 1), // Hover
				XMFLOAT4(0.6f, 0.6f, 0.6f, 1), // Pressed
				XMFLOAT4(0.5f, 0.5f, 0.5f, 1) // Disabled
			);
			m_btnTimeControl[i]->SetScale(0.5f, 0.5f);
			m_elements.push_back(m_btnTimeControl[i]);
		}

		m_btnTimeControl[0]->SetOnClick(OnSceneSwitchClicked, m_gameSystem); // 最初のボタンにシーン切り替えのコールバックを設定


		OnAppear();
	}

	void OnAppear(void)
	{
		// メニュー背景
		ApplyBounceTween(m_background, { MENU_BACKGROUND_TEXTURE_POS_X, MENU_BACKGROUND_TEXTURE_POS_Y });

		// リジュームボタン
		ApplyBounceTween(m_btnResume, { MENU_BTN_RESUME_TEXTURE_POS_X, MENU_BTN_RESUME_TEXTURE_POS_Y });

		// セーブボタン
		ApplyBounceTween(m_btnSave, { MENU_BTN_SAVE_TEXTURE_POS_X, MENU_BTN_SAVE_TEXTURE_POS_Y });

		// 終了ボタン
		ApplyBounceTween(m_btnQuit, { MENU_BTN_QUIT_TEXTURE_POS_X, MENU_BTN_QUIT_TEXTURE_POS_Y });

		// 時間制御ボタン
		for (int i = 0; i < MENU_BTN_TIME_CONTROL_NUM; ++i)
		{
			float posX = MENU_BTN_TIME_CONTROL_TEXTURE_POS_X + i * (MENU_BTN_TIME_CONTROL_TEXTURE_SIZE_X + MENU_BTN_TIME_CONTROL_SPACING_X);
			ApplyBounceTween(m_btnTimeControl[i], { posX, MENU_BTN_TIME_CONTROL_TEXTURE_POS_Y });
		}
	}

	~GameMenuModal()
	{
		SAFE_DELETE(m_btnResume);
		SAFE_DELETE(m_btnSave);
		SAFE_DELETE(m_btnQuit);

		SAFE_DELETE(m_background);
	}

	void Update(void) override
	{
		for (auto* elem : m_elements)
			elem->Update();
	}

	void Draw(void) override
	{
		// 背景を描画
		m_background->Draw();

		// ボタンを描画
		for (auto* elem : m_elements)
			elem->Draw();
	}

	void GameMenuModal::CollectTweenTargets(SimpleArray<ISpriteTransformable*>& outTargets) const override
	{
		outTargets.push_back(m_background);
		outTargets.push_back(m_btnResume);
		outTargets.push_back(m_btnSave);
		outTargets.push_back(m_btnQuit);
	}

private:
	void ApplyBounceTween(ISpriteTransformable* target, const XMFLOAT2& targetPos, float offset = 300.0f)
	{
		// 初期位置を計算（目標位置の上方）
		float startY = targetPos.y - offset;
		target->SetPosition({ targetPos.x, startY });

		// バウンドするアニメーションを追加
		m_TweenManager->AddMove(target, { targetPos.x, targetPos.y + 30 }, 0.3f, EaseType::OutCubic)
			->ThenMove({ targetPos.x, targetPos.y - 10 }, 0.2f, EaseType::InOutCubic)
			->ThenMove({ targetPos.x, targetPos.y }, 0.15f, EaseType::OutCubic);
	}

	static void OnSceneSwitchClicked(void* userData)
	{
		GameSystem* self = static_cast<GameSystem*>(userData);

		auto& fader = UIManager::get_instance().GetFader();

		if (!fader.IsFading())
		{
			struct ReturnToTitleContext
			{
				GameSystem* gameSystem;
			};

			fader.StartFadeOut(); // 画面を黒くフェードイン

			DeferredTaskOptions options{};
			options.async = false; // 同期で実行
			options.debugName = "Switch_Scene"; // デバッグ用の名前
			auto* ctx = new ReturnToTitleContext{ self }; // コンテキストを作成して渡す

			RawDeferredTask::Add(
				ctx, // コンテキストを渡す
				[](void* ptr)
				{
					GameSystem* self = static_cast<ReturnToTitleContext*>(ptr)->gameSystem;
					if (self->GetGround()->GetCurrentScene() == SceneID::SCENE_1)
						self->GetGround()->LoadScene(SceneID::SCENE_2);
					else
						self->GetGround()->LoadScene(SceneID::SCENE_1);
					UIManager::get_instance().GetFader().StartFadeIn(); // タイトル画面をフェードアウト表示
				},
				[](void* ptr)
				{
					return UIManager::get_instance().GetFader().IsFullyHidden(); // 完全に暗くなったら実行する♪
				},
				options
			);
		}
	}

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