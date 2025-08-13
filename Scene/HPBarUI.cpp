#include "Scene/HPBarUI.h"
#include "UI/Base/UIManager.h"
#include "UI/Base/UIOverlayRenderer.h"

void HPBarUI::Update()
{
    // 必要に応じてHP減少などテストコードを書く
}

void HPBarUI::Draw()
{
    UIOverlayRenderer& overlay = UIManager::get_instance().GetOverlayRenderer();
    ID3D11DeviceContext* ctx = UIManager::get_instance().GetRenderer().GetDeviceContext();

    // 1. HPバーの描画
    overlay.DrawHPBar(
        ctx,
        m_position.x, m_position.y,
        m_size.x, m_size.y,
        m_hp,
        m_barColor,
        m_bgColor
    );

    // 2. 装飾テクスチャの重ね描き（フルHD）
    if (m_decoTextureSRV)
    {
        overlay.DrawOverlayTexture(
            ctx,
            325.0f, 185.0f,              // 描画位置（画面左上）
            650.0f, 370.0f,        // 描画サイズ（フルHD）
            m_decoTextureSRV,
            { 1, 1, 1, 1 }           // 色（RGBA）— アルファ0.5fなどで透明にも
        );
    }
}


void HPBarUI::SetHP(float hp)
{
    if (hp < 0.0f) m_hp = 0.0f;
    else if (hp > 1.0f) m_hp = 1.0f;
    else m_hp = hp;
}
