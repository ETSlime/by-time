#pragma once
//=============================================================================
//
// [UIMainHUD.h]
// Author : 
// 
//=============================================================================
#include "Scene/Player.h"
#include "UI/Base/UIElement.h"

class UIMainHUD : public UIElement
{
public:
    UIMainHUD() {};
    ~UIMainHUD();

    void Initialize(void);
    void Update(void) override;
    void Draw(void) override;

private:
    void DrawNumers(int number, float posX, float posY);

	Player* m_player = nullptr;
    UISprite* m_hpBarBack = nullptr;
    UISprite* m_hpBarFill = nullptr;

    UISprite* m_equipSlot = nullptr;
    UISprite* m_weaponIcon = nullptr;
    UISprite* m_gunIcon = nullptr;
    UISprite* m_seedIcon = nullptr;
	UISprite* m_bulletIcon = nullptr;
    UISprite* m_sunBulletIcon = nullptr;
	UISprite* m_itemCount = nullptr;
	UISprite* m_itemCountSign = nullptr;
	UISprite* m_numInfinity = nullptr;

	UISprite* m_bossHPGauge = nullptr;
	UISprite* m_bossHPGaugeCover = nullptr;

    UISprite* m_crosshair = nullptr;

    SimpleArray<UISprite*> m_sprites;
};