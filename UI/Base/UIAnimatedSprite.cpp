//=============================================================================
//
// UIアニメーションスプライト管理クラス [UIAnimatedSprite.cpp]
// Author : 
// これは 2D UI 上で「アニメーションするスプライト（画像）」を表示する、
// 再生機能付きの UIElement サブクラスなのだよ～
// 
//=============================================================================
#include "UI/Base/UIAnimatedSprite.h"

void UIAnimatedSprite::Init(ID3D11ShaderResourceView* tex, ID3D11Buffer* vb, 
    int columns, int rows, float frameTime, bool loop)
{
    m_texture = tex;
    m_vertexBuffer = vb;
    m_columns = columns;
    m_rows = rows;
    m_frameTime = frameTime;
    m_loop = loop;

    m_startFrame = 0;
    m_endFrame = columns * rows - 1;
    m_currentFrame = m_startFrame;
    m_finished = false;
}

void UIAnimatedSprite::PlayFrames(int start, int end)
{
    m_startFrame = start;
    m_endFrame = end;
    m_currentFrame = start;
    m_finished = false;
}

void UIAnimatedSprite::SetOnFinish(void (*callback)())
{
    m_onFinish = callback;
}

void UIAnimatedSprite::Play(void)
{
    m_finished = false;
    m_currentTime = 0.0f;

    // フレームが終わってたら最初に戻す
    if (m_currentFrame >= m_endFrame || m_currentFrame < m_startFrame)
        m_currentFrame = m_startFrame;
}

void UIAnimatedSprite::Stop(void)
{
    m_currentFrame = m_startFrame;
    m_finished = true;
}

void UIAnimatedSprite::Update(void)
{
    if (m_finished || m_paused || (!m_loop && m_currentFrame >= m_endFrame))
        return;

    m_currentTime += m_timer.GetDeltaTime() * m_playbackSpeed;

    if (m_currentTime >= m_frameTime)
    {
        m_currentTime -= m_frameTime;
        m_currentFrame++;

        if (m_currentFrame >= m_endFrame)
        {
            if (m_loop)
                m_currentFrame = m_startFrame;
            else
            {
                m_currentFrame = m_endFrame;
                m_finished = true;
                if (m_onFinish)
                    m_onFinish();
            }
        }
    }
}

void UIAnimatedSprite::Draw()
{
    if (!m_texture || !m_vertexBuffer) return;

    int totalFrames = m_columns * m_rows;
    if (m_currentFrame >= totalFrames) return;

    // UV計算
    float uvW = 1.0f / static_cast<float>(m_columns);
    float uvH = 1.0f / static_cast<float>(m_rows);

    int col = m_currentFrame % m_columns;
    int row = m_currentFrame / m_columns;

    float u = col * uvW;
    float v = row * uvH;

    // スプライトを描画（左上基準で）
    UISpriteRenderer::SetSpriteWithColor(
        m_vertexBuffer,
        m_position.x, m_position.y,
        m_size.x, m_size.y,
        u, v, uvW, uvH,
        XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)
    );

    // テクスチャをバインド
    m_resourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, m_texture);

    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    m_renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    // 描画
    m_renderer.GetDeviceContext()->Draw(4, 0);
}

void UIAnimatedSprite::SetFrameDirectly(float ratio)
{
    if (m_loop || m_finished) return; // ループ中や再生終了なら無視

    int totalFrames = m_endFrame - m_startFrame;
    m_currentFrame = m_startFrame + static_cast<int>(totalFrames * ratio);
    if (m_currentFrame >= m_endFrame)
        m_currentFrame = m_endFrame - 1;
}
