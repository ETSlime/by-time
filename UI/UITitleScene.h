#pragma once
//=============================================================================
//
//  [UITitleScene.h]
// Author : 
//
//=============================================================================
#include "UI/Base/UIManager.h"
#include "UI/Base/UIAnimatedSprite.h"
#include "Core/TextureMgr.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define TITLE_ANIM_TEXTURE_PATH         "data/TEXTURE/bg000.jpg"
#define TITLE_ANIM_TILE_X               1
#define TITLE_ANIM_TILE_Y               1
#define TITLE_ANIM_SPEED                0.1f

class UITitleScene : public UIElement
{
public:

    UITitleScene()
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
    }

    ~UITitleScene()
    {
        SAFE_DELETE(m_titleAnim);
    }

    void Update(void) override
    {
        if (m_titleAnim)
            m_titleAnim->Update();

        if (!m_fadeOutStarted)
        {
            // 決定キー押下でフェードアウト開始＆ローディングへ切り替え
            if (m_inputManager.GetKeyboardTrigger(KEY_CONFIRM))
            {
                m_fadeOutStarted = true;
            }
        }
        else
        {
            m_fadeAlpha += m_timer.GetDeltaTime() * 1.5f;
            if (m_fadeAlpha >= 1.0f)
            {
                m_fadeAlpha = 1.0f;
                // 完全にフェードアウトしたらローディングモードに切り替え
                GameSystem::get_instance().ChangeMode(GameMode::LOADING);
            }
        }
    }

    void Draw() override
    {
        if (m_titleAnim)
            m_titleAnim->Draw();

        // 黒のフルスクリーンオーバーレイを描画（透明度は m_fadeAlpha）
        m_UIManager.DrawOverlay(
            static_cast<float>(SCREEN_CENTER_X), 
            static_cast<float>(SCREEN_CENTER_Y),
            static_cast<float>(SCREEN_WIDTH),
            static_cast<float>(SCREEN_HEIGHT),
            { 0, 0, 0, m_fadeAlpha } // このα値でフェードイン・フェードアウトするよ！
        );
    }

private:
    UIAnimatedSprite* m_titleAnim = nullptr; // タイトルのアニメ
    bool m_fadeOutStarted = false;            // フェードアウト開始フラグ
    float m_fadeAlpha = 0.0f;                // フェード用透明度

    Timer& m_timer = Timer::get_instance();
    InputManager& m_inputManager = InputManager::get_instance();
    UIManager& m_UIManager = UIManager::get_instance();
};
