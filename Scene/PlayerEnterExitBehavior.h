#pragma once
//=============================================================================
//
// [PlayerEnterExitBehavior.h]
// Author : 
// 
//=============================================================================
#include "UI/Base/UIInteractPrompt.h"
#include "UI/Base/UIManager.h"
#include "Scene/Player.h"
#include "Scene/EventTrigger.h"

class PlayerEnterExitBehavior : public IEventBehavior
{
private:
	Player* m_player = nullptr; // プレイヤーキャラクターへのポインタ
	UIInteractPrompt* m_prompt = nullptr; // インタラクションプロンプトへのポインタ

public:
    void OnTrigger(const Collider* activator) override;

    void OnExit(const Collider* activator) override;
};
