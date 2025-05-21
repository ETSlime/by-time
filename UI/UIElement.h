//=============================================================================
//
// ぜんぶのUIを優しく包む基底クラスちゃん [UIElement.h]
// Author : 
// 位置・サイズ・Z順・ホバー・オーバーレイなど、UIに必要な基本機能を提供する抽象クラスですっ
// 全てのUIちゃんたちの土台になって、しずかに見守る頼れるお姉さんタイプなの
//
//=============================================================================
#pragma once
#include "Core/Graphics/Renderer.h"

class UIElement
{
public:
    virtual void Update(void) = 0;
    virtual void Draw(void) = 0;

    virtual void OnHover(void) {}
    virtual void OnUnhover(void) {}

    void SetZOrder(int z) { m_zOrder = z; }
    int  GetZOrder(void) const { return m_zOrder; }

    bool IsMouseOver(float mouseX, float mouseY) const { return false; }

    virtual bool NeedsOverlay() const { return HasCustomOverlay(); }
    virtual bool HasCustomOverlay(void) const { return m_customOverlay; }
    virtual void DrawCustomOverlay(void) const {}

protected:
    XMFLOAT2 m_position;
    XMFLOAT2 m_size;
    int m_zOrder = 0;
    bool m_isHovering = false;
    bool m_customOverlay = false;
};
