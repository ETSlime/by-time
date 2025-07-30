#include "UI/UIResultScene.h"
#include "Core/GameSystem.h"
#include "UI/Base/UIManager.h"

UIResultScene::UIResultScene(ResultType type)
    : m_resultType(type)
{
}

void UIResultScene::Init()
{
    const char* path = (m_resultType == ResultType::CLEAR)
        ? "data/TEXTURE/UI/gemeclear.jpg"
        : "data/TEXTURE/UI/gameover.png";

    m_resultSprite = new UIAnimatedSprite();
    m_resultSprite->Init(
        TextureMgr::get_instance().CreateTexture(path),
        UIManager::get_instance().GetSpriteVertexBuffer(),
        1, 1, 0.0f, false
    );

    m_resultSprite->SetPosition({ static_cast<float>(SCREEN_CENTER_X), static_cast<float>(SCREEN_CENTER_Y) });
    m_resultSprite->SetSize({ static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) });

    UIManager::get_instance().GetFader().StartFadeIn(1.0f);
}

UIResultScene::~UIResultScene()
{
    SAFE_DELETE(m_resultSprite);
}

void UIResultScene::Update()
{
    if (m_resultSprite)
        m_resultSprite->Update();

    auto& fader = UIManager::get_instance().GetFader();

    if (!m_fadeOutStarted && m_input.IsMouseLeftTriggered())
    {
        m_fadeOutStarted = true;
        fader.StartFadeOut(1.0f);
    }

    if (m_fadeOutStarted && fader.IsFadeOutComplete())
    {
        GameSystem::get_instance().ChangeMode(GameMode::TITLE, true);
    }
}

void UIResultScene::Draw()
{
    if (m_resultSprite)
        m_resultSprite->Draw();
}
