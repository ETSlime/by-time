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
	Player* m_player = nullptr;
    UISprite* m_hpBarBack = nullptr;
    UISprite* m_hpBarFill = nullptr;

    UISprite* m_equipSlot = nullptr;
    UISprite* m_weaponIcon = nullptr;
    UISprite* m_gunIcon = nullptr;
    UISprite* m_seedIcon = nullptr;

    UISprite* m_crosshair = nullptr;

    SimpleArray<UISprite*> m_sprites;
};