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

    DebugProc& m_debugProc = DebugProc::get_instance();
};

// ======= 選択ノード（Selector） =======
class SelectorNode : public BehaviorNode 
{
public:
    void AddChild(BehaviorNode* child) 
    {
        m_children.push_back(child);
    }

    bool Execute(void) override
    {
        for (UINT i = 0; i < m_children.getSize(); i++)
        {
            if (m_children[i]->Execute())
            {
                return true;  // どれか成功したら、それを選択する
            }
        }
        return false;  // 全ての子が失敗した場合、失敗を返す
    }

private:
    SimpleArray<BehaviorNode*> m_children;
};

// ======= 順序ノード（Sequence） =======
class SequenceNode : public BehaviorNode
{
public:
    void AddChild(BehaviorNode* child) 
    {
        m_children.push_back(child);
    }

    bool Execute(void) override 
    {
        for (UINT i = 0; i < m_children.getSize(); i++)
        {
            if (!m_children[i]->Execute())
            {
                return false;  // どれか失敗したら、全体も失敗
            }
        }
        return true;  // 全て成功なら、成功を返す
    }

private:
    SimpleArray<BehaviorNode*> m_children;
};

// ======= 冷却状態をチェックするノード =======
class CheckCooldown : public BehaviorNode
{
public:
    CheckCooldown(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(m_enemy).isInCooldown;
    }

private:
    Enemy* m_enemy;
};

// ======= クールダウン中の移動ノード =======
class PerformCooldownMovement : public BehaviorNode
{
public:
    PerformCooldownMovement(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        bool nextMove = false;

        switch (GetEnemyAttributes(m_enemy).cooldownState)
        {
        case CooldownState::MOVE:
            GetEnemyAttributes(m_enemy).cooldownMoveTimer -= m_enemy->timer.GetScaledDeltaTime();
            if (GetEnemyAttributes(m_enemy).cooldownMoveTimer <= 0.0f)
            {
                GetEnemyAttributes(m_enemy).cooldownMoveTimer = 0.0f;
                nextMove = true;
            }
            else
                m_enemy->CooldownMove();
            break;
        case CooldownState::WAIT:
            GetEnemyAttributes(m_enemy).cooldownWaitTimer -= m_enemy->timer.GetScaledDeltaTime();
            if (GetEnemyAttributes(m_enemy).cooldownWaitTimer <= 0.0f)
            {
                GetEnemyAttributes(m_enemy).cooldownWaitTimer = 0.0f;
                nextMove = true;
            }
            else
                m_enemy->CooldownWait();
            break;
        default:
            break;
        }
        
        if (nextMove)
        {
            float randValue = GetRandFloat(0.0f, 1.0f);
            if (randValue < GetEnemyAttributes(m_enemy).cooldownProbability)
            {
                GetEnemyAttributes(m_enemy).cooldownState = CooldownState::MOVE;
                GetEnemyAttributes(m_enemy).cooldownMoveDirection = (GetRand(0, 1) == 0) ? -1.0f : 1.0f;
                GetEnemyAttributes(m_enemy).cooldownMoveTimer = GetEnemyAttributes(m_enemy).attackCooldownTimer *
                    GetRandFloat(HILI_MIN_COOLDOWN_MOVE_TIME, HILI_MAX_COOLDOWN_MOVE_TIME);

                GetEnemyAttributes(m_enemy).cooldownProbability += 0.2f;
            }
            else
            {
                GetEnemyAttributes(m_enemy).cooldownState = CooldownState::WAIT;
                GetEnemyAttributes(m_enemy).cooldownWaitTimer = GetEnemyAttributes(m_enemy).attackCooldownTimer *
                    GetRandFloat(HILI_MIN_COOLDOWN_WAIT_TIME, HILI_MAX_COOLDOWN_WAIT_TIME);

                GetEnemyAttributes(m_enemy).cooldownProbability -= 0.2f;
            }
        }


        GetEnemyAttributes(m_enemy).attackCooldownTimer -= m_enemy->timer.GetScaledDeltaTime();
        if (GetEnemyAttributes(m_enemy).attackCooldownTimer <= 0.0f)
        {
            GetEnemyAttributes(m_enemy).isInCooldown = false;
            GetEnemyAttributes(m_enemy).fixedDirMove = false;
        }
        return true;
    }

private:
    Enemy* m_enemy;
};

// ======= プレイヤー発見をチェックするノード =======
class CheckDetectPlayer : public BehaviorNode
{
public:
    CheckDetectPlayer(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        return m_enemy->DetectPlayer();
    }

private:
    Enemy* m_enemy;
};

// ======= 敵の状態を更新するノード（驚き、追跡） =======
class UpdateEnemyState : public BehaviorNode
{
public:
    UpdateEnemyState(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        if (!GetEnemyAttributes(m_enemy).isSitting) 
        {

            if (!GetEnemyAttributes(m_enemy).isChasingPlayer)
            {
                GetEnemyAttributes(m_enemy).isSurprised = true;
            }
            if (!GetEnemyAttributes(m_enemy).isSurprised) 
            {
                GetEnemyAttributes(m_enemy).isChasingPlayer = true;
            }
        }
        return true;
    }

private:
    Enemy* m_enemy;
};

// ======= 追跡状態をチェックするノード =======
class CheckChasingPlayer : public BehaviorNode
{
public:
    CheckChasingPlayer(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(m_enemy).isChasingPlayer || GetEnemyAttributes(m_enemy).isSurprised;
    }

private:
    Enemy* m_enemy;
};

// ======= プレイヤーを追跡するノード =======
class MoveToPlayer : public BehaviorNode
{
public:
    MoveToPlayer(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        m_enemy->ChasePlayer();
        return true;
    }

private:
    Enemy* m_enemy;
};

// ======= ランダム移動をチェックするノード =======
class CheckRandomMove : public BehaviorNode
{
public:
    CheckRandomMove(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        return GetEnemyAttributes(m_enemy).randomMove;
    }

private:
    Enemy* m_enemy;
};

// ======= 巡回・待機ノード =======
class PatrolMovement : public BehaviorNode
{
public:
    PatrolMovement(Enemy* e) : m_enemy(e) {}

    bool Execute(void) override 
    {
        m_enemy->Patrol();
        return true;
    }

private:
    Enemy* m_enemy;
};


class BehaviorTree 
{
protected:
    SelectorNode m_root;  // ルートノード（選択ノード）

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

        m_root.AddChild(cooldownSequence);
        m_root.AddChild(detectAndChasePlayerSequence);
        m_root.AddChild(randomMoveSequence);
    }

    void RunBehaviorTree() 
    {
        m_root.Execute();
    }
};
