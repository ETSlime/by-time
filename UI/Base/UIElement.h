//=============================================================================
//
// ぜんぶのUIを優しく包む基底クラスちゃん [UIElement.h]
// Author : 
// 位置・サイズ・Z順・ホバー・オーバーレイなど、UIに必要な基本機能を提供する抽象クラスですっ
// 全てのUIちゃんたちの土台になって、しずかに見守る頼れるお姉さんタイプなの
//
//=============================================================================
#pragma once
#include "Core/Timer.h"
#include "Core/Graphics/Renderer.h"
#include "UI/Base/UISpriteRenderer.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct UVRect
{
    float left;
    float top;
    float right;
    float bottom;
}; // UV矩形構造体

class UIElement
{
public:
    virtual ~UIElement() {}

    virtual void Update(void) = 0;
    virtual void Draw(void) = 0;

    virtual void OnHover(void) {}
    virtual void OnUnhover(void) {}

    // 位置とサイズを設定するための関数
    void SetPosition(const XMFLOAT2& pos) { m_position = pos; }
    void SetSize(const XMFLOAT2& size) { m_size = size; }
    void SetColor(const XMFLOAT4& color) { m_color = color; }

    // ゲッター（他のUI要素からも取得できるように）
    const XMFLOAT2& GetPosition() const { return m_position; }
    const XMFLOAT2& GetSize() const { return m_size; }
    const XMFLOAT4& GetColor() const { return m_color; }

    void SetZOrder(int z) { m_zOrder = z; }
    int  GetZOrder(void) const { return m_zOrder; }

//追記・変更点
    //bool IsMouseOver(float mouseX, float mouseY) const { return false; }
    bool IsMouseOver(float mouseX, float mouseY) const
    {
        return (mouseX >= m_position.x && mouseX <= m_position.x + m_size.x &&
            mouseY >= m_position.y && mouseY <= m_position.y + m_size.y);
    }
//追記・変更点

    virtual bool NeedsOverlay() const { return HasCustomOverlay(); }
    virtual bool HasCustomOverlay(void) const { return m_customOverlay; }
    virtual void DrawCustomOverlay(void) const {}

    virtual void OnMouseClick(float x, float y) {}
protected:
    XMFLOAT2 m_position = { 0, 0 };     // 左上座標（または中心でもOK）
    XMFLOAT2 m_size{ 100, 100 };     // サイズ（幅×高さ）
    XMFLOAT4 m_color = { 1, 1, 1, 1 };  // 色（RGBA）
    int m_zOrder = 0;
    bool m_isHovering = false;
    bool m_customOverlay = false;
};