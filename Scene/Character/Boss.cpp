//=============================================================================
//
// Boss処理 [Boss.cpp]
// Author : 
//
//=============================================================================
#include "Scene/Character/Boss.h"
#include "Utility/InputManager.h"
#include "Scene/Player.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define PLAY_ANIM_SPD				1.0f
#define ANIM_BLEND_SPD				0.032f

#define WEAPON_SIZE						35.0f
#define WEAPON_ON_HANDS_POS_OFFSET_X	-55.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Y	45.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Z	-35.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_X	0.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_Y	XM_PI * 0.5f
#define WEAPON_ON_HANDS_ROT_OFFSET_Z	XM_PI * 0.5f

Boss::Boss(Transform transform, EnemyState initState):Enemy(EnemyType::Boss, transform)
{
	SkinnedGameObjectConfig config;
	config.modelPath = "data/MODEL/enemy/Boss";
	config.modelName = "Character_output.fbx";
	config.modelType = SkinnedModelType::Boss;
	config.drawWorldAABB = false;
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ENEMY;
	Instantiate(config);

	m_initState = initState;
	m_playAnimSpeed = PLAY_ANIM_SPD;
}

void Boss::Initialize(void)
{
	Enemy::Initialize();
	if (instance.pModel)
		instance.pModel->ResetCurrentAnim();
	if (m_stateMachine)
		m_stateMachine->SetCurrentState(STATE(m_enemyAttr.initState));
}

Boss::~Boss()
{
	SAFE_DELETE(m_stateMachine);
}

void Boss::PostInit(void)
{
	instance.transform.pos = m_enemyAttr.initTrans.pos;
	instance.transform.rot = m_enemyAttr.initTrans.rot;
	instance.transform.scl = m_enemyAttr.initTrans.scl;

	InitHPGauge();

	// 遅延用コンテキスト構築
	auto* ctx = new InitContext{
		this,
	};
	DeferredTaskOptions options{};
	options.async = true; // 非同期実行
	options.debugName = "AsyncFbxModelLoader_InitializeAnim_Boss"; // デバッグ用の名前
	// タスクを DeferredTaskSystem に登録
	RawDeferredTask::Add(
		ctx,
		[](void* ptr)
		{
			auto* ctx = static_cast<InitContext*>(ptr);
			ctx->owner->InitializeAnimation();

			ctx->owner->GameObject::PostInit();
		},
		nullptr,
		options
	);

	InitializeBoneHitCollider(
		m_attackCollider, 
		XMFLOAT3(0.5f, 0.7f, 0.5f), 
		XMFLOAT3(-50.0f, 50.0f, -10.0f), 
		ColliderTag::ENEMY_ATTACK
	);
}

void Boss::InitializeAnimation(void)
{
	AddAnimation("data/MODEL/enemy/Boss/", "Idle.fbx", AnimClipName::ANIM_IDLE);
	AddAnimation("data/MODEL/enemy/Boss/", "Walking.fbx", AnimClipName::ANIM_WALK);
	AddAnimation("data/MODEL/enemy/Boss/", "Standing Melee Attack 360 High.fbx", AnimClipName::ANIM_STANDING_MELEE_ATTACK, AnimPlayMode::ONCE);
	AddAnimation("data/MODEL/enemy/Boss/", "Sword And Shield Death.fbx", AnimClipName::ANIM_DIE, AnimPlayMode::ONCE);

	SetupAnimStateMachine(m_initState);
	InitAnimInfo();
}

void Boss::AddAnimation(char* animPath, char* animName, AnimClipName clipName, AnimPlayMode animPlayMode)
{
	if (instance.pModel)
		m_fbxLoader.LoadAnimation(instance.pModel, instance.pModel->GetModelData(), animPath, animName, clipName, animPlayMode);
}

void Boss::LoadWeapon(char* modelPath, char* modelName)
{
	SkinnedGameObjectConfig config;
	config.modelPath = modelPath;
	config.modelName = modelName;
	config.modelType = SkinnedModelType::Weapon;
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ENEMY_ATTACK;
	config.scale = XMFLOAT3(WEAPON_SIZE, WEAPON_SIZE, WEAPON_SIZE);
	config.position = XMFLOAT3(WEAPON_ON_HANDS_POS_OFFSET_X, WEAPON_ON_HANDS_POS_OFFSET_Y, WEAPON_ON_HANDS_POS_OFFSET_Z);
	config.rotation = XMFLOAT3(WEAPON_ON_HANDS_ROT_OFFSET_X, WEAPON_ON_HANDS_ROT_OFFSET_Y, WEAPON_ON_HANDS_ROT_OFFSET_Z);
	m_weapon.Instantiate(config);
}

void Boss::Update(void)
{
	Enemy::Update();

	if (!instance.load || !m_stateMachine) return;

	m_stateMachine->Update(ANIM_BLEND_SPD, dynamic_cast<ISkinnedMeshModelChar*>(this));

	uint64_t state = m_stateMachine->GetCurrentState();
	if (state != STATE(EnemyState::ATTACK))
		m_attackCollider.collider.enable = false;

	float animSpeed = m_playAnimSpeed * m_timer.GetScaledDeltaTime();
	switch (m_stateMachine->GetCurrentState())
	{
	case STATE(EnemyState::IDLE):
		PlayGeneralAnim(AnimClipName::ANIM_IDLE, animSpeed);
		break;
	case STATE(EnemyState::WALK):
		PlayGeneralAnim(AnimClipName::ANIM_WALK, animSpeed);
		break;
	case STATE(EnemyState::RUN):
		PlayGeneralAnim(AnimClipName::ANIM_RUN, animSpeed);
		break;
	case STATE(EnemyState::ATTACK):
		PlayAttackAnim();
		break;
	case STATE(EnemyState::HIT1):
		PlayGeneralAnim(AnimClipName::ANIM_HIT_REACTION_1, animSpeed);
		break;
	case STATE(EnemyState::HIT2):
		PlayGeneralAnim(AnimClipName::ANIM_HIT_REACTION_2, animSpeed);
		break;
	case STATE(EnemyState::SIT):
		PlayGeneralAnim(AnimClipName::ANIM_SIT, animSpeed);
		break;
	case STATE(EnemyState::SURPRISED):
		PlayGeneralAnim(AnimClipName::ANIM_SURPRISED, animSpeed * 1.2f);
		break;
	case STATE(EnemyState::DIE):
		PlayGeneralAnim(AnimClipName::ANIM_DIE, animSpeed);
		break;
	default:
		break;
	}

	instance.pModel->UpdateBoneTransform(m_stateMachine->GetBoneMatrices());

	//UpdateWeapon();
}

void Boss::Draw(void)
{
	Enemy::Draw();
	//m_weapon.Draw();
}

void Boss::InitAnimInfo(void)
{
	AnimationClip* attack1 = instance.pModel->GetAnimationClip(AnimClipName::ANIM_STANDING_MELEE_ATTACK);
	if (attack1)
	{
		attack1->animInfo.animPhase.startMoveFraction = 0.0f;
		attack1->animInfo.animPhase.endMoveFraction = 0.3f;
		attack1->animInfo.animPhase.startAttackFraction = 0.3f;
		attack1->animInfo.animPhase.endAttackFraction = 0.6f;
	}
}

void Boss::UpdateWeapon(void)
{
	if (m_enemyAttr.isDead)
		m_weapon.SetCastShadow(false);
	else
		m_weapon.SetCastShadow(true);

	m_weapon.Update();
	XMMATRIX weaponMtx = instance.pModel->GetBodyTransformMtx(BodyTransformIndex::RightHand);
	weaponMtx = XMMatrixMultiply(m_weapon.GetWorldMatrix(), weaponMtx);
	weaponMtx = XMMatrixMultiply(weaponMtx, instance.transform.mtxWorld);

	m_weapon.SetWorldMatrix(weaponMtx);

	// ローカル空間のAABBを取得
	BOUNDING_BOX weaponBB = m_weapon.GetSkinnedMeshModel()->GetBoundingBox();
	XMFLOAT3 localMin = weaponBB.minPoint;
	XMFLOAT3 localMax = weaponBB.maxPoint;

	// ローカルAABBの8頂点を生成
	XMVECTOR localCorners[8] = {
		XMVectorSet(localMin.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMax.z, 1.0f),
	};

	// 武器の変換行列（アニメーション＋ワールド行列）
	weaponMtx = m_weapon.GetWorldMatrix(); // ※ここがボーン変形やアタッチ先の変換を含むこと！

	// AABB初期化（最大／最小を初期値に設定）
	XMVECTOR worldMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
	XMVECTOR worldMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);

	// 各頂点を変換してAABB範囲を更新
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR transformed = XMVector3TransformCoord(localCorners[i], weaponMtx);
		worldMin = XMVectorMin(worldMin, transformed);
		worldMax = XMVectorMax(worldMax, transformed);
	}

	// ワールド空間のAABBを格納
	XMStoreFloat3(&weaponBB.minPoint, worldMin);
	XMStoreFloat3(&weaponBB.maxPoint, worldMax);

	// コライダーに反映
	m_weapon.SetColliderBoundingBox(weaponBB);
}

AnimStateMachine* Boss::GetStateMachine(void)
{
	return m_stateMachine;
}

void Boss::PlayGeneralAnim(AnimClipName clipName, float speed)
{
	if (instance.pModel->GetCurrentAnim() != clipName)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(speed);
}

void Boss::SetupAnimStateMachine(EnemyState initState)
{
	m_stateMachine = new AnimStateMachine(dynamic_cast<ISkinnedMeshModelChar*>(this));

	m_stateMachine->AddState(STATE(EnemyState::IDLE), instance.pModel->GetAnimationClip(AnimClipName::ANIM_IDLE));
	m_stateMachine->AddState(STATE(EnemyState::ATTACK), instance.pModel->GetAnimationClip(AnimClipName::ANIM_STANDING_MELEE_ATTACK));
	m_stateMachine->AddState(STATE(EnemyState::WALK), instance.pModel->GetAnimationClip(AnimClipName::ANIM_WALK));
	instance.pModel->GetAnimationClip(AnimClipName::ANIM_DIE)->SetAnimPlayMode(AnimPlayMode::ONCE);
	m_stateMachine->AddState(STATE(EnemyState::DIE), instance.pModel->GetAnimationClip(AnimClipName::ANIM_DIE));

	//状態遷移
	m_stateMachine->AddTransition(STATE(EnemyState::IDLE), STATE(EnemyState::WALK), &ISkinnedMeshModelChar::CanWalk);
	m_stateMachine->AddTransition(STATE(EnemyState::IDLE), STATE(EnemyState::ATTACK), &ISkinnedMeshModelChar::CanAttack);
	m_stateMachine->AddTransition(STATE(EnemyState::IDLE), STATE(EnemyState::DIE), &ISkinnedMeshModelChar::CanDie);
	m_stateMachine->AddTransition(STATE(EnemyState::WALK), STATE(EnemyState::IDLE), &ISkinnedMeshModelChar::CanStopMoving);
	m_stateMachine->AddTransition(STATE(EnemyState::WALK), STATE(EnemyState::ATTACK), &ISkinnedMeshModelChar::CanAttack);
	m_stateMachine->AddTransition(STATE(EnemyState::WALK), STATE(EnemyState::DIE), &ISkinnedMeshModelChar::CanDie);
	m_stateMachine->AddTransition(STATE(EnemyState::ATTACK), STATE(EnemyState::IDLE), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(EnemyState::ATTACK), STATE(EnemyState::DIE), &ISkinnedMeshModelChar::CanDie);


	m_stateMachine->SetEndCallback(STATE(EnemyState::ATTACK), &ISkinnedMeshModelChar::OnAttackAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(EnemyState::DIE), &ISkinnedMeshModelChar::OnDieAnimationEnd);
	

	m_enemyAttr.initState = initState;
	m_stateMachine->SetCurrentState(STATE(initState));
}

void Boss::PlayAttackAnim(void)
{
	float curAnimTime = m_stateMachine->GetCurrentAnimTime();
	if (curAnimTime >= m_stateMachine->GetCurrentAnimClip()->animInfo.animPhase.startAttackFraction
		&& curAnimTime <= m_stateMachine->GetCurrentAnimClip()->animInfo.animPhase.endAttackFraction)
	{
		m_weapon.SetColliderEnable(true);
	}
	else
	{
		m_weapon.SetColliderEnable(false);
	}

	float dist = m_enemyAttr.distPlayerSq;

	if (dist > m_enemyAttr.attackRange * m_enemyAttr.attackRange)
	{
		const AnimationClip* currentAnimClip = m_stateMachine->GetCurrentAnimClip();

		// アニメーションの再生進捗を取得
		float attackAnimTime = m_stateMachine->GetCurrentAnimTime();
		// 最大移動距離
		float moveStep = min(dist - m_enemyAttr.attackRange, HILI_MAX_ATTACK_STEP);
		// どのくらいの割合の時間が移動に使われるか計算
		float moveTimeFraction = currentAnimClip->animInfo.animPhase.endMoveFraction - currentAnimClip->animInfo.animPhase.startMoveFraction;

		// `num_steps` を自動調整
		int num_steps = max(50, min(200, (int)(moveStep / 5.0f)));

		// `sum_speed_factors` を数値積分で計算
		float sum_speed_factors = 0.0f;
		for (int i = 0; i <= num_steps; i++)
		{
			float phase = (float)i / num_steps;
			sum_speed_factors += phase * (2.0f - phase);
		}

		// MAX_SPEED を計算
		float MAX_SPEED = moveStep / sum_speed_factors;

		// 現在の移動フェーズの正規化値を計算
		float attackPhase = (attackAnimTime - currentAnimClip->animInfo.animPhase.startMoveFraction) / moveTimeFraction;

		// 指定された時間範囲内で移動
		if (attackAnimTime >= currentAnimClip->animInfo.animPhase.startMoveFraction
			&& attackAnimTime <= currentAnimClip->animInfo.animPhase.endMoveFraction)
		{
			// 速度補正係数を計算 (S 曲線補間)
			float speedFactor = attackPhase * (2.0f - attackPhase);
			instance.attributes.spd = MAX_SPEED * speedFactor;

			// すでに攻撃範囲内にいる場合、速度を急激に減衰させる
			if (dist <= m_enemyAttr.attackRange * m_enemyAttr.attackRange)
			{
				instance.attributes.spd *= (1.0f - attackPhase * attackPhase); // 速度を急激に減少
			}
		}
		else
		{
			instance.attributes.spd = 0.0f; // 移動を停止
		}
	}

	AnimationClip* clip = m_stateMachine->GetCurrentAnimClip();
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_STANDING_MELEE_ATTACK)
		instance.pModel->SetCurrentAnim(clip);
	instance.pModel->PlayCurrentAnim(m_playAnimSpeed);

	if (clip->GetCurrentPlayTime() > 0.1f && clip->GetCurrentPlayTime() < 0.9f)
		m_attackCollider.collider.enable = true;
	else
		m_attackCollider.collider.enable = false;

	UpdateBoneHitCollider(m_attackCollider, BodyTransformIndex::RightHand);

}

bool Boss::CanWalk(void) const
{
	return (instance.attributes.isMoving || instance.attributes.isRotating) && !instance.attributes.isAttacking;
}

bool Boss::CanStopMoving() const
{
	return !instance.attributes.isMoving;
}

bool Boss::CanStopRunning() const
{
	return !instance.attributes.isRunning && instance.attributes.isMoving;
}

bool Boss::CanAttack() const
{
	return instance.attributes.isAttacking;
}

bool Boss::CanRun(void) const
{
	return instance.attributes.isRunning;
}

bool Boss::CanHit(void) const
{
	return instance.attributes.isHit1;
}

bool Boss::CanHit2(void) const
{
	return instance.attributes.isHit2;
}

bool Boss::CanSurprised(void) const
{
	return m_enemyAttr.isSurprised;
}

bool Boss::CanDie(void) const
{
	return m_enemyAttr.die;
}

void Boss::OnAttackAnimationEnd(void)
{
	m_weapon.SetColliderEnable(false);

	instance.attributes.isAttacking = false;

	m_enemyAttr.isInCooldown = true;
	m_enemyAttr.attackCooldownTimer = GetRandFloat(HILI_MIN_ATTACK_CDTIME, HILI_MAX_ATTACK_CDTIME);
	m_enemyAttr.cooldownProbability = 0.5f;
	instance.attributes.isMoving = false;
}

void Boss::OnHitAnimationEnd(void)
{
	instance.attributes.isHit1 = false;
	instance.attributes.isHit2 = false;
	instance.attributes.hitTimer = 0;

	m_enemyAttr.isChasingPlayer = true;
}

void Boss::OnSurprisedEnd(void)
{
	m_enemyAttr.isSurprised = false;
	m_enemyAttr.isChasingPlayer = true;
	m_enemyAttr.randomMove = true;
}

void Boss::OnDieAnimationEnd()
{
	m_enemyAttr.startFadeOut = true;
}

void Boss::RenderDebugInfo(void)
{
	GameObject::RenderDebugInfo();

#if DRAW_BOUNDING_BOX
	if (m_attackCollider.collider.enable)
	{
		XMFLOAT4 color = { 1.0f, 0.0f, 0.0f, 1.0f };
		m_debugBoundingBoxRenderer.DrawBox(m_attackCollider.collider.aabb, Camera::get_instance().GetViewProjMtx(), color);
	}
#endif
}
