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

	// セッター（他のUI要素からは変更できないように）
    void SetPosition(const XMFLOAT2& pos) { m_position = pos; }
	void SetScale(const XMFLOAT2& scale) { m_scale = scale; }
	void SetRotation(float rot) { m_rot = rot; }
    void SetSize(const XMFLOAT2& size) { m_size = size; }
    void SetColor(const XMFLOAT4& color) { m_color = color; }

    // ゲッター（他のUI要素からも取得できるように）
    const XMFLOAT2& GetPosition(void) const { return m_position; }
    const XMFLOAT2& GetScale(void) const { return m_scale; }
	float GetRotation(void) const { return m_rot; }
    const XMFLOAT2& GetSize(void) const { return m_size; }
    const XMFLOAT4& GetColor(void) const { return m_color; }

    void SetZOrder(int z) { m_zOrder = z; }
    int  GetZOrder(void) const { return m_zOrder; }

    bool IsMouseOver(float mouseX, float mouseY) const { return false; }

    virtual bool NeedsOverlay(void) const { return HasCustomOverlay(); }
    virtual bool HasCustomOverlay(void) const { return m_customOverlay; }
    virtual void DrawCustomOverlay(void) const {}

protected:
    XMFLOAT2 m_position = { 0, 0 };     // 左上座標（または中心でもOK）
    XMFLOAT2 m_scale{ 1.0f, 1.0f };
    float m_rot = 0.0f;
    XMFLOAT2 m_size{ 100, 100 };     // サイズ（幅×高さ）
    XMFLOAT4 m_color = { 1, 1, 1, 1 };  // 色（RGBA）
    int m_zOrder = 0;
    bool m_isHovering = false;
    bool m_customOverlay = false;
};