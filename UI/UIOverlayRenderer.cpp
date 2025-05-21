//=============================================================================
//
// UIOverlayRenderer処理 [UIOverlayRenderer.cpp]
// Author : 
//
//=============================================================================
#include "UIOverlayRenderer.h"
#include "UISpriteRenderer.h"
#include "Core/TextureMgr.h"

bool UIOverlayRenderer::Initialize(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    if (m_vertexBuffer == nullptr)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(UIVertex) * 4;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&desc, nullptr, &m_vertexBuffer);
    }

    if (FAILED(hr))
		return false;

    m_whiteTextureSRV = TextureMgr::get_instance().CreateTexture(UI_TEXTURE_PATH);

    if (m_whiteTextureSRV == nullptr)
        return false;

    return true;
}

void UIOverlayRenderer::Shutdown()
{
    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    SafeRelease(&m_whiteTextureSRV);
}

void UIOverlayRenderer::Draw(ID3D11DeviceContext* ctx, float x, float y, float width, float height, XMFLOAT4 color)
{
    // アニメーション中であれば、アルファ値を補間する
    if (m_isFading)
    {
        color.w = ComputeAlpha();
    }

    // テクスチャをバインド
    Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &m_whiteTextureSRV);
    // 頂点バッファを更新
    UISpriteRenderer::SetSpriteWithColor(m_vertexBuffer, x, y, width, height, 0, 0, 1, 1, color);

    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    Renderer::get_instance().GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    Renderer::get_instance().GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    // 描画
    Renderer::get_instance().GetDeviceContext()->Draw(4, 0);
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