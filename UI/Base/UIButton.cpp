//=============================================================================
//
// [UIButton.cpp]
// Author : 
//
//=============================================================================
#include "UI/Base/UIButton.h"


UIButton::UIButton()
{
	// tint 配列を初期化（デフォルト値は白）
	for (auto& c : m_tint) c = { 1,1,1,1 };
}

UIButton::~UIButton()
{
}

void UIButton::InitWithTextures(ID3D11Buffer* vb, char* normalTex, char* hoverTex, char* pressedTex, char* disabledTex)
{
	m_vertexBuffer = vb;
    m_useTintMode = false;

    m_tex[0] = m_textureMgr.CreateTexture(normalTex);
    m_tex[1] = hoverTex ? m_textureMgr.CreateTexture(hoverTex) : m_tex[0];
    m_tex[2] = pressedTex ? m_textureMgr.CreateTexture(pressedTex) : m_tex[0];
    m_tex[3] = disabledTex ? m_textureMgr.CreateTexture(disabledTex) : m_tex[0];

    UpdateVisual();
}

void UIButton::InitWithSingleTexture(ID3D11Buffer* vb, char* texture, XMFLOAT4& tint_normal, XMFLOAT4& tint_hover, XMFLOAT4& tint_pressed, XMFLOAT4& tint_disabled)
{
	m_vertexBuffer = vb;
    m_useTintMode = true;

    m_tex[0] = m_textureMgr.CreateTexture(texture);
    // すべて同じテクスチャ
    m_tex[1] = m_tex[2] = m_tex[3] = m_tex[0];

    m_tint[0] = tint_normal;
    m_tint[1] = tint_hover;
    m_tint[2] = tint_pressed;
    m_tint[3] = tint_disabled;

    UpdateVisual();
}

void UIButton::Update(void)
{
	HandleMouseInput();
}

void UIButton::Draw(void)
{
    if (!m_currentTex) return;

    UISpriteRenderer::SetSpriteWithColor(
        m_vertexBuffer,
        m_position.x, m_position.y,
        m_size.x, m_size.y,
		0, 0, 1, 1,  // UV座標（全体を使用）
        m_currentTint);  // 色

    // テクスチャをバインド
    m_resourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, m_currentTex);

    UINT stride = sizeof(UIVertex);
    UINT offset = 0;
    m_renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    // 描画
    m_renderer.GetDeviceContext()->Draw(4, 0);
}

void UIButton::HandleMouseInput(void)
{
	int mouseX = GetMousePosX(); // マウスのX座標
	int mouseY = GetMousePosY(); // マウスのY座標
    bool isPressed = m_inputManager.IsMouseLeftPressed();

    if (!m_isEnabled)
    {
        m_state = ButtonState::Disabled;
        UpdateVisual();
        return;
    }

    const bool inside = IsCursorInside(mouseX, mouseY);

    // 押下開始
    if (isPressed && !m_prevMouseDown && inside)
    {
        m_state = ButtonState::Pressed;
        m_pressedInside = true;
        UpdateVisual();
    }
    // 押下中に外へ出た
    else if (isPressed && m_state == ButtonState::Pressed && !inside)
    {
        m_state = ButtonState::Normal;
        m_pressedInside = false;
        UpdateVisual();
    }
    // ボタン離す
    else if (!isPressed && m_prevMouseDown)
    {
        // 押したまま中にいて離した → クリック成功
        if (m_pressedInside && inside && m_onClick)
        {
            // 効果音
            //if (m_clickSE) AudioManager::GetInstance().PlaySE(m_clickSE);
            // コールバック
            m_onClick(m_userData);
        }
        m_pressedInside = false;

        // ホバー状態に戻るか通常に戻るか
        m_state = inside ? ButtonState::Hovered : ButtonState::Normal;
        UpdateVisual();
    }
    // ホバー検知
    else if (!isPressed)
    {
        const ButtonState next = inside ? ButtonState::Hovered : ButtonState::Normal;
        if (next != m_state)
        {
            m_state = next;
            UpdateVisual();
        }
    }

    m_prevMouseDown = isPressed;
}

void UIButton::SetOnClick(UIButtonCallback callback, void* userData)
{
    m_onClick = callback;
    m_userData = userData;
}

void UIButton::SetEnabled(bool enabled)
{
    if (m_isEnabled == enabled) return;
    m_isEnabled = enabled;
    m_state = enabled ? ButtonState::Normal : ButtonState::Disabled;
    UpdateVisual();
}

bool UIButton::IsCursorInside(int mouseX, int mouseY) const
{
    const auto& pos = GetPosition();
    const auto& size = GetSize();

	// ボタンの中心位置からの相対座標を計算
    const float left = pos.x - size.x * 0.5f;
    const float right = pos.x + size.x * 0.5f;
    const float top = pos.y - size.y * 0.5f;
    const float bottom = pos.y + size.y * 0.5f;

    return (mouseX >= left && mouseX <= right &&
        mouseY >= top && mouseY <= bottom);
}

void UIButton::UpdateVisual(void)
{
    const int idx = static_cast<int>(m_state);
    m_currentTex = m_tex[idx];
    m_currentTint = m_useTintMode ? m_tint[idx] : XMFLOAT4(1, 1, 1, 1);
}

