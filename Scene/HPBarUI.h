#pragma once

#include "UI/Base/UIElement.h"
#include <DirectXMath.h>

class HPBarUI : public UIElement
{
public:
    HPBarUI() = default;
    ~HPBarUI() override = default;

    void Update() override;
    void Draw() override;

    void SetHP(float hp);
    float GetHP() const { return m_hp; }

    void SetPosition(const DirectX::XMFLOAT2& pos) { m_position = pos; }
    void SetSize(const DirectX::XMFLOAT2& size) { m_size = size; }

    void SetBgColor(const DirectX::XMFLOAT4& color) { m_bgColor = color; }

    void SetBackgroundColor(const DirectX::XMFLOAT4& color) { m_bgColor = color; }
    DirectX::XMFLOAT4 GetBackgroundColor() const { return m_bgColor; }

    void SetDecorationTexture(ID3D11ShaderResourceView* srv) { m_decoTextureSRV = srv; }
private:
    float m_hp = 1.0f;  // 0.0f〜1.0f
    DirectX::XMFLOAT2 m_position{ 0.0f, 0.0f };
    DirectX::XMFLOAT2 m_size{ 200.0f, 25.0f };
    DirectX::XMFLOAT4 m_barColor{ 0.1f, 0.1f, 0.1f, 1.0f };
    DirectX::XMFLOAT4 m_bgColor{ 0.1f, 0.1f, 0.1f, 0.0f };

    ID3D11ShaderResourceView* m_decoTextureSRV = nullptr; // 装飾テクスチャ（任意）
};
