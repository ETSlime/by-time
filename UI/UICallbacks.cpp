//=============================================================================
//
// [UICallbacks.cpp]
// Author : 
//
//=============================================================================
#include "UI/UICallbacks.h"
#include "UI/UITitleScene.h"

void OnClick_NewGame(void* userData)
{
	UITitleScene* scene = static_cast<UITitleScene*>(userData);
	scene->RequestStartFadeOut();
}
