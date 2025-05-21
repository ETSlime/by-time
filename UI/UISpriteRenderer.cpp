//=============================================================================
//
// UISpriteRenderer処理 [UISpriteRenderer.cpp]
// Author : 
//
//=============================================================================
#include "UISpriteRenderer.h"

void UISpriteRenderer::SetSpriteCenter(ID3D11Buffer* buf, float X, float Y, float Width, float Height,
    float U, float V, float UW, float VH)
{
    D3D11_MAPPED_SUBRESOURCE msr;
    Renderer::get_instance().GetDeviceContext()->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    UIVertex* vertex = reinterpret_cast<UIVertex*>(msr.pData);

    float hw = Width * 0.5f;
    float hh = Height * 0.5f;

    // 中心を基準に頂点を設定
    vertex[0].Position = XMFLOAT2(X - hw, Y - hh); // 左上
    vertex[1].Position = XMFLOAT2(X + hw, Y - hh); // 右上
    vertex[2].Position = XMFLOAT2(X - hw, Y + hh); // 左下
    vertex[3].Position = XMFLOAT2(X + hw, Y + hh); // 右下

    vertex[0].TexCoord = XMFLOAT2(U, V);
    vertex[1].TexCoord = XMFLOAT2(U + UW, V);
    vertex[2].TexCoord = XMFLOAT2(U, V + VH);
    vertex[3].TexCoord = XMFLOAT2(U + UW, V + VH);

    Renderer::get_instance().GetDeviceContext()->Unmap(buf, 0);
}

void UISpriteRenderer::SetSpriteLeftTop(ID3D11Buffer* buf, float X, float Y, float Width, float Height,
    float U, float V, float UW, float VH)
{
    D3D11_MAPPED_SUBRESOURCE msr;
    Renderer::get_instance().GetDeviceContext()->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    UIVertex* vertex = reinterpret_cast<UIVertex*>(msr.pData);

    // 左上を基準に頂点を設定
    vertex[0].Position = XMFLOAT2(X, Y);                   // 左上
    vertex[1].Position = XMFLOAT2(X + Width, Y);           // 右上
    vertex[2].Position = XMFLOAT2(X, Y + Height);          // 左下
    vertex[3].Position = XMFLOAT2(X + Width, Y + Height);  // 右下

    vertex[0].TexCoord = XMFLOAT2(U, V);
    vertex[1].TexCoord = XMFLOAT2(U + UW, V);
    vertex[2].TexCoord = XMFLOAT2(U, V + VH);
    vertex[3].TexCoord = XMFLOAT2(U + UW, V + VH);

    Renderer::get_instance().GetDeviceContext()->Unmap(buf, 0);
}

void UISpriteRenderer::SetSpriteWithColor(ID3D11Buffer* buf, float X, float Y, float Width, float Height,
    float U, float V, float UW, float VH, XMFLOAT4 color)
{
    D3D11_MAPPED_SUBRESOURCE msr;
    Renderer::get_instance().GetDeviceContext()->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    UIVertex* vertex = reinterpret_cast<UIVertex*>(msr.pData);

    float hw = Width * 0.5f;
    float hh = Height * 0.5f;

    // 中心基準で頂点設定（左上、右上、左下、右下）
    vertex[0].Position = XMFLOAT2(X - hw, Y - hh);
    vertex[1].Position = XMFLOAT2(X + hw, Y - hh);
    vertex[2].Position = XMFLOAT2(X - hw, Y + hh);
    vertex[3].Position = XMFLOAT2(X + hw, Y + hh);

    vertex[0].TexCoord = XMFLOAT2(U, V);
    vertex[1].TexCoord = XMFLOAT2(U + UW, V);
    vertex[2].TexCoord = XMFLOAT2(U, V + VH);
    vertex[3].TexCoord = XMFLOAT2(U + UW, V + VH);

    // カラー設定
    vertex[0].Color = color;
    vertex[1].Color = color;
    vertex[2].Color = color;
    vertex[3].Color = color;

    Renderer::get_instance().GetDeviceContext()->Unmap(buf, 0);
}

void UISpriteRenderer::SetSpriteWithRotation(ID3D11Buffer* buf, float X, float Y, float Width, float Height,
    float U, float V, float UW, float VH,
    XMFLOAT4 /*color*/, float Rot)
{
    D3D11_MAPPED_SUBRESOURCE msr;
    Renderer::get_instance().GetDeviceContext()->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    UIVertex* vertex = reinterpret_cast<UIVertex*>(msr.pData);

    float hw = Width * 0.5f;
    float hh = Height * 0.5f;

    // 回転中心からの距離と角度を計算
    float baseAngle = atan2f(hh, hw);
    float radius = sqrtf(hw * hw + hh * hh);

    float x, y;

    // 頂点を回転して配置
    x = X - cosf(baseAngle + Rot) * radius;
    y = Y - sinf(baseAngle + Rot) * radius;
    vertex[0].Position = XMFLOAT2(x, y); // 左上

    x = X + cosf(baseAngle - Rot) * radius;
    y = Y - sinf(baseAngle - Rot) * radius;
    vertex[1].Position = XMFLOAT2(x, y); // 右上

    x = X - cosf(baseAngle - Rot) * radius;
    y = Y + sinf(baseAngle - Rot) * radius;
    vertex[2].Position = XMFLOAT2(x, y); // 左下

    x = X + cosf(baseAngle + Rot) * radius;
    y = Y + sinf(baseAngle + Rot) * radius;
    vertex[3].Position = XMFLOAT2(x, y); // 右下

    vertex[0].TexCoord = XMFLOAT2(U, V);
    vertex[1].TexCoord = XMFLOAT2(U + UW, V);
    vertex[2].TexCoord = XMFLOAT2(U, V + VH);
    vertex[3].TexCoord = XMFLOAT2(U + UW, V + VH);

    Renderer::get_instance().GetDeviceContext()->Unmap(buf, 0);
}
