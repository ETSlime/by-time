//=============================================================================
//
// UIOverlayRenderer処理 [UIOverlayRenderer.cpp]
// Author : 
//
//=============================================================================
#include "UI/Base/UIOverlayRenderer.h"
#include "UI/Base/UISpriteRenderer.h"
#include "Core/TextureMgr.h"

bool UIOverlayRenderer::Initialize(ID3D11Buffer* vertexBuffer)
{
    if (!vertexBuffer) return false;
    m_vertexBuffer = vertexBuffer;

    m_whiteTextureSRV = TextureMgr::get_instance().CreateTexture(UI_TEXTURE_PATH);
    if (m_whiteTextureSRV == nullptr)
        return false;

    return true;
}

void UIOverlayRenderer::Draw(ID3D11DeviceContext* ctx, float x, float y, float width, float height, XMFLOAT4 color)
{
    // アニメーション中であれば、アルファ値を補間する
    if (m_isFading)
    {
        color.w = ComputeAlpha();
    }

    // テクスチャをバインド
    m_shaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, m_whiteTextureSRV);
    // 頂点バッファを更新
    UISpriteRenderer::SetSpriteWithColor(m_vertexBuffer, x, y, width, height, 0, 0, 1, 1, color);

    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    m_renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画
    m_renderer.GetDeviceContext()->Draw(4, 0);
}

void UIOverlayRenderer::StartFadeIn(float duration, XMFLOAT4 color)
{
    m_fadeIn = true;
    m_fadeDuration = duration;
    m_fadeTime = 0.0f;
    m_isFading = true;
    m_targetColor = color;
}

void UIOverlayRenderer::StartFadeOut(float duration, XMFLOAT4 color)
{
    m_fadeIn = false;
    m_fadeDuration = duration;
    m_fadeTime = 0.0f;
    m_isFading = true;
    m_targetColor = color;
}

void UIOverlayRenderer::Update(float deltaTime)
{
    if (m_isFading)
    {
        m_fadeTime += deltaTime;
        if (m_fadeTime >= m_fadeDuration)
        {
            m_fadeTime = m_fadeDuration;
            m_isFading = false;
        }
    }
}

float UIOverlayRenderer::ComputeAlpha() const
{
    float t = m_fadeTime / m_fadeDuration;
    t = (t > 1.0f) ? 1.0f : t;
    return m_fadeIn ? (t * m_targetColor.w) : ((1.0f - t) * m_targetColor.w);
}