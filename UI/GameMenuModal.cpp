//=============================================================================
//
// [GameMenuModal.cpp]
// Author : 
// 
//=============================================================================
#include "UI/GameMenuModal.h"
#include "Core/GameSystem.h"

GameMenuModal::GameMenuModal(GameSystem* gameSystem) : m_gameSystem(gameSystem)
{
	m_elements.reserve(12);

	m_background = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(MENU_BACKGROUND_TEXTURE_PATH));
	m_background->SetSize(MENU_BACKGROUND_TEXTURE_SIZE_X, MENU_BACKGROUND_TEXTURE_SIZE_Y);
	m_background->SetScale(0.5f, 0.5f);

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
	m_btnResume->SetOnClick(OnResumeClicked, m_gameSystem);
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
	m_btnQuit->SetOnClick(OnExitClicked, m_gameSystem); // 終了ボタンにコールバックを設定
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
		m_btnTimeControl[i]->SetUseTweenScale(true); // Tweenスケールを有効にする
		m_elements.push_back(m_btnTimeControl[i]);
	}

	m_btnTimeControl[0]->SetOnClick(OnSceneSwitchClicked, m_gameSystem); // 最初のボタンにシーン切り替えのコールバックを設定


	OnAppear();
}

GameMenuModal::~GameMenuModal()
{
	SAFE_DELETE(m_btnResume);
	SAFE_DELETE(m_btnSave);
	SAFE_DELETE(m_btnQuit);

	SAFE_DELETE(m_background);
}

void GameMenuModal::OnAppear(void)
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


void GameMenuModal::Update(void)
{
	for (auto* elem : m_elements)
		elem->Update();
}

void GameMenuModal::Draw(void)
{
	// 背景を描画
	m_background->Draw();

	// ボタンを描画
	for (auto* elem : m_elements)
		elem->Draw();
}

void GameMenuModal::CollectTweenTargets(SimpleArray<ISpriteTransformable*>& outTargets) const
{
	outTargets.push_back(m_background);
	outTargets.push_back(m_btnResume);
	outTargets.push_back(m_btnSave);
	outTargets.push_back(m_btnQuit);
}

void GameMenuModal::ApplyBounceTween(ISpriteTransformable* target, const XMFLOAT2& targetPos, float offset)
{
	// 初期位置を計算（目標位置の上方）
	float startY = targetPos.y - offset;
	target->SetPosition({ targetPos.x, startY });

	// バウンドするアニメーションを追加
	m_TweenManager->AddMove(target, { targetPos.x, targetPos.y + 50 }, 0.3f, EaseType::OutCubic)
		->ThenMove({ targetPos.x, targetPos.y - 20 }, 0.2f, EaseType::InOutCubic)
		->ThenMove({ targetPos.x, targetPos.y + 10 }, 0.15f, EaseType::OutCubic)
		->ThenMove({ targetPos.x, targetPos.y }, 0.1f, EaseType::OutCubic);
}

void GameMenuModal::OnResumeClicked(void* userData)
{
	GameSystem* self = static_cast<GameSystem*>(userData);
	self->SetResumeBtnClick(true); // ゲームシステムにリジュームを通知
}

void GameMenuModal::OnExitClicked(void* userData)
{
	UIManager::get_instance().GetFader().StartFadeOut(); // 画面を黒くフェードイン

	DeferredTaskOptions options{};
	options.async = false; // 同期で実行

	RawDeferredTask::Add(
		nullptr,
		[](void* ptr)
		{
			ExitGame(); // ゲームを終了する
		},
		[](void* ptr)
		{
			return UIManager::get_instance().GetFader().IsFullyHidden(); // 完全に暗くなったら実行する♪
		},
		options
	);
}

void GameMenuModal::OnSceneSwitchClicked(void* userData)
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
				Ground& ground = Ground::get_instance();

				if (ground.GetCurrentScene() == SceneID::SCENE_1)
					ground.LoadScene(SceneID::SCENE_2);
				else
					ground.LoadScene(SceneID::SCENE_1);
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
