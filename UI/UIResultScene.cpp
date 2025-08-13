#include "UI/UIResultScene.h"
#include "Core/GameSystem.h"


UIResultScene::UIResultScene(ResultType type)
    : m_resultType(type)
{
}

void UIResultScene::Init()
{
#ifdef _DEBUG
    m_debugProc.PrintDebugProc("UIResultScene::Init() called\n");
#endif

    const char* path = (m_resultType == ResultType::GAMEOVER)
        ? "data/TEXTURE/gameover.png"
        : "data/TEXTURE/gameclear.jpg";

    m_resultSprite = new UIAnimatedSprite();
    m_resultSprite->Init(
        TextureMgr::get_instance().CreateTexture(path),
        UIManager::get_instance().GetSpriteVertexBuffer(),
        1, 1, 0.0f, false
    );

    m_resultSprite->SetPosition({ static_cast<float>(SCREEN_CENTER_X), static_cast<float>(SCREEN_CENTER_Y) });
    m_resultSprite->SetSize({ static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) });

    m_resultSprite->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    m_resultSprite->SetZOrder(0);

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
    fader.Update();

    auto& input = InputManager::get_instance();

    if (!m_fadeOutStarted && !fader.IsFading() && input.IsMouseLeftTriggered())
    {
        m_fadeOutStarted = true;
        fader.StartFadeOut(1.0f);
    }

    // ここでは ChangeMode は呼ばない
}


void UIResultScene::Draw()
{
#ifdef _DEBUG
    m_debugProc.PrintDebugProc("UIResultScene::Draw() called\n");
#endif

    auto& renderer = Renderer::get_instance();

    // UI描画に必要な設定を確実にする
    renderer.SetDepthMode(DepthMode::Disable);    // 深度テスト無効
    renderer.SetBlendState(BLEND_MODE_ALPHABLEND); // ブレンド有効
    renderer.SetUIInputLayout();                   // UI用の入力レイアウトを設定
    renderer.SetRenderUI();                         // UI描画用パイプライン設定

    if (m_resultSprite)
        m_resultSprite->Draw();
}