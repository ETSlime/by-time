//=============================================================================
//
// [UIInteractPrompt.cpp]
// Author : 
// 
//=============================================================================
#include "UI/Base/UIInteractPrompt.h"
#include "UI/Base/UIManager.h"
#include "Core/TextureMgr.h"

void UIInteractPrompt::Initialize(const char* iconPath)
{
	m_icon = new UISprite(
		UIManager::get_instance().GetSpriteVertexBuffer(),
		TextureMgr::get_instance().CreateTexture(const_cast<char*>(iconPath))
	);
}

void UIInteractPrompt::Update(void)
{
}

void UIInteractPrompt::Draw(void)
{
	m_icon->Draw();
}

void UIInteractPrompt::SetCallback(InteractCallback cb, void* userData)
{
	m_callback = cb;
	m_userData = userData;
}

void UIInteractPrompt::Trigger(void)
{
	if (m_callback)
		m_callback(m_userData);
}
