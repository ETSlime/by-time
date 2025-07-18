//=============================================================================
//
// [PlayerEnterExitBehavior.cpp]
// Author : 
// 
//=============================================================================
#include "Scene/PlayerEnterExitBehavior.h"
#include "Core/GameSystem.h"

void PlayerEnterExitBehavior::OnTrigger(const Collider* activator)
{
    if (activator->tag != ColliderTag::PLAYER)
        return;

	m_player = GameSystem::get_instance().GetPlayer();

    m_prompt = new UIInteractPrompt();
    m_prompt->Initialize("data/TEXTURE/UI/interactUI.png");

    m_prompt->SetCallback(m_player->OnClimbPromptTriggered, m_player);
    m_prompt->SetPosition(700, 500); // ‰æ–Ê’†‰›‚É”z’u

    UIManager::get_instance().SetActivePrompt(m_prompt);
}

void PlayerEnterExitBehavior::OnExit(const Collider* activator)
{
    if (activator->tag != ColliderTag::PLAYER)
        return;

    UIManager::get_instance().ClearActivePrompt();
}
