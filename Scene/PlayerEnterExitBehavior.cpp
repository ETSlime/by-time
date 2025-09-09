//=============================================================================
//
// [PlayerEnterExitBehavior.cpp]
// Author : 
// 
//=============================================================================
#include "Scene/PlayerEnterExitBehavior.h"
#include "Core/GameSystem.h"

void PlayerEnterExitBehavior::Initialize(void)
{
    m_prompt = new UIInteractPrompt();
    m_prompt->Initialize("data/TEXTURE/UI/interactUI.png");
    m_prompt->SetPosition(1200, 500);
    m_player = GameSystem::get_instance().GetPlayer();
}

void PlayerEnterExitBehavior::OnTrigger(const Collider* activator)
{
    if (activator->tag != ColliderTag::PLAYER)
        return;

    switch (m_actionType)
    {
    case PlayerCallbackAction::PlaceItem:
    {
        if (!m_player->HasSeed())
            return;

        PlaceItemContext* ctx = new PlaceItemContext();
		ctx->player = m_player;
		ctx->itemType = ItemType::Seed; // ここでは種を配置すること
        ctx->owner = this;
		ctx->postCallback = [](PlayerEnterExitBehaviorContext* ctx)
			{
                ctx->owner->m_actionType = PlayerCallbackAction::None;
			};

        m_prompt->SetCallback(m_player->OnPlaceItemPromptTriggered, ctx);

        break;
    }
    case PlayerCallbackAction::StartClimbUp:
    {
        ClimbContext* climbCtx = new ClimbContext();
		climbCtx->player = m_player;
		climbCtx->climbState = PlayerState::START_CLIMBING_UP; // クライムアップ状態
        climbCtx->owner = this;

		// コールバック関数を設定
        climbCtx->postCallback = [](PlayerEnterExitBehaviorContext* ctx)
			{
                ctx->owner->m_actionType = PlayerCallbackAction::StopClimbDown; // 反転させる
                //ctx->owner->m_triggerAABB->minPoint = XMFLOAT3(-1501.6f, -4975.0f, 583.6f);
                //ctx->owner->m_triggerAABB->maxPoint = XMFLOAT3(-1319.2f, -4938.0f, 764.0f);
			};

        m_prompt->SetCallback(m_player->OnClimbPromptTriggered, climbCtx);

        break;
    }
    case PlayerCallbackAction::StopClimbUp:
    {
        ClimbContext* climbCtx = new ClimbContext();
        climbCtx->player = m_player;
        climbCtx->climbState = PlayerState::STOP_CLIMBING_UP; // クライムアップ状態
        climbCtx->owner = this;

        // コールバック関数を設定
        climbCtx->postCallback = [](PlayerEnterExitBehaviorContext* ctx)
            {
                ctx->owner->m_actionType = PlayerCallbackAction::StartClimbDown; // 反転させる
            };

        m_prompt->SetCallback(m_player->OnClimbPromptTriggered, climbCtx);
        m_player->GetPlayerAttributes().enableClimbUp = false; // クライムダウンを無効化
        break;
    }
    case PlayerCallbackAction::StartClimbDown:
    {
        ClimbContext* climbCtx = new ClimbContext();
        climbCtx->player = m_player;
        climbCtx->climbState = PlayerState::START_CLIMBING_DOWN; // クライムアップ状態
        climbCtx->owner = this;

        // コールバック関数を設定
        climbCtx->postCallback = [](PlayerEnterExitBehaviorContext* ctx)
            {
                ctx->owner->m_actionType = PlayerCallbackAction::StopClimbUp; // 反転させる
            };

        m_prompt->SetCallback(m_player->OnClimbPromptTriggered, climbCtx);

        break;
    }
    case PlayerCallbackAction::StopClimbDown:
    {
        ClimbContext* climbCtx = new ClimbContext();
        climbCtx->player = m_player;
        climbCtx->climbState = PlayerState::STOP_CLIMBING_DOWN; // クライムアップ状態
        climbCtx->owner = this;
        // コールバック関数を設定
        climbCtx->postCallback = [](PlayerEnterExitBehaviorContext* ctx)
            {
                ctx->owner->m_actionType = PlayerCallbackAction::StartClimbUp; // 反転させる
                //ctx->owner->m_triggerAABB->minPoint = XMFLOAT3(-1501.6f, -4914.0f, 583.6f);
                //ctx->owner->m_triggerAABB->maxPoint = XMFLOAT3(-1319.2f, -4877.0f, 764.0f);
            };

        m_prompt->SetCallback(m_player->OnClimbPromptTriggered, climbCtx);
        m_player->GetPlayerAttributes().enableClimbDown = false; // クライムダウンを無効化
        break;
    }
    default:
        return;
    }

    UIManager::get_instance().SetActivePrompt(m_prompt);
}

void PlayerEnterExitBehavior::OnExit(const Collider* activator)
{
    if (activator->tag != ColliderTag::PLAYER)
        return;

    switch (m_actionType)
    {
    case PlayerCallbackAction::StopClimbUp:
        m_player->GetPlayerAttributes().enableClimbUp = true; // クライムアップを有効化
        break;
    case PlayerCallbackAction::StopClimbDown:
		m_player->GetPlayerAttributes().enableClimbDown = true; // クライムダウンを有効化
        break;
    default:
        break;
    }

    m_interacted = false;
    UIManager::get_instance().ClearActivePrompt();
}
