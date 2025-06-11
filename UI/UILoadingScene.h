#pragma once
//=============================================================================
//
// 読み込みシーン用 UI 管理クラス [UILoadingScene.h]
// Author : 
// これはゲーム中の「ローディング画面」を管理する UIElement の子クラスだよ～っ
// 黒背景フェード + アニメーション + プログレスバーで、
// 見た目も快適さも可愛さも全部守ってくれる守護精霊ちゃんなのっ
//
// 主な流れ：
//   ① 最初に黒背景でフェードイン（外部で済んでたらスキップ可能）
//   ② 一定時間内に読み込みが完了すれば UI を出さず即終了（かしこいっ！）
//   ③ 長くかかる時は、かわいいアニメーションと進行バーを表示
//   ④ 全部終わったら黒背景でフェードインしてゲーム画面へ～
// 
// 特徴：
//  - アニメーション再生には UIAnimatedSprite を使用
//  - 読み込み完了は外部から NotifyResourceLoadComplete で通知
//  - 進行度は SetProgress で外部からリアルタイム更新可能っ
//  - 自動的に内部ステートを切り替えて最適な見せ方にしてくれるよ
//
//=============================================================================
#include "UI/Base/UIManager.h"
#include "UI/Base/UIAnimatedSprite.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define LOADING_ANIM_TEXTURE_PATH         "data/TEXTURE/loading 10.png"
#define LOADING_ANIM_THRESHOLD            0.5f

 // 内部サブ状態マネージャー
enum class LoadingState
{
    FadeInBlack,          // 最初に黒くフェードイン（外部でやってるなら skip）
    WaitForResources,     // リソース読み込み待ち（短時間ならUI 表示しない）
    FadeOutBlackToShowUI, // 黒からフェードアウトしてアニメUI表示へ
    ShowLoadingUI,        // 読み込みアニメUIを表示中
    FadeInBeforeFinish,   // 読み込み完了後、再度黒にフェードイン
    Done                  // 完了！
};

class UILoadingScene : public UIElement
{
public:
    UILoadingScene(void) {}
    ~UILoadingScene(void) { SAFE_DELETE(m_loadingAnim); }

    // 初期化（アニメーションテクスチャとVB）
    // すでにフェードイン済みなら skipFadeIn を true にする
    void Init(ID3D11ShaderResourceView* tex, bool skipFadeIn = false);
    void Update(void) override;
    void Draw(void) override;

    bool IsFinished(void) const { return m_state == LoadingState::Done; }

    // GameSystem から「読み込み完了」通知
    void NotifyResourceLoadComplete(void) { m_resourceLoaded = true; }

    // 進行度を外部から設定（0.0 ～ 1.0）
    void SetProgress(float progress) { m_loadProgress = progress; }

private:
    // プログレスバーを中央下に表示
    void DrawProgressBar(void);

    float m_elapsedTime = 0.0f;
    bool m_showLoadingUI = false;
    bool m_resourceLoaded = false; // 外部から通知を受けたかどうか
    bool m_isAutoPlay = false;
    LoadingState m_state = LoadingState::FadeInBlack;
    const float m_threshold = LOADING_ANIM_THRESHOLD; // これ以上でアニメUI表示

    // プログレス表示用
    float m_loadProgress = 0.0f; // 現在の読み込み進行 (0.0f～1.0f)
    float m_animProgress = 0.0f; // アニメの内部進行（0～1）

    // アニメーション用スプライト
    UIAnimatedSprite* m_loadingAnim = nullptr;

    Timer& m_timer = Timer::get_instance();
    UIManager& m_UIManager = UIManager::get_instance();
    ScreenFader& m_screenFader = m_UIManager.GetFader();
};