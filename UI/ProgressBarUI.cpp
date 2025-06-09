// ProgressBarUI.cpp

#include "UI/ProgressBarUI.h"
#include "UI/UIManager.h"

ProgressBarUI::ProgressBarUI(float x, float y, float width, float height)
{
    m_position = { x, y };
    m_size = { width, height };
    m_progress = 0.0f;

    // デフォルトカラー
    m_backgroundColor = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
    m_fillColor = { 0.0f, 1.0f, 0.0f, 1.0f };       // 緑

    m_backgroundTexture = nullptr;
    m_fillTexture = nullptr;

}

void ProgressBarUI::Update()
{
    // 必要に応じてアニメーションや自動進行など
}

void ProgressBarUI::Draw()
{
    OutputDebugStringA("ProgressBarUI::Draw CALLED!\n"); // ←これを追加

    ID3D11DeviceContext* context = Renderer::get_instance().GetDeviceContext();

    // 背景の描画
    if (m_backgroundTexture)
    {
        UIManager::get_instance().DrawTexture(m_backgroundTexture, m_position.x, m_position.y, m_size.x, m_size.y);
    }
    else
    {
        UIManager::get_instance().DrawOverlay(m_position.x, m_position.y, m_size.x, m_size.y, m_backgroundColor);
    }

    // 塗り部分（進捗に応じた幅）
    float fillWidth = m_size.x * Clamp(m_progress, 0.0f, 1.0f);
    if (fillWidth > 0.0f)
    {
        if (m_fillTexture)
        {
            UIManager::get_instance().DrawTexture(m_fillTexture, m_position.x, m_position.y, fillWidth, m_size.y);
        }
        else
        {
            UIManager::get_instance().DrawOverlay(m_position.x, m_position.y, fillWidth, m_size.y, m_fillColor);
        }
    }
}

void ProgressBarUI::SetProgress(float progress)
{
    m_progress = Clamp(progress, 0.0f, 1.0f);
}

float ProgressBarUI::GetProgress() const
{
    return m_progress;
}

void ProgressBarUI::SetBackgroundTexture(ID3D11ShaderResourceView* texture)
{
    m_backgroundTexture = texture;
}

void ProgressBarUI::SetFillTexture(ID3D11ShaderResourceView* texture)
{
    m_fillTexture = texture;
}

bool ProgressBarUI::IsMouseOver(float mx, float my) const
{
    return mx >= m_position.x && mx <= m_position.x + m_size.x &&
        my >= m_position.y && my <= m_position.y + m_size.y;
}

void ProgressBarUI::OnMouseClick(float mouseX, float mouseY)
{
    if (!IsMouseOver(mouseX, mouseY)) return;

    float relativeX = (mouseX - m_position.x) / m_size.x;
    SetProgress(Clamp(relativeX, 0.0f, 1.0f));
}
