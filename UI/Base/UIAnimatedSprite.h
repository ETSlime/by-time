#pragma once
//=============================================================================
//
// UIアニメーションスプライト管理クラス [UIAnimatedSprite.h]
// Author : 
// これは 2D UI 上で「アニメーションするスプライト（画像）」を表示する、
// 再生機能付きの UIElement サブクラスなのだよ～
//
// 主な機能：
//  - テクスチャアトラス（格子状）でのアニメーション再生
//  - 任意のフレーム範囲で再生（例：PlayFrames(4, 7)）
//  - 再生速度調整 / 一時停止 / フレーム直接指定
//  - 非ループ時、最後のフレームでコールバックが呼ばれる
//  - フレーム切り替えは Timer に基づく累積時間制御～
// 
//=============================================================================
#include "UI/Base/UIElement.h"

class UIAnimatedSprite : public UIElement
{
public:
    void Init(ID3D11ShaderResourceView* tex, ID3D11Buffer* vb, 
        int columns, int rows, float frameTime, bool loop = true);

    // 再生範囲を指定（例：PlayFrames(4, 7) で 4~7フレーム再生）
    void PlayFrames(int start, int end);

    // 終了時のコールバック設定（非ループ時に最後のフレームで実行）
    void SetOnFinish(void (*callback)());

    void Play(void);
    void Stop(void);

    void Pause(void) { m_paused = true; }
    void Resume(void) { m_paused = false; }

    void Update(void) override;
    void Draw(void) override;

    // アニメーションの再生速度を設定する
    void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    float GetPlaybackSpeed(void) const { return m_playbackSpeed; }

    // 指定された進行度に応じてアニメーションフレームを手動設定する関数 (0.0～1.0 の範囲）
    void SetFrameDirectly(float ratio);

private:
    int m_columns = 1;     // 横のフレーム数
    int m_rows = 1;        // 縦のフレーム数
    int m_startFrame = 0;      // 再生開始フレーム
    int m_endFrame = 0;        // 再生終了フレーム
    float m_frameTime = 0.1f; // 各フレームの表示時間（秒）
    float m_playbackSpeed = 1.0f; // 再生スピード（1.0 = 通常）
    float m_currentTime = 0.0f; // 累積タイマー
    int m_currentFrame = 0; // 現在のフレーム番号
    bool m_loop = true; // ループ再生するか
    bool m_paused = false;
    bool m_finished = false;   // 一度終了したらtrueに
    void (*m_onFinish)() = nullptr; // 再生終了時のコールバック

    ID3D11ShaderResourceView* m_texture = nullptr;
    ID3D11Buffer* m_vertexBuffer = nullptr; // 頂点バッファ（スプライト用）
    Timer& m_timer = Timer::get_instance();
    Renderer& m_renderer = Renderer::get_instance();
    ShaderResourceBinder& m_resourceBinder = ShaderResourceBinder::get_instance();
};