//=============================================================================
//
// エネミーの行動管理クラス [Enemy.h]
// Author : 
// 敵キャラのHP・移動・攻撃・クールダウンなどをぜんぶ面倒みるAI担当クラスですっ！
// ビヘイビアツリーを使って、驚いたり座ったり追いかけたり、いろんなモードで生きてます
// 
//=============================================================================
#pragma once
#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Core/Timer.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define ENEMY_HIT_WINDOW			(60)
#define ENEMY_RESPAWN_TIME			(2000.0f)

enum class CooldownState
{
	MOVE,
	WAIT,
};

enum class EnemyUIType
{
	HPGauge,
	HPGaugeCover,
};

enum class EnemyType
{
	Hilichurl,
};

//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct EnemyAttributes
{
	float				HP;
	float				maxHP;

	float				timer;

	float				moveDuration;	// 移動する時間
	float				moveTimer;		// 移動時間のカウンター
	float				waitTime;		// 待機時間
	float				distPlayerSq;

	bool				fixedDirMove;
	float				fixedDir;

	bool				isWaiting;
	bool				isSurprised;
	bool				isChasingPlayer;
	bool				isSitting;
	bool				isInCooldown;

	float				viewAngle;     // 視野角 (例: 60° = 60 * (PI / 180))
	float				viewDistance;  // 視認範囲
	float				chaseRange;    // 追跡範囲
	float				attackRange;   // 攻撃範囲
	float				attackCooldownTimer;
	float				attackCooldownMin;
	float				attackCooldownMax;

	float				cooldownWaitTimer;
	float				cooldownMoveDirection;
	float				cooldownOrbitRadius;
	float				cooldownMoveTimer;

	float				cooldownProbability;
	CooldownState		cooldownState;


	EnemyState			initState;
	Transform			initTrans;

	EnemyType			enemyType;

	bool				die;
	bool				isDead;
	bool				startFadeOut;
	bool				randomMove;
	float				respawnTimer;

	void Initialize()
	{
		isWaiting = false;
		isSurprised = false;
		isChasingPlayer = false;
		isSitting = false;
		isInCooldown = false;
		die = false;
		isDead = false;
		startFadeOut = false;
		randomMove = true;

		fixedDirMove = false;
		fixedDir = 0.0f;

		timer = 0.0f;
		moveDuration = 0.0f;
		moveTimer = 0.0f;
		distPlayerSq = 0.0f;
		waitTime = 0.0f;
		cooldownMoveDirection = 0.0f;
		cooldownWaitTimer = 0.0f;
		cooldownMoveTimer = 0.0f;
		cooldownOrbitRadius = 0.0f;
		cooldownProbability = 0.5f;
	}
};

struct UISprite
{
	XMFLOAT3	pos;			// 位置
	XMFLOAT3	scl;			// スケール
	XMMATRIX	rot;
	MATERIAL	material;		// マテリアル
	float		fWidth;			// 幅
	float		fHeight;		// 高さ
	BOOL		bUse;			// 使用しているかどうか

};

struct MoveTable
{
	INTERPOLATION_DATA* moveTblAddr;
	int size;
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************

// 前方宣言
class Player;
class BehaviorTree;
class BehaviorNode;

class Enemy : public GameObject<SkinnedMeshModelInstance>
{
public:

	friend class BehaviorNode;

	Enemy(EnemyType enemyType, Transform trans);
	~Enemy();

	inline const EnemyAttributes& GetEnemyAttribute(void) { return m_enemyAttr; }
	void Update(void) override;
	void Draw(void) override;
	void DrawUI(EnemyUIType type);

	void SetPlayer(const Player* player) { m_player = player; }

	void ReduceHP(float amount) { m_enemyAttr.HP -= amount; }

	void SetRandomMove(bool random) { m_enemyAttr.randomMove = random; }

	void InitHPGauge(void);
	void UpdateHPGauge(void);

	void ChasePlayer(void);
	void AttackPlayer(void);
	void Patrol(void);
	void CooldownMove(void);
	void CooldownWait(void);
	bool DetectPlayer(void);
	bool CheckAvailableToMove(void);

protected:
	EnemyAttributes m_enemyAttr;
	const Player* m_player;
	virtual void Initialize(void);
private:
	void SetNewPosTarget(void);
	void StartWaiting(void);

	bool UpdateAliveState(void);

	HRESULT MakeVertexHPGauge(float w, float h);
	void DrawHPGauge(void);
	void DrawHPGaugeCover(void);

	BehaviorTree*				m_behaviorTree;  // 敵AIの行動ツリー

	// テクスチャ情報
	ID3D11ShaderResourceView*	m_HPGaugeTex = nullptr;
	ID3D11ShaderResourceView*	m_HPGaugeCoverTex = nullptr;

	// 頂点バッファ
	ID3D11Buffer*				m_HPGaugeVertexBuffer = nullptr;
	ID3D11Buffer*				m_HPGaugeCoverVertexBuffer = nullptr;
	UISprite					m_HPGauge;

	//void UpdateEditorSelect(int sx, int sy);
};