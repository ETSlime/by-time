#pragma once
#include "UI/Base/UIManager.h"
#include "UI/Base/UISprite.h"
#include "Core/TextureMgr.h"
#include "Utility/InputManager.h"

#define GAMECLEAR_TEXTURE_PATH       "data/TEXTURE/gameclear.jpg" // 1枚テクスチャ

class UIGameOverScene : public UIElement
{
public:
    UIGameOverScene() {}

    void Init(GameSystem* system)
    {
        m_fullSprite = new UISprite();
        m_fullSprite->Init(TextureMgr::get_instance().CreateTexture(GAMECLEAR_TEXTURE_PATH),
            UIManager::get_instance().GetSpriteVertexBuffer());
        m_fullSprite->SetPosition({ float(SCREEN_CENTER_X), float(SCREEN_CENTER_Y) });
        m_fullSprite->SetSize({ float(SCREEN_WIDTH), float(SCREEN_HEIGHT) });

        m_clickLocked = false;
        m_fadeOutStarted = false;

        UIManager::get_instance().GetFader().StartFadeIn(1.0f);

        m_inputManager = &InputManager::get_instance();
    }

    ~UIGameOverScene()
    {
        SAFE_DELETE(m_fullSprite);
    }

    void Update() override
    {
        if (m_fullSprite) m_fullSprite->Update();

        if (!m_clickLocked && !m_fadeOutStarted)
        {
            if (m_inputManager->IsMouseLeftClicked())
            {
                StartFadeOut();
            }
        }

        if (m_fadeOutStarted)
        {
            if (UIManager::get_instance().GetFader().IsFadeOutComplete())
            {
                GameSystem::get_instance().ChangeMode(GameMode::TITLE, true);
                m_clickLocked = false;
            }
        }
    }

    void Draw() override
    {
        if (m_fullSprite) m_fullSprite->Draw();
    }

private:
    void StartFadeOut()
    {
        m_fadeOutStarted = true;
        m_clickLocked = true;
        UIManager::get_instance().GetFader().StartFadeOut(1.0f);
    }

    UISprite* m_fullSprite = nullptr;

    bool m_clickLocked = false;
    bool m_fadeOutStarted = false;

    InputManager* m_inputManager = nullptr;
    Timer& m_timer = Timer::get_instance();
    UIManager& m_UIManager = UIManager::get_instance();
};
