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
            GetEnemyAttributes(m_enemy).cooldownMoveTimer -= m_enemy->m_timer.GetScaledDeltaTime();
            if (GetEnemyAttributes(m_enemy).cooldownMoveTimer <= 0.0f)
            {
                GetEnemyAttributes(m_enemy).cooldownMoveTimer = 0.0f;
                nextMove = true;
            }
            else
                m_enemy->CooldownMove();
            break;
        case CooldownState::WAIT:
            GetEnemyAttributes(m_enemy).cooldownWaitTimer -= m_enemy->m_timer.GetScaledDeltaTime();
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


        GetEnemyAttributes(m_enemy).attackCooldownTimer -= m_enemy->m_timer.GetScaledDeltaTime();
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
        m_enemy->ChaseAndAttackPlayer();
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

class CheckBossPhase : public BehaviorNode 
{
public:
    CheckBossPhase(Boss* boss) :m_boss(boss) {}
    bool Execute() override 
    {
        auto& attributes = GetEnemyAttributes(m_boss);
        if (attributes.HP <= attributes.maxHP * 0.6f && m_boss->GetBossPhase() == 1)
        {
            return true;
        }
        return false;
    }
private: Boss* m_boss;
};

class EnterPhase2 : public BehaviorNode 
{
public:
    EnterPhase2(Boss* boss) :m_boss(boss) {}
    bool Execute() override 
    {
        m_boss->EnterPhase2();
        return true;
    }
private: Boss* m_boss;
};

class CheckPhaseExecute : public BehaviorNode 
{
public:
    CheckPhaseExecute(Boss* boss, int phase, BehaviorNode* node) :m_boss(boss), m_phase(phase), m_node(node) {}
    bool Execute() override 
    {
        if (m_boss->GetBossPhase() == m_phase)
        {
            return m_node->Execute();
        }
        return false;
    }
private: 
    Boss* m_boss;
    int m_phase; 
    BehaviorNode* m_node;
};

// ======= 攻撃ノード（共通化） =======
class BossPerformAttack : public BehaviorNode 
{
public:
    BossPerformAttack(Boss* boss, const BossAttackPattern pattern) : m_boss(boss), m_attackPattern(pattern) {}
    bool Execute() override 
    {
        if (!m_boss->CanActNow()) return false;
        return m_boss->PerformAttack(m_attackPattern); // 攻撃アニメーション終了で true
    }
private:
    Boss* m_boss;
    BossAttackPattern m_attackPattern;
};

// ボス：近接攻撃ノード
class BossMeleeAttack : public BehaviorNode 
{
public:
    BossMeleeAttack(Boss* boss, float attackRange, float chaseTimeout)
        : m_boss(boss), m_attackRange(attackRange), m_chaseTimeout(chaseTimeout) {}

    bool Execute() override 
    {
        if (!m_boss->CanActNow()) return false;

        float dist = m_boss->DistanceToPlayer();

        // 攻撃範囲に入った → 近接攻撃
        if (dist <= m_attackRange) 
        {

            bool finished = m_boss->PerformAttack(BossAttackPattern::MeleeAttackCombo2);
            if (finished) 
            {
                m_boss->AddConsecutiveMeleeCount();
                m_boss->SetLastMeleeTime(Timer::get_instance().GetDeltaTime());

                // 近接回数が閾値を超えた → 遠距離へ切替
                if (m_boss->GetConsecutiveMeleeCount() >= 3) 
                {
                    m_boss->SetConsecutiveMeleeCount(0);
                    return false; // このノード失敗 → Selector が次のノード（魔法攻撃）に移行
                }
            }
            return finished;
        }

        // 攻撃範囲外 → 追跡
        m_boss->ChasePlayer();
        m_elapsed += Timer::get_instance().GetDeltaTime();
        if (m_elapsed > m_chaseTimeout) 
        {
            m_elapsed = 0.0f;
            return false; // fallback → 魔法
        }
        return true;
    }

private:
    Boss* m_boss;
    float m_attackRange;
    float m_chaseTimeout;
    float m_elapsed = 0.0f;
};

// ボス：魔法攻撃ノード
class BossMagicAttack : public BehaviorNode 
{
public:
    BossMagicAttack(Boss* boss, float safeDistance)
        : m_boss(boss), m_safeDistance(safeDistance) {}

    bool Execute() override 
    {
        if (!m_boss->CanActNow()) return false;

        float dist = m_boss->DistanceToPlayer();

        // プレイヤーが近すぎる場合 → 後退してから魔法
        if (dist < m_safeDistance) 
        {
            if (m_boss->PerformAttack(BossAttackPattern::BackStep)) return false; // 後退中
        }

        bool finished = m_boss->PerformAttack(BossAttackPattern::IceShard);
        if (finished) 
        {
            m_boss->SetConsecutiveMeleeCount(0); // 魔法を撃ったら近接カウンタリセット
        }
        return finished;
    }

private:
    Boss* m_boss;
    float m_safeDistance;
};

// ボス：近接攻撃（フェーズ2用 3連撃）
class BossMeleeAttackPhase2 : public BehaviorNode
{
public:
    BossMeleeAttackPhase2(Boss* boss, float attackRange, float chaseTimeout)
        : m_boss(boss), m_attackRange(attackRange), m_chaseTimeout(chaseTimeout) {}

    bool Execute() override
    {
        if (!m_boss->CanActNow()) return false;

        float dist = m_boss->DistanceToPlayer();

        // 攻撃範囲に入った → 近接攻撃（3連撃）
        if (dist <= m_attackRange)
        {
            bool finished = m_boss->PerformAttack(BossAttackPattern::MeleeAttackCombo3);
            if (finished)
            {
                m_boss->AddConsecutiveMeleeCount();
                m_boss->SetLastMeleeTime(Timer::get_instance().GetDeltaTime());

                // 近接回数が閾値を超えた → 遠距離へ切替
                if (m_boss->GetConsecutiveMeleeCount() >= 3)
                {
                    m_boss->SetConsecutiveMeleeCount(0);
                    return false; // このノード失敗 → Selector が次のノード（遠距離攻撃）に移行
                }
            }
            return finished;
        }

        // 攻撃範囲外 → 追跡
        m_boss->ChasePlayer();
        m_elapsed += Timer::get_instance().GetDeltaTime();
        if (m_elapsed > m_chaseTimeout)
        {
            m_elapsed = 0.0f;
            return false; // fallback → 遠距離攻撃
        }
        return true;
    }

private:
    Boss* m_boss;
    float m_attackRange;
    float m_chaseTimeout;
    float m_elapsed = 0.0f;
};

// ボス：遠距離攻撃（フェーズ2用：魔法 or レーザー）
class BossRangedAttackPhase2 : public BehaviorNode
{
public:
    BossRangedAttackPhase2(Boss* boss, float safeDistance)
        : m_boss(boss), m_safeDistance(safeDistance) {}

    bool Execute() override
    {
        if (!m_boss->CanActNow()) return false;

        float dist = m_boss->DistanceToPlayer();

        // プレイヤーが近すぎる場合 → 後退してから遠距離
        if (dist < m_safeDistance)
        {
            if (m_boss->PerformAttack(BossAttackPattern::BackStep)) return false; // 後退中
        }

        // 遠距離攻撃を選択（確率で魔法かレーザー）
        float randVal = GetRandFloat(0.0f, 1.0f);
        BossAttackPattern chosenAttack = (randVal < 0.3f)
            ? BossAttackPattern::IceShard   // 30% 魔法
            : BossAttackPattern::Laser; // 70% レーザー

        bool finished = m_boss->PerformAttack(chosenAttack);
        if (finished)
        {
            m_boss->SetConsecutiveMeleeCount(0); // 遠距離を撃ったら近接カウンタリセット
        }
        return finished;
    }

private:
    Boss* m_boss;
    float m_safeDistance;
};

// ボス：突進ノード
class BossDashAttack : public BehaviorNode 
{
public:
    BossDashAttack(Boss* boss, float dashThreshold, float dashTimeout)
        : m_boss(boss), m_dashThreshold(dashThreshold), m_dashTimeout(dashTimeout) {}

    bool Execute() override 
    {
        float dist = m_boss->DistanceToPlayer();

        // 距離が閾値を超えた場合 → タイマー開始
        if (dist > m_dashThreshold) 
        {
			m_elapsed += Timer::get_instance().GetDeltaTime();

            // アクション可能チェック
            if (!m_boss->CanActNow())
                return false;

            if (m_elapsed > m_dashTimeout) 
            {
                if (m_boss->PerformAttack(BossAttackPattern::DashToPlayer)) return false; // 突進中
                m_elapsed = 0.0f;
                return true; // 突進完了
            }
        }
        else 
        {
            m_elapsed = 0.0f; // リセット
        }
        return false;
    }

private:
    Boss* m_boss;
    float m_dashThreshold;
    float m_dashTimeout;
    float m_elapsed = 0.0f;
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
		if (enemy->GetEnemyAttribute().enemyType == EnemyType::Boss)
		{
			Boss* boss = static_cast<Boss*>(enemy);

            SelectorNode* rootSelector = new SelectorNode();

            // ===== 冷却シーケンス =====
            SequenceNode* cooldownSequence = new SequenceNode();
            cooldownSequence->AddChild(new CheckCooldown(boss));
            cooldownSequence->AddChild(new PerformCooldownMovement(boss));
            rootSelector->AddChild(cooldownSequence);

            // ===== フェーズ切替 =====
            SequenceNode* phaseChange = new SequenceNode();
            phaseChange->AddChild(new CheckBossPhase(boss));
            phaseChange->AddChild(new EnterPhase2(boss));
			rootSelector->AddChild(phaseChange);

            // ===== フェーズ1（HP > 60%） =====
            SelectorNode* phase1Behavior = new SelectorNode();

			// 突進 → 近接 or 魔法
            phase1Behavior->AddChild(new BossDashAttack(boss, /*dashThreshold*/700.0f, /*dashTimeout*/8.0f));

            // 近接攻撃（追跡＋近接 or 魔法fallback）
            phase1Behavior->AddChild(new BossMeleeAttack(boss, /*attackRange*/350.0f, /*chaseTimeout*/4.0f));

            // 魔法攻撃（通常選択肢）
            phase1Behavior->AddChild(new BossMagicAttack(boss, /*safeDistance*/700.0f));
            rootSelector->AddChild(new CheckPhaseExecute(boss, 1, phase1Behavior));

            // ===== フェーズ2（HP <= 60%） =====
            SelectorNode* phase2Behavior = new SelectorNode();

			// 突進 → 3連撃 or 遠距離技
            phase1Behavior->AddChild(new BossDashAttack(boss, /*dashThreshold*/700.0f, /*dashTimeout*/6.0f));

            // 1. 近接（3連撃）
            phase2Behavior->AddChild(new BossMeleeAttackPhase2(boss, /*attackRange*/350.0f, /*chaseTimeout*/3.0f));

            // 2. 遠距離（魔法 or レーザー）
            phase2Behavior->AddChild(new BossRangedAttackPhase2(boss, /*safeDistance*/700.0f));

            rootSelector->AddChild(new CheckPhaseExecute(boss, 2, phase2Behavior));

            m_root.AddChild(rootSelector);
		}
		else
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

    }

    void RunBehaviorTree() 
    {
        m_root.Execute();
    }
};
