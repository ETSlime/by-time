//=============================================================================
//
//  [UILoadingScene.cpp]
// Author : 
//
//=============================================================================
#include "UI/UILoadingScene.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define LOADING_ANIM_TILE_X       3
#define LOADING_ANIM_TILE_Y       4
#define LOADING_ANIM_SPD          0.1f
#define LOADING_SMOOTH_SPD        1.0f
#define LOADING_FAST_SPD          4.0f

void UILoadingScene::Init(ID3D11ShaderResourceView* tex, bool skipFadeIn)
{
    m_state = skipFadeIn ? LoadingState::WaitForResources : LoadingState::FadeInBlack;

    // ローディングアニメーション用スプライト
    m_loadingAnim = new UIAnimatedSprite();
    m_loadingAnim->Init(tex, 
        m_UIManager.GetSpriteVertexBuffer(), 
        LOADING_ANIM_TILE_X, 
        LOADING_ANIM_TILE_Y, 
        LOADING_ANIM_SPD);

    // 表示位置とサイズを設定
    m_loadingAnim->SetPosition({ static_cast<float>(SCREEN_CENTER_X), static_cast<float>(SCREEN_CENTER_Y) });
    m_loadingAnim->SetSize({ static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT) });
    m_loadingAnim->Play();

    if (!skipFadeIn)
        m_screenFader.StartFadeIn();
}

void UILoadingScene::Update(void)
{
    m_elapsedTime += m_timer.GetDeltaTime();

    // 読み込み完了前までは進行度に合わせて滑らかにアニメバー更新
    //if (!m_resourceLoaded)
    //{
        m_animProgress += (m_loadProgress - m_animProgress) * m_timer.GetDeltaTime() * LOADING_SMOOTH_SPD;
    //}
    //else
    //{
    //    // 読み込み完了後は一気にゴールへ進める
    //    m_animProgress += (1.0f - m_animProgress) * m_timer.GetDeltaTime() * LOADING_FAST_SPD;
    //}

    // アニメーション更新
    if (m_loadingAnim)
    {
        //if (!m_resourceLoaded) 
        //{
        //    // 読み込み未完了 → プログレスに応じて手動でフレーム指定（進行度バーと連動）
        //    m_loadingAnim->SetFrameDirectly(m_animProgress);
        //}
        //else 
        //{
        //    // 読み込み完了後 → 通常再生でアニメーションを進行（速度制御も可能）
            m_loadingAnim->Update();
        //}
    }

    m_screenFader.Update();

    // 状態遷移の処理
    switch (m_state)
    {
        // 最初のフェードイン（黒画面へ）
    case LoadingState::FadeInBlack:
        if (m_screenFader.IsFullyHidden())
            m_state = LoadingState::WaitForResources;
        break;

        // リソース読み込み待ち（軽いならアニメなし）
    case LoadingState::WaitForResources:
        if (m_elapsedTime >= m_threshold && !m_showLoadingUI)
        {
            // 一定時間以上かかっているのでアニメを表示する
            m_showLoadingUI = true;
            m_screenFader.StartFadeIn();
            m_state = LoadingState::FadeOutBlackToShowUI;
        }
        if (m_resourceLoaded && !m_showLoadingUI)
        {
            // 読み込み完了、アニメ不要ならそのままゲームへ
            m_screenFader.StartFadeOut();
            m_state = LoadingState::Done;
        }
        break;

        // 黒フェードアウト → アニメ表示
    case LoadingState::FadeOutBlackToShowUI:
        if (m_screenFader.IsFullyVisible())
            m_state = LoadingState::ShowLoadingUI;
        break;

        // アニメ表示中（読み込み完了を待つ）
    case LoadingState::ShowLoadingUI:
        if (m_resourceLoaded)
        {
            m_screenFader.StartFadeOut();
            m_state = LoadingState::FadeInBeforeFinish;
        }
        break;

        // フェードイン（アニメ終了→画面を黒く戻す）
    case LoadingState::FadeInBeforeFinish:
        if (m_screenFader.IsFullyHidden())
            m_state = LoadingState::Done;
        break;

    default:
        break;
    }
}

void UILoadingScene::Draw(void)
{
    if (m_state != LoadingState::Done)
    {
        if (m_loadingAnim)
            m_loadingAnim->Draw();

        DrawProgressBar();
    }
}

void UILoadingScene::DrawProgressBar(void)
{
    float barWidth = 400.0f * m_animProgress;
    float barHeight = 16.0f;
    float x = SCREEN_WIDTH / 2.0f - 200.0f;
    float y = SCREEN_HEIGHT / 2.0f + 80.0f;

    UIManager::get_instance().DrawOverlay(
        x, y, barWidth, barHeight,
        { 1.0f, 1.0f, 1.0f, 1.0f }
    );
}