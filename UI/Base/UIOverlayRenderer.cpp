//=============================================================================
//
// UIOverlayRenderer処理 [UIOverlayRenderer.cpp]
// Author : 
//
//=============================================================================
#include "UI/Base/UIOverlayRenderer.h"
#include "UI/Base/UISpriteRenderer.h"
#include "Core/TextureMgr.h"
#include <algorithm>

bool UIOverlayRenderer::Initialize(ID3D11Buffer* vertexBuffer)
{
    if (!vertexBuffer) return false;
    m_vertexBuffer = vertexBuffer;

    m_whiteTextureSRV = TextureMgr::get_instance().CreateTexture(UI_TEXTURE_PATH);
    if (m_whiteTextureSRV == nullptr)
        return false;

    // --- アルファブレンドステートの初期化 ---
    ID3D11Device* device = m_renderer.GetDevice();  // あなたの環境に合わせて取得方法を調整

    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = device->CreateBlendState(&blendDesc, &m_alphaBlendState);
    if (FAILED(hr)) {
        return false; // 失敗時の処理
    }

    return true;
}

UIOverlayRenderer::~UIOverlayRenderer()
{
    if (m_alphaBlendState)
    {
        m_alphaBlendState->Release();
        m_alphaBlendState = nullptr;
    }
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

//void UIOverlayRenderer::DrawHPBar(
//    ID3D11DeviceContext* ctx,
//    float x, float y,
//    float width, float height,
//    float hpRatio,
//    DirectX::XMFLOAT4 barColor,
//    DirectX::XMFLOAT4 bgColor)
//{
//    hpRatio = (hpRatio < 0.0f) ? 0.0f : ((hpRatio > 1.0f) ? 1.0f : hpRatio);
//
//     //フェード状態を一時退避して無効化
//     bool originalIsFading = m_isFading;
//     float originalAlpha = 1.0f;
//     if (originalIsFading)
//     {
//         originalAlpha = ComputeAlpha();
//         m_isFading = false;  // フェード無効化
//     }
//
//    // 背景バー（全面）
//    Draw(ctx, x + width * 0.5f, y + height * 0.5f, width, height, bgColor);
//
//    // HPバー（左端固定）
//    float hpWidth = width * hpRatio;
//    if (hpWidth > 0.0f)
//    {
//        float hpX = x + hpWidth * 0.5f;
//        float hpY = y + height * 0.5f;
//        Draw(ctx, hpX, hpY, hpWidth, height, barColor);
//    }
//}

void UIOverlayRenderer::DrawHPBar(
    ID3D11DeviceContext* ctx,
    float x, float y,
    float width, float height,
    float hpRatio,
    DirectX::XMFLOAT4 barColor,
    DirectX::XMFLOAT4 bgColor)
{
    // hpRatioの範囲制限は最初に行う
    hpRatio = (hpRatio < 0.0f) ? 0.0f : ((hpRatio > 1.0f) ? 1.0f : hpRatio);

    // フェード状態一時退避
    bool originalIsFading = m_isFading;
    float originalAlpha = 1.0f;
    if (originalIsFading)
    {
        originalAlpha = ComputeAlpha();
        m_isFading = false;
    }

    // 背景バー（長方形のまま）
    Draw(ctx, x + width * 0.5f, y + height * 0.5f, width, height, bgColor);

    // HPバー（台形に変更）
    float hpWidth = width * hpRatio;
    if (hpWidth > 0.0f)
    {
        float hpWidth = width * hpRatio;
        float centerX = x + hpWidth * 0.5f;
        float centerY = y + height * 0.5f;
        float skewX = -55.0f;  // お好みで調整

        UISpriteRenderer::SetSpriteParallelogram(
            m_vertexBuffer,
            centerX, centerY,
            hpWidth, height,
            skewX,
            0, 0, 1, 1,
            barColor
        );

        // 描画コマンド
        UINT stride = sizeof(UIVertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ctx->Draw(4, 0);
    }
}


void UIOverlayRenderer::Draw2DTexturedRect(
    ID3D11DeviceContext* ctx,
    float x, float y,
    float width, float height,
    const DirectX::XMFLOAT4& color)
{
    // UV全体 (0〜1) を使用して矩形描画
    UISpriteRenderer::SetSpriteWithColor(m_vertexBuffer, x, y, width, height, 0.0f, 0.0f, 1.0f, 1.0f, color);

    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ctx->Draw(4, 0);
}

void UIOverlayRenderer::DrawOverlayTexture(
    ID3D11DeviceContext* ctx,
    float x, float y,
    float width, float height,
    ID3D11ShaderResourceView* textureSRV,
    const DirectX::XMFLOAT4& color)
{
    if (!ctx || !textureSRV) return;

    // 1. シェーダーリソースビューをセット
    ctx->PSSetShaderResources(0, 1, &textureSRV);

    // 2. ブレンドステート（アルファブレンド）を有効にする
    ctx->OMSetBlendState(m_alphaBlendState, nullptr, 0xffffffff);

    // 3. 描画用の頂点バッファをセット
    // ここでは、画面左上が原点(0,0)のピクセル座標系と仮定
    // 既存の2D矩形描画関数を呼ぶ場合が多いです

    Draw2DTexturedRect(ctx, x, y, width, height, color);

    // 4. シェーダーリソースを解除（クリーンアップ）
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    ctx->PSSetShaderResources(0, 1, nullSRV);

    // 5. ブレンドステートを元に戻す（必要なら）
    ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

