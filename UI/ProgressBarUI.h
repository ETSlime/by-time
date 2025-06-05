#pragma once
#include "UIElement.h"
#include <d3d11.h>
#include <DirectXMath.h>

class ProgressBarUI : public UIElement
{
public:
    ProgressBarUI(float x, float y, float width, float height);

    void Update() override;
    void Draw() override;

    void SetProgress(float progress); // 0.0 ～ 1.0
    float GetProgress() const;

    // テクスチャ設定（任意）
    void SetBackgroundTexture(ID3D11ShaderResourceView* texture);
    void SetFillTexture(ID3D11ShaderResourceView* texture);

    // マウス操作
    void OnMouseClick(float mouseX, float mouseY);

private:
    float m_progress; // 0.0f ～ 1.0f
    DirectX::XMFLOAT4 m_backgroundColor;
    DirectX::XMFLOAT4 m_fillColor;

    ID3D11ShaderResourceView* m_backgroundTexture;
    ID3D11ShaderResourceView* m_fillTexture;

    template<typename T>
    T Clamp(const T& value, const T& low, const T& high)
    {
        return (value < low) ? low : (value > high) ? high : value;
    }

    bool IsMouseOver(float mx, float my) const;
};
