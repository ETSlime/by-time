#pragma once
//=============================================================================
//
// 動画再生バーUIちゃん [SeekBarUI.h]
// Author : 
// シンプルな横長バーで、再生の進捗を視覚的に表示してくれるやさしい子
//
//=============================================================================
#include "UI/Base/UIElement.h"


// C++14対応 clamp（std::clampの代用）
template<typename T>
inline const T& clamp(const T& v, const T& lo, const T& hi)
{
    return (v < lo) ? lo : (hi < v ? hi : v);
}



//=============================================================================
// SeekBarUI クラス定義
//=============================================================================
class SeekBarUI : public UIElement
{
public:
    SeekBarUI();

    // 毎フレーム呼ばれる更新処理
    void Update(void) override;

    // 描画処理（進捗バーを表示）
    void Draw(void) override;

    // 進捗値をセット（0.0f～1.0fに制限）
    void SetProgress(float progress);

    // 画面中央下部に配置する
    void UpdatePosition(int screenWidth, int screenHeight);

    void OnMouseClick(float x, float y) override;// マウスクリック対応

private:
    float m_progress = 0.0f;  // 再生進捗（0.0 ～ 1.0）

    bool IsMouseInside(float x, float y) const; // 範囲判定用ヘルパー
};