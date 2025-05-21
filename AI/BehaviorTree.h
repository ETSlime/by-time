#pragma once
//=============================================================================
//
// 敵AI用ビヘイビアツリー制御 [BehaviorTree.h]
// Author : 
// セレクター・シーケンス・条件ノードを用いた敵行動の決定ロジックを管理する
// 各行動はExecuteにより逐次評価・実行される
//
//=============================================================================
#include "Utility/SimpleArray.h"
#include "Scene/Enemy.h"

class BehaviorNode 
{
public:
    virtual bool Execute(void) = 0;

protected:
    EnemyAttributes& GetEnemyAttributes(Enemy* enemy)
    {
        return enemy->m_enemyAttr;
    }

    DebugProc& debugProc = DebugProc::get_instance();
};

// ======= 選択ノード（Selector） =======
class SelectorNode : public BehaviorNode 
{
public:
    SimpleArray<BehaviorNode*> children;

    void AddChild(BehaviorNode* child) 
    {
        children.push_back(child);
    }

    bool Execute(void) override
    {
        for (UINT i = 0; i < children.getSize(); i++) 
        {
            if (children[i]->Execute())
            {
                return true;  // どれか成功したら、それを選択する
            }
        }
        return false;  // 全ての子が失敗した場合、失敗を返す
    }
};

// ======= 順序ノード（Sequence） =======
class SequenceNode : public BehaviorNode
{
public:
    SimpleArray<BehaviorNode*> children;

    void AddChild(BehaviorNode* child) 
    {
        children.push_back(child);
    }

    bool Execute(void) override 
    {
        for (UINT i = 0; i < children.getSize(); i++)
        {
            if (!children[i]->Execute()) 
            {
                return false;  // どれか失敗したら、全体も失敗
            }
        }
        return true;  // 全て成功なら、成功を返す
    }
};

// ======= 冷却状態をチェックするノード =======
class CheckCooldown : public BehaviorNode
{
    Enemy* enemy;
public:
    CheckCooldown(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(enemy).isInCooldown;
    }
};

// ======= クールダウン中の移動ノード =======
class PerformCooldownMovement : public BehaviorNode
{
    Enemy* enemy;
public:
    PerformCooldownMovement(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        bool nextMove = false;

        debugProc.PrintDebugProc("%d\n", static_cast<int>(GetEnemyAttributes(enemy).cooldownState));

        switch (GetEnemyAttributes(enemy).cooldownState)
        {
        case CooldownState::MOVE:
            GetEnemyAttributes(enemy).cooldownMoveTimer -= enemy->timer.GetScaledDeltaTime();
            if (GetEnemyAttributes(enemy).cooldownMoveTimer <= 0.0f)
            {
                GetEnemyAttributes(enemy).cooldownMoveTimer = 0.0f;
                nextMove = true;
            }
            else
                enemy->CooldownMove();
            break;
        case CooldownState::WAIT:
            GetEnemyAttributes(enemy).cooldownWaitTimer -= enemy->timer.GetScaledDeltaTime();
            if (GetEnemyAttributes(enemy).cooldownWaitTimer <= 0.0f)
            {
                GetEnemyAttributes(enemy).cooldownWaitTimer = 0.0f;
                nextMove = true;
            }
            else
                enemy->CooldownWait();
            break;
        default:
            break;
        }
        
        if (nextMove)
        {
            float randValue = GetRandFloat(0.0f, 1.0f);
            if (randValue < GetEnemyAttributes(enemy).cooldownProbability)
            {
                GetEnemyAttributes(enemy).cooldownState = CooldownState::MOVE;
                GetEnemyAttributes(enemy).cooldownMoveDirection = (GetRand(0, 1) == 0) ? -1.0f : 1.0f;
                GetEnemyAttributes(enemy).cooldownMoveTimer = GetEnemyAttributes(enemy).attackCooldownTimer *
                    GetRandFloat(HILI_MIN_COOLDOWN_MOVE_TIME, HILI_MAX_COOLDOWN_MOVE_TIME);

                GetEnemyAttributes(enemy).cooldownProbability += 0.2f;
            }
            else
            {
                GetEnemyAttributes(enemy).cooldownState = CooldownState::WAIT;
                GetEnemyAttributes(enemy).cooldownWaitTimer = GetEnemyAttributes(enemy).attackCooldownTimer *
                    GetRandFloat(HILI_MIN_COOLDOWN_WAIT_TIME, HILI_MAX_COOLDOWN_WAIT_TIME);

                GetEnemyAttributes(enemy).cooldownProbability -= 0.2f;
            }
        }


        GetEnemyAttributes(enemy).attackCooldownTimer -= enemy->timer.GetScaledDeltaTime();
        if (GetEnemyAttributes(enemy).attackCooldownTimer <= 0.0f)
        {
            GetEnemyAttributes(enemy).isInCooldown = false;
            GetEnemyAttributes(enemy).fixedDirMove = false;
        }
        return true;
    }
};

// ======= プレイヤー発見をチェックするノード =======
class CheckDetectPlayer : public BehaviorNode
{
    Enemy* enemy;
public:
    CheckDetectPlayer(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        return enemy->DetectPlayer();
    }
};

// ======= 敵の状態を更新するノード（驚き、追跡） =======
class UpdateEnemyState : public BehaviorNode
{
    Enemy* enemy;
public:
    UpdateEnemyState(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        if (!GetEnemyAttributes(enemy).isSitting) 
        {

            if (!GetEnemyAttributes(enemy).isChasingPlayer)
            {
                GetEnemyAttributes(enemy).isSurprised = true;
            }
            if (!GetEnemyAttributes(enemy).isSurprised) 
            {
                GetEnemyAttributes(enemy).isChasingPlayer = true;
            }
        }
        return true;
    }
};

// ======= 追跡状態をチェックするノード =======
class CheckChasingPlayer : public BehaviorNode
{
    Enemy* enemy;
public:
    CheckChasingPlayer(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(enemy).isChasingPlayer || GetEnemyAttributes(enemy).isSurprised;
    }
};

// ======= プレイヤーを追跡するノード =======
class MoveToPlayer : public BehaviorNode
{
    Enemy* enemy;
public:
    MoveToPlayer(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        enemy->ChasePlayer();
        return true;
    }
};

// ======= ランダム移動をチェックするノード =======
class CheckRandomMove : public BehaviorNode
{
    Enemy* enemy;
public:
    CheckRandomMove(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(enemy).randomMove;
    }
};

// ======= 巡回・待機ノード =======
class PatrolMovement : public BehaviorNode
{
    Enemy* enemy;
public:
    PatrolMovement(Enemy* e) : enemy(e) {}

    bool Execute(void) override 
    {
        enemy->Patrol();
        return true;
    }
};


class BehaviorTree 
{
protected:
    SelectorNode root;  // ルートノード（選択ノード）

public:
    BehaviorTree(Enemy* enemy) 
    {
        InitializeTree(enemy);
    }

    virtual void InitializeTree(Enemy* enemy) 
    {
        // ===== 冷却シーケンス =====
        SequenceNode* cooldownSequence = new SequenceNode();
        cooldownSequence->AddChild(new CheckCooldown(enemy));
        cooldownSequence->AddChild(new PerformCooldownMovement(enemy));

        // ===== プレイヤー発見シーケンス =====
        SequenceNode* detectAndChasePlayerSequence = new SequenceNode();
        detectAndChasePlayerSequence->AddChild(new CheckDetectPlayer(enemy));
        detectAndChasePlayerSequence->AddChild(new UpdateEnemyState(enemy));
        detectAndChasePlayerSequence->AddChild(new CheckChasingPlayer(enemy));
        detectAndChasePlayerSequence->AddChild(new MoveToPlayer(enemy));

        // ===== 巡回シーケンス（通常の移動） =====
        SequenceNode* randomMoveSequence = new SequenceNode();
        randomMoveSequence->AddChild(new CheckRandomMove(enemy));
        randomMoveSequence->AddChild(new PatrolMovement(enemy));

        root.AddChild(cooldownSequence);
        root.AddChild(detectAndChasePlayerSequence);
        root.AddChild(randomMoveSequence);
    }

    void RunBehaviorTree() 
    {
        root.Execute();
    }
};
