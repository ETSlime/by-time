#include "ProgressBarUI.h"
#include "UIManager.h"
#include "Utility/InputManager.h"

ProgressBarUI::ProgressBarUI(float x, float y, float width, float height)
{
    m_position = { x, y };
    m_size = { width, height };
    m_progress = 0.0f;
    m_backgroundColor = { 0.2f, 0.2f, 0.2f, 1.0f };
    m_fillColor = { 0.8f, 0.4f, 0.1f, 1.0f };
    m_backgroundTexture = nullptr;
    m_fillTexture = nullptr;
}

void ProgressBarUI::Update()
{
    float mx = static_cast<float>(InputManager::get_instance().GetMouseX());
    float my = static_cast<float>(InputManager::get_instance().GetMouseY());

    if (InputManager::get_instance().IsMouseLeftPressed() && IsMouseOver(mx, my))
    {
        OnMouseClick(mx, my);
    }
}

void ProgressBarUI::Draw()
{
    auto& ui = UIManager::get_instance();

    // ”wŒi
    if (m_backgroundTexture)
    {
        ui.DrawTexture(m_backgroundTexture, m_position.x, m_position.y, m_size.x, m_size.y);
    }
    else
    {
        ui.DrawOverlay(m_position.x, m_position.y, m_size.x, m_size.y, m_backgroundColor);
    }

    // ƒvƒƒOƒŒƒX•”•ª
    float fillWidth = m_size.x * m_progress;
    if (m_fillTexture)
    {
        ui.DrawTexture(m_fillTexture, m_position.x, m_position.y, fillWidth, m_size.y);
    }
    else
    {
        ui.DrawOverlay(m_position.x, m_position.y, fillWidth, m_size.y, m_fillColor);
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
    float relativeX = (mouseX - m_position.x) / m_size.x;
    SetProgress(relativeX);
}
