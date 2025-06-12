//=============================================================================
// SeekBarUI.cpp
// 動画再生バーUIクラス（SeekBarUI）
//=============================================================================
#include "SeekBarUI.h"
#include "UI/Base/UIManager.h"
#include "Utility/InputManager.h"

//-----------------------------------------------------------------------------
// コンストラクタ - 初期サイズや色、位置の設定
//-----------------------------------------------------------------------------
SeekBarUI::SeekBarUI()
{
    SetSize({ 1000.0f, 40.0f });                // 横1000ピクセル・縦40ピクセルのバー
    SetColor({ 0.2f, 0.6f, 1.0f, 1.0f });      // 青っぽい色で進捗表示
    m_progress = 0.0f;                         // 最初は0％
    m_position = { 0.0f, 0.0f };               // 後で UpdatePosition で決定
}

//-----------------------------------------------------------------------------
// フレームごとの更新処理（今回は進捗を自動で加算）
//-----------------------------------------------------------------------------
void SeekBarUI::Update()
{
    // デモ用に自動で進捗を進めて、1.0を超えたらループ
    m_progress += 0.001f;
    if (m_progress > 1.0f)
        m_progress = 0.0f;

    //  // マウス左クリックの瞬間を検出
    //if (InputManager::get_instance().IsMouseLeftTriggered())
    //{
    //    long mouseX = InputManager::get_instance().GetMouseX();
    //    long mouseY = InputManager::get_instance().GetMouseY();
    //
    //    OnMouseClick(static_cast<float>(mouseX), static_cast<float>(mouseY));
    //}
}

//-----------------------------------------------------------------------------
// 描画処理（背景バーと進捗バーを重ねて描画）
//-----------------------------------------------------------------------------
void SeekBarUI::Draw()
{
    // 背景バーをグレーで描画
    UIManager::get_instance().DrawOverlay(
        m_position.x, m_position.y,
        m_size.x, m_size.y,
        { 0.3f, 0.3f, 0.3f, 1.0f });

    // 進捗バー（左から progress に応じて横幅を縮めて描画）
    UIManager::get_instance().DrawOverlay(
        m_position.x, m_position.y,
        m_size.x * m_progress, m_size.y,
        m_color);
}

//-----------------------------------------------------------------------------
// 進捗値をセット（0.0 ～ 1.0 の範囲でclamp）
//-----------------------------------------------------------------------------
void SeekBarUI::SetProgress(float p)
{
    m_progress = clamp(p, 0.0f, 1.0f);
}

//-----------------------------------------------------------------------------
// 画面サイズを基に中央下部へ配置
//-----------------------------------------------------------------------------
void SeekBarUI::UpdatePosition(int screenWidth, int screenHeight)
{
    m_position.x = (screenWidth - m_size.x) / 2.0f;
    m_position.y = screenHeight - m_size.y - 60.0f;
}


//-----------------------------------------------------------------------------
// マウスクリックでバー上をクリックしたら、進捗を更新
//-----------------------------------------------------------------------------
void SeekBarUI::OnMouseClick(float x, float y)
{
    if (!IsMouseInside(x, y)) return;

    // マウスのX座標からバーの進捗を計算
    float relativeX = x - m_position.x;
    float newProgress = relativeX / m_size.x;

    // 安全のため clamping
    newProgress = clamp(newProgress, 0.0f, 1.0f);

    // 進捗反映
    SetProgress(newProgress);

    // デバッグ出力（確認用）
    OutputDebugStringA("SeekBarUI::OnMouseClick - Progress Updated\n");
}


//-----------------------------------------------------------------------------
// 指定座標がバーの中にあるかチェック
//-----------------------------------------------------------------------------
bool SeekBarUI::IsMouseInside(float x, float y) const
{
    return (x >= m_position.x && x <= m_position.x + m_size.x &&
        y >= m_position.y && y <= m_position.y + m_size.y);
}
