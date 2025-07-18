#pragma once
//=============================================================================
//
//  [UITitleScene.h]
// Author : 
//
//=============================================================================
#include "UI/Base/UIManager.h"
#include "UI/Base/UIAnimatedSprite.h"
#include "UI/Base/UIButton.h"
#include "Core/TextureMgr.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define TITLE_ANIM_TEXTURE_PATH         "data/TEXTURE/bg000.png"
#define TITLE_NEWGAME_TEXTURE_PATH      "data/TEXTURE/UI/newgame_xlixk.png"
#define TITLE_BTN_TEXTURE_SIZE_X        380
#define TITLE_BTN_TEXTURE_SIZE_Y        80
#define TITLE_ANIM_TILE_X               1
#define TITLE_ANIM_TILE_Y               1
#define TITLE_ANIM_SPEED                0.1f

class UITitleScene : public UIElement
{
public:
    UITitleScene() {}

	void Init(GameSystem* system)
	{
        // タイトルアニメの生成
        m_titleAnim = new UIAnimatedSprite();

        // テクスチャと情報を設定（例：6列×1行のタイトルアニメ）
        m_titleAnim->Init(
            TextureMgr::get_instance().CreateTexture(TITLE_ANIM_TEXTURE_PATH),
            UIManager::get_instance().GetSpriteVertexBuffer(),
            TITLE_ANIM_TILE_X, TITLE_ANIM_TILE_Y,
            TITLE_ANIM_SPEED, true // TITLE_ANIM_SPEED秒ごと、ループ再生
        );

        // 表示位置とサイズを設定
        m_titleAnim->SetPosition({ static_cast<float>(SCREEN_CENTER_X), static_cast<float>(SCREEN_CENTER_Y) });
        m_titleAnim->SetSize({ static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) });

        m_btnNewGame = new UIButton();
        m_btnNewGame->InitWithSingleTexture(UIManager::get_instance().GetSpriteVertexBuffer(), TITLE_NEWGAME_TEXTURE_PATH);
        m_btnNewGame->SetPosition({ static_cast<float>(SCREEN_CENTER_X), static_cast<float>(SCREEN_CENTER_Y) });
        m_btnNewGame->SetSize({ static_cast<float>(TITLE_BTN_TEXTURE_SIZE_X),  static_cast<float>(TITLE_BTN_TEXTURE_SIZE_Y) });
        m_btnNewGame->SetOnClick(OnClick_NewGame, this);


        // 最初はフェードインから始める（必要であれば）
        m_UIManager.GetFader().StartFadeIn(1.0f); // 1秒でフェードイン
	}

    ~UITitleScene()
    {
        SAFE_DELETE(m_titleAnim);
        SAFE_DELETE(m_btnNewGame);

    }

    void Update(void) override
    {
        if (m_titleAnim)
            m_titleAnim->Update();

        if (m_btnNewGame)
            m_btnNewGame->Update();

        auto& fader = UIManager::get_instance().GetFader();

        // フェードアウトがまだ始まってない場合、決定入力を待つ
        if (m_fadeOutStarted)
        {
            // フェードアウトが完了したらゲームモードを変更
            if (fader.IsFadeOutComplete())
            {
                GameSystem::get_instance().ChangeMode(GameMode::LOADING, true);
                UnlockClick(); // ← シーン切り替え完了時にロック解除
            }
        }
    }

    void Draw(void) override
    {
        if (m_titleAnim)
            m_titleAnim->Draw();

        // ScreenFader の描画は UIManager 側で一括管理されるのでここでは不要

		if (m_btnNewGame)
			m_btnNewGame->Draw();
    }

    void RequestStartFadeOut(void)
    {
        auto& fader = UIManager::get_instance().GetFader();
        m_fadeOutStarted = true;
        fader.StartFadeOut(1.0f); // 1秒でフェードアウト開始
    }

    static void OnClick_NewGame(void* userData)
    {
        UITitleScene* scene = static_cast<UITitleScene*>(userData);

        if (scene->IsClickLocked()) return; // 二重クリック防止
        scene->RequestStartFadeOut();
        scene->LockClick(); // ロック！
    }

	// クリックロックの管理
    void LockClick(void) { m_clickLocked = true; }
    void UnlockClick(void) { m_clickLocked = false; }
    bool IsClickLocked(void) const { return m_clickLocked; }

private:

    UIAnimatedSprite* m_titleAnim = nullptr; // タイトルのアニメ
    bool m_clickLocked = false; // クリックロック（多重起動防止）
    bool m_fadeOutStarted = false; // フェードアウト開始フラグ
	UIButton* m_btnNewGame = nullptr; // 新規ゲームボタン
	UIButton* m_btnLoadGame = nullptr; // ロードゲームボタン
	UIButton* m_btnExitGame = nullptr; // ゲーム終了ボタン

    Timer& m_timer = Timer::get_instance();
    InputManager& m_inputManager = InputManager::get_instance();
    UIManager& m_UIManager = UIManager::get_instance();
};
