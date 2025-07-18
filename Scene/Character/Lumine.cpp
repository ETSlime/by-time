//=============================================================================
//
// Lumine処理 [Lumine.cpp]
// Author : 
//
//=============================================================================
#include "Scene/Character/Lumine.h"
#include "Utility/Debugproc.h"
#include "Scene/Player.h"
#include "Scene/EnemyManager.h"
#include "Effects/EffectSystem.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define PLAY_ANIM_SPD				1.0f
#define ANIM_BLEND_SPD				0.032f

#define WEAPON_SIZE						50.0f
#define WEAPON_ON_HANDS_POS_OFFSET_X	-55.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Y	95.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Z	-35.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_X	0.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_Y	XM_PI
#define WEAPON_ON_HANDS_ROT_OFFSET_Z	0.0f

#define GUN_SIZE			2.0f
#define GUN_POS_OFFSET_X	-41.0f
#define GUN_POS_OFFSET_Y	78.1f
#define GUN_POS_OFFSET_Z	-1.4f
#define GUN_ROT_OFFSET_X	-0.6f
#define GUN_ROT_OFFSET_Y	2.6f
#define GUN_ROT_OFFSET_Z	1.2f

#define WEAPON_ON_BACK_POS_OFFSET_X		0.0f
#define WEAPON_ON_BACK_POS_OFFSET_Y		95.0f
#define WEAPON_ON_BACK_POS_OFFSET_Z		25.0f
#define WEAPON_ON_BACK_ROT_OFFSET_X		XM_PI * 1.5f
#define WEAPON_ON_BACK_ROT_OFFSET_Y		0.0f
#define WEAPON_ON_BACK_ROT_OFFSET_Z		XM_PI

#define ATTACK_COMBO_WINDOW		(70)
#define ATTACK_RANGE			(120.0f)
#define	MAX_ATTACK_STEP			(800.0f)
#define WEAPON_ON_BACK_TIME		(600.0f)
#define WEAPON_ON_HAND_TIME		(200.0f)

Lumine::Lumine()
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // _DEBUG

	SkinnedGameObjectConfig config;
	config.modelPath = "data/MODEL/character/Lumine";
	config.modelName = "Character_output.fbx";
	config.modelType = SkinnedModelType::Lumine;
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::PLAYER;
	Instantiate(config);

	instance.collider.SetCollisionCallback(OnItemPickupCallback, this);

	LoadWeapon("data/MODEL/character/Lumine", "Mitsurugi.fbx");
	LoadWeapon("data/MODEL/Weapon/Celestial_Revolver/Celestial_Revolver.obj");

	swordTrail = new SwordTrail(&m_weapon);
	swordTrail->SetTrailType(SwordTrailType::Normal);
	swordTrail->SetTrailFrames(5);

	InitializeStatus();

	InitEffect();
}

Lumine::~Lumine()
{
	SAFE_DELETE(m_stateMachine);
	SAFE_DELETE(swordTrail);
}

void Lumine::InitializeStatus(void)
{
	m_playAnimSpeed = PLAY_ANIM_SPD;
	weaponOnBackTimer = 0;
	weaponOnHandTimer = 0;
	weaponOnBack = false;
	storeWeaponOnBack = false;
	storeWeapon = false;
	instance.renderProgress.progress = 0.0f;
	instance.attributes.Initialize();
	if (m_stateMachine)
		m_stateMachine->SetCurrentState(STATE(PlayerState::STANDING));
}

void Lumine::PostInit(void)
{
	// 遅延用コンテキスト構築
	auto* ctx = new InitContext{
		this,
	};
	DeferredTaskOptions options{};
	options.async = true; // 非同期実行
	options.debugName = "AsyncFbxModelLoader_InitializeAnim_Lumine"; // デバッグ用の名前
	// タスクを DeferredTaskSystem に登録
	RawDeferredTask::Add(
		ctx,
		[](void* ptr)
		{
			auto* ctx = static_cast<InitContext*>(ptr);
			ctx->owner->InitializeAnimation();

			ctx->owner->instance.pModel->SetBodyDiffuseTexture("data/MODEL/character/Lumine/texture_0.png");
			ctx->owner->instance.pModel->SetHairDiffuseTexture("data/MODEL/character/Lumine/hair.png");
			ctx->owner->instance.pModel->SetFaceDiffuseTexture("data/MODEL/character/Lumine/face.png");

			ctx->owner->GameObject::PostInit();
		},
		nullptr,
		options
	);
}

void Lumine::InitializeAnimation(void)
{
	AddAnimation("data/MODEL/character/Lumine/", "Idle.fbx", AnimClipName::ANIM_STANDING);
	AddAnimation("data/MODEL/character/Lumine/", "Looking Around.fbx", AnimClipName::ANIM_IDLE);
	AddAnimation("data/MODEL/character/Lumine/", "Sneak Walk.fbx", AnimClipName::ANIM_DASH);
	AddAnimation("data/MODEL/character/Lumine/", "Running.fbx", AnimClipName::ANIM_RUN);
	AddAnimation("data/MODEL/character/Lumine/", "Walking.fbx", AnimClipName::ANIM_WALK);
	AddAnimation("data/MODEL/character/Lumine/", "Jump.fbx", AnimClipName::ANIM_JUMP);
	AddAnimation("data/MODEL/character/Lumine/", "Standing React Small From Left.fbx", AnimClipName::ANIM_HIT_REACTION_1);
	AddAnimation("data/MODEL/character/Lumine/", "Sword And Shield Slash.fbx", AnimClipName::ANIM_SWORD_SHIELD_SLASH);
	AddAnimation("data/MODEL/character/Lumine/", "Sword And Shield Slash 2.fbx", AnimClipName::ANIM_SWORD_SHIELD_SLASH2);
	AddAnimation("data/MODEL/character/Lumine/", "Sword And Shield Slash 3.fbx", AnimClipName::ANIM_SWORD_SHIELD_SLASH3);

	// Aim
	AddAnimation("data/MODEL/character/Lumine/", "Walk_Aiming.fbx", AnimClipName::ANIM_WALK_AIMING);
	AddAnimation("data/MODEL/character/Lumine/", "Run_Aiming.fbx", AnimClipName::ANIM_RUN_ANIMING);
	AddAnimation("data/MODEL/character/Lumine/", "Idle_Aiming.fbx", AnimClipName::ANIM_IDLE_AIMING);
	AddAnimation("data/MODEL/character/Lumine/", "Stand_Jump_Aiming.fbx", AnimClipName::ANIM_STAND_JUMP_AIMING);
	AddAnimation("data/MODEL/character/Lumine/", "Run_Jump_Aiming.fbx", AnimClipName::ANIM_RUN_JUMP_AIMING);

	AddAnimation("data/MODEL/character/Lumine/", "Climbing Up Wall.fbx", AnimClipName::ANIM_CLIB_WALL);

	SetupAnimStateMachine();
	InitAnimInfo();
}

void Lumine::AddAnimation(char* animPath, char* animName, AnimClipName clipName)
{
	if (instance.pModel)
		m_fbxLoader.LoadAnimation(instance.pModel, instance.pModel->GetModelData(), animPath, animName, clipName);
}

void Lumine::LoadWeapon(char* modelPath, char* modelName)
{
	if (modelName == nullptr)
	{
		m_gunPosX = GUN_POS_OFFSET_X;
		m_gunPosY = GUN_POS_OFFSET_Y;
		m_gunPosZ = GUN_POS_OFFSET_Z;
		m_gunRotX = GUN_ROT_OFFSET_X;
		m_gunRotY = GUN_ROT_OFFSET_Y;
		m_gunRotZ = GUN_ROT_OFFSET_Z;
		GameObjectConfig config;
		config.modelPath = modelPath;
		config.collisionType = ObjectCollisionType::NONE;
		config.scale = XMFLOAT3(GUN_SIZE, GUN_SIZE, GUN_SIZE);
		config.castShadow = true;
		m_gun.Instantiate(config);
		m_gun.SetPosition(XMFLOAT3(m_gunPosX, m_gunPosY, m_gunPosZ));
		m_gun.SetRotation(XMFLOAT3(m_gunRotX, m_gunRotY, m_gunRotZ));
	}
	else
	{
		SkinnedGameObjectConfig config;
		config.modelPath = modelPath;
		config.modelName = modelName;
		config.modelType = SkinnedModelType::Weapon;
		config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
		config.colliderTag = ColliderTag::PLAYER_ATTACK;
		config.scale = XMFLOAT3(WEAPON_SIZE, WEAPON_SIZE, WEAPON_SIZE);
		config.position = XMFLOAT3(WEAPON_ON_HANDS_POS_OFFSET_X, WEAPON_ON_HANDS_POS_OFFSET_Y, WEAPON_ON_HANDS_POS_OFFSET_Z);
		config.rotation = XMFLOAT3(WEAPON_ON_HANDS_ROT_OFFSET_X, WEAPON_ON_HANDS_ROT_OFFSET_Y, WEAPON_ON_HANDS_ROT_OFFSET_Z);
		config.castShadow = false;
		m_weapon.Instantiate(config);
	}

}

void Lumine::Update(void)
{
	if (!instance.load)
	{
		GameObject::Update();
		return;
	}

	if (!m_inputManager.GetKeyboardPress(DIK_W)
		&& !m_inputManager.GetKeyboardPress(DIK_S)
		&& !m_inputManager.GetKeyboardPress(DIK_A)
		&& !m_inputManager.GetKeyboardPress(DIK_D))
		instance.attributes.isMoving = false;


	if (instance.attributes.attackWindow2 == true
		|| instance.attributes.attackWindow3 == true)
	{
		instance.attributes.attackWinwdowCnt += timer.GetScaledDeltaTime();
		if (instance.attributes.attackWinwdowCnt >= ATTACK_COMBO_WINDOW
			|| instance.attributes.isMoving)
		{
			instance.attributes.attackWindow2 = false;
			instance.attributes.attackWindow3 = false;
			instance.attributes.attackWinwdowCnt = 0;
		}
	}

	if (m_inputManager.IsMouseRightPressed())
	{
		instance.attributes.aimMode = true;
	}
	else
	{
		instance.attributes.aimMode = false;
	}

	if (m_inputManager.GetKeyboardPress(DIK_LSHIFT))
	{
		instance.attributes.isRunning = true;
	}
	else
	{
		instance.attributes.isRunning = false;
	}

	switch (m_stateMachine->GetCurrentState())
	{
	case STATE(PlayerState::STANDING):
		PlayGeneralAnim(AnimClipName::ANIM_STANDING, m_playAnimSpeed);
		break;
	case STATE(PlayerState::IDLE):
		PlayGeneralAnim(AnimClipName::ANIM_IDLE, m_playAnimSpeed);
		break;
	case STATE(PlayerState::WALK):
		PlayGeneralAnim(AnimClipName::ANIM_WALK, m_playAnimSpeed);
		break;
	case STATE(PlayerState::RUN):
		PlayGeneralAnim(AnimClipName::ANIM_RUN, m_playAnimSpeed);
		break;
	case STATE(PlayerState::DASH):
		PlayGeneralAnim(AnimClipName::ANIM_DASH, m_playAnimSpeed);
		break;
	case STATE(PlayerState::JUMP):
		PlayJumpAnim();
		break;
	case STATE(PlayerState::ATTACK_1):
		PlayAttackAnim();
		break;
	case STATE(PlayerState::ATTACK_2):
		PlayAttack2Anim();
		break;
	case STATE(PlayerState::ATTACK_3):
		PlayAttack3Anim();
		break;
	case STATE(PlayerState::HIT):
		PlayGeneralAnim(AnimClipName::ANIM_HIT_REACTION_1, m_playAnimSpeed * 1.2f);
		break;
	case STATE(PlayerState::IDLE_AIMING):
		PlayGeneralAnim(AnimClipName::ANIM_IDLE_AIMING, m_playAnimSpeed);
		break;
	case STATE(PlayerState::WALK_AIMING):
		PlayGeneralAnim(AnimClipName::ANIM_WALK_AIMING, m_playAnimSpeed);
		break;
	case STATE(PlayerState::RUN_AIMING):
		PlayGeneralAnim(AnimClipName::ANIM_RUN_ANIMING, m_playAnimSpeed);
		break;
	case STATE(PlayerState::STAND_JUMP_AIMING):
		PlayGeneralAnim(AnimClipName::ANIM_STAND_JUMP_AIMING, m_playAnimSpeed * 1.2f);
		break;
	case STATE(PlayerState::RUN_JUMP_AIMING):
		PlayGeneralAnim(AnimClipName::ANIM_RUN_JUMP_AIMING, m_playAnimSpeed);
		break;
	case STATE(PlayerState::CLIMB_WALL):
		PlayClimbAnim();
		break;
	default:
		break;
	}

	GameObject::Update();
	m_stateMachine->Update(ANIM_BLEND_SPD * timer.GetScaledDeltaTime(), dynamic_cast<ISkinnedMeshModelChar*>(this));
	instance.pModel->UpdateBoneTransform(m_stateMachine->GetBoneMatrices());

	UpdateWeapon();

	if (instance.attributes.isAttacking || instance.attributes.isAttacking2 || instance.attributes.isAttacking3)
		swordTrail->Update();

	
	if (m_inputManager.IsMouseLeftPressed() || m_inputManager.IsMouseLeftTriggered())
	{
		XMFLOAT3 pos, dir;

		// 水の弾の速度
		float speed = 345.0f;
		if (instance.attributes.isMoving)
		{
			if (instance.attributes.isRunning)
				speed *= VALUE_MOVE * VALUE_RUN_FACTOR * 60.0f;
			else
				speed *= VALUE_MOVE * 60.0f;
		}

		// 水の弾の位置
		XMStoreFloat3(&pos, m_weapon.GetWorldMatrix().r[3]);
		float yaw = instance.transform.rot.y;
		dir = XMFLOAT3(sin(yaw), 0.0f, cos(yaw));
		// 武器の位置を少し上にずらす
		pos.y -= 25.0f;
		pos.x += sinf(instance.transform.rot.y) * 25.0f;
		pos.z += cosf(instance.transform.rot.y) * 25.0f;

		if (m_waterFluid && VoxelBufferUploader::get_instance().IsBuffersUploaded())
			m_waterFluid->EmitWater(dir, speed, 35.0f, 1, pos);
	}
}

void Lumine::Draw(void)
{
	if (!instance.load) return;

	if (storeWeaponOnBack)
	{
		instance.renderProgress.isRandomFade = true;
		instance.renderProgress.progress -= 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress < 0.0f)
		{
			instance.renderProgress.progress = 0.0f;
			weaponOnBack = true;
			weaponOnBackTimer = WEAPON_ON_BACK_TIME;
			storeWeaponOnBack = false;
			m_weapon.SetPosition(XMFLOAT3(WEAPON_ON_BACK_POS_OFFSET_X, WEAPON_ON_BACK_POS_OFFSET_Y, WEAPON_ON_BACK_POS_OFFSET_Z));
			m_weapon.SetRotation(XMFLOAT3(WEAPON_ON_BACK_ROT_OFFSET_X, WEAPON_ON_BACK_ROT_OFFSET_Y, WEAPON_ON_BACK_ROT_OFFSET_Z));
		}
	}
	else if (weaponOnBackTimer >= 0 && weaponOnBack == true)
	{
		instance.renderProgress.progress += 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress >= 1.0f)
		{
			//weaponOnBack = false;
			instance.renderProgress.progress = 1.0f;
		}
	}
	else if (storeWeapon)
	{
		instance.renderProgress.progress -= 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress < 0.0f)
		{
			instance.renderProgress.progress = 0.0f;
			m_weapon.SetCastShadow(false);
			storeWeapon = false;
		}
	}

	renderer.SetRenderProgress(instance.renderProgress);
	if (instance.renderProgress.progress > 0)
	{
		m_weapon.SetCastShadow(true);
		m_weapon.Draw();
	}


	if (instance.attributes.charSwitchEffect == TRUE)
	{
		instance.renderProgress.progress += 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress >= 1.0f)
		{
			instance.renderProgress.progress = 1.0f;
			instance.attributes.charSwitchEffect = FALSE;
		}
	}
	else
	{
		RenderProgressCBuffer defaultRenderProgress;
		defaultRenderProgress.isRandomFade = false;
		defaultRenderProgress.progress = 1.0f;

		renderer.SetRenderProgress(defaultRenderProgress);

	}
	GameObject::Draw();

	m_renderer.SetStaticModelInputLayout(); // モデルの入力レイアウトを設定
	m_renderer.SetRenderObject(); // モデルの描画を設定
	m_gun.Draw();
	m_renderer.SetSkinnedMeshInputLayout(); // スキニングメッシュの入力レイアウトを設定
	m_renderer.SetRenderSkinnedMeshModel(); // スキニングメッシュモデルの描画を設定
}

void Lumine::DrawEffect(void)
{
	if (instance.attributes.isAttacking ||
		instance.attributes.isAttacking2 ||
		instance.attributes.isAttacking3)
		swordTrail->Draw();
}

AnimStateMachine* Lumine::GetStateMachine(void)
{
	return m_stateMachine;
}

void Lumine::PlayJumpAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_JUMP)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(m_playAnimSpeed * 1.2f);
}

void Lumine::PlayClimbAnim(void)
{
	float animSpeed = m_playAnimSpeed;
	if (instance.attributes.climbDown)
		animSpeed *= -1.0f; // 下りるときはアニメーションを逆再生
	else if (instance.attributes.climbUp)
		animSpeed *= 1.0f; // 上るときはアニメーションを正再生
	else
		animSpeed = 0.0f; // クライミング中はアニメーションを停止

	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_CLIB_WALL)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(animSpeed);
}

void Lumine::SetupAnimStateMachine()
{
	m_stateMachine = new AnimStateMachine(dynamic_cast<ISkinnedMeshModelChar*>(this));

	// 各アニメーションの状態を追加
	m_stateMachine->AddState(STATE(PlayerState::IDLE), instance.pModel->GetAnimationClip(AnimClipName::ANIM_IDLE));
	m_stateMachine->AddState(STATE(PlayerState::STANDING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_STANDING));
	m_stateMachine->AddState(STATE(PlayerState::WALK), instance.pModel->GetAnimationClip(AnimClipName::ANIM_WALK));
	m_stateMachine->AddState(STATE(PlayerState::RUN), instance.pModel->GetAnimationClip(AnimClipName::ANIM_RUN));
	m_stateMachine->AddState(STATE(PlayerState::DASH), instance.pModel->GetAnimationClip(AnimClipName::ANIM_DASH));
	m_stateMachine->AddState(STATE(PlayerState::JUMP), instance.pModel->GetAnimationClip(AnimClipName::ANIM_JUMP));

	// 攻撃関連のアニメーション
	m_stateMachine->AddState(STATE(PlayerState::ATTACK_1), instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH));
	m_stateMachine->AddState(STATE(PlayerState::ATTACK_2), instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH2));
	m_stateMachine->AddState(STATE(PlayerState::ATTACK_3), instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH3));
	m_stateMachine->AddState(STATE(PlayerState::HIT), instance.pModel->GetAnimationClip(AnimClipName::ANIM_HIT_REACTION_1));

	// Aim関連のアニメーション
	m_stateMachine->AddState(STATE(PlayerState::IDLE_AIMING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_IDLE_AIMING));
	m_stateMachine->AddState(STATE(PlayerState::WALK_AIMING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_WALK_AIMING));
	m_stateMachine->AddState(STATE(PlayerState::RUN_AIMING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_RUN_ANIMING));
	m_stateMachine->AddState(STATE(PlayerState::STAND_JUMP_AIMING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_STAND_JUMP_AIMING));
	m_stateMachine->AddState(STATE(PlayerState::RUN_JUMP_AIMING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_RUN_JUMP_AIMING));

	// Climb関連のアニメーション
	m_stateMachine->AddState(STATE(PlayerState::CLIMB_WALL), instance.pModel->GetAnimationClip(AnimClipName::ANIM_CLIB_WALL));

	//状態遷移
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::WALK), &ISkinnedMeshModelChar::CanWalk);
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::CanAttack);
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::ATTACK_2), &ISkinnedMeshModelChar::CanAttack2);
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::ATTACK_3), &ISkinnedMeshModelChar::CanAttack3);
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump);
	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::HIT), &ISkinnedMeshModelChar::CanHit);
	m_stateMachine->AddTransition(STATE(PlayerState::JUMP), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::CanAttack);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopMoving);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::RUN), &ISkinnedMeshModelChar::CanRun);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::HIT), &ISkinnedMeshModelChar::CanHit);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::WALK_AIMING), &ISkinnedMeshModelChar::CanWalkAim);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopMoving);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN), STATE(PlayerState::WALK), &ISkinnedMeshModelChar::CanStopRunning);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN), STATE(PlayerState::RUN_AIMING), &ISkinnedMeshModelChar::CanRunAim, false, false);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_1), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_1), STATE(PlayerState::HIT), &ISkinnedMeshModelChar::CanHit);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_2), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_2), STATE(PlayerState::HIT), &ISkinnedMeshModelChar::CanHit);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_3), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_3), STATE(PlayerState::HIT), &ISkinnedMeshModelChar::CanHit);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_1), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump, false);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_2), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump, false);
	m_stateMachine->AddTransition(STATE(PlayerState::ATTACK_3), STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::CanJump, false);
	m_stateMachine->AddTransition(STATE(PlayerState::HIT), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);

	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::IDLE_AIMING), &ISkinnedMeshModelChar::CanAim);
	m_stateMachine->AddTransition(STATE(PlayerState::IDLE_AIMING), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopAiming);
	m_stateMachine->AddTransition(STATE(PlayerState::IDLE_AIMING), STATE(PlayerState::WALK_AIMING), &ISkinnedMeshModelChar::CanWalkAim);
	m_stateMachine->AddTransition(STATE(PlayerState::IDLE_AIMING), STATE(PlayerState::RUN_AIMING), &ISkinnedMeshModelChar::CanRunAim);
	m_stateMachine->AddTransition(STATE(PlayerState::IDLE_AIMING), STATE(PlayerState::STAND_JUMP_AIMING), &ISkinnedMeshModelChar::CanJumpAim);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK_AIMING), STATE(PlayerState::IDLE_AIMING), &ISkinnedMeshModelChar::CanStopMoving);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK_AIMING), STATE(PlayerState::RUN_AIMING), &ISkinnedMeshModelChar::CanRunAim);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK_AIMING), STATE(PlayerState::WALK), &ISkinnedMeshModelChar::CanWalkAimToWalk);
	m_stateMachine->AddTransition(STATE(PlayerState::WALK_AIMING), STATE(PlayerState::STAND_JUMP_AIMING), &ISkinnedMeshModelChar::CanJumpAim);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN_AIMING), STATE(PlayerState::IDLE_AIMING), &ISkinnedMeshModelChar::CanStopMoving);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN_AIMING), STATE(PlayerState::WALK_AIMING), &ISkinnedMeshModelChar::CanStopRunning);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN_AIMING), STATE(PlayerState::RUN), &ISkinnedMeshModelChar::CanRunAimToRun, false, false);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN_AIMING), STATE(PlayerState::RUN_JUMP_AIMING), &ISkinnedMeshModelChar::CanJumpAim);
	m_stateMachine->AddTransition(STATE(PlayerState::RUN_JUMP_AIMING), STATE(PlayerState::RUN_AIMING), &ISkinnedMeshModelChar::AlwaysTrue, true);
	m_stateMachine->AddTransition(STATE(PlayerState::STAND_JUMP_AIMING), STATE(PlayerState::IDLE_AIMING), &ISkinnedMeshModelChar::AlwaysTrue, true);

	m_stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::CLIMB_WALL), &ISkinnedMeshModelChar::CanClimb, false, false);
	m_stateMachine->AddTransition(STATE(PlayerState::CLIMB_WALL), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopClimbing, false, false);

	m_stateMachine->SetEndCallback(STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::OnAttackAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::ATTACK_2), &ISkinnedMeshModelChar::OnAttackAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::ATTACK_3), &ISkinnedMeshModelChar::OnAttackAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::HIT), &ISkinnedMeshModelChar::OnHitAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::JUMP), &ISkinnedMeshModelChar::OnJumpAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::STAND_JUMP_AIMING), &ISkinnedMeshModelChar::OnJumpAnimationEnd);
	m_stateMachine->SetEndCallback(STATE(PlayerState::RUN_JUMP_AIMING), &ISkinnedMeshModelChar::OnJumpAnimationEnd);

	m_stateMachine->SetCurrentState(STATE(PlayerState::STANDING));
}

void Lumine::InitAnimInfo(void)
{
	AnimationClip* attack1 = instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH);
	if (attack1)
	{
		attack1->animInfo.animPhase.startMoveFraction = 0.0f;
		attack1->animInfo.animPhase.endMoveFraction = 0.3f;
		attack1->animInfo.animPhase.startAttackFraction = 0.3f;
		attack1->animInfo.animPhase.endAttackFraction = 0.6f;
	}
	AnimationClip* attack2 = instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH2);
	if (attack2)
	{
		attack2->animInfo.animPhase.startMoveFraction = 0.0f;
		attack2->animInfo.animPhase.endMoveFraction = 0.3f;
		attack2->animInfo.animPhase.startAttackFraction = 0.3f;
		attack2->animInfo.animPhase.endAttackFraction = 0.6f;
	}
	AnimationClip* attack3 = instance.pModel->GetAnimationClip(AnimClipName::ANIM_SWORD_SHIELD_SLASH3);
	if (attack3)
	{
		attack3->animInfo.animPhase.startMoveFraction = 0.0f;
		attack3->animInfo.animPhase.endMoveFraction = 0.3f;
		attack3->animInfo.animPhase.startAttackFraction = 0.3f;
		attack3->animInfo.animPhase.endAttackFraction = 0.6f;
	}
}

void Lumine::PlayGeneralAnim(AnimClipName clipName, float speed)
{
	if (instance.pModel->GetCurrentAnim() != clipName)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(speed);
}

bool Lumine::ExecuteAction(ActionEnum action)
{
	switch (action)
	{
	case ActionEnum::ATTACK:
		if (m_stateMachine->GetCurrentState() == STATE(PlayerState::ATTACK_1) ||
			m_stateMachine->GetCurrentState() == STATE(PlayerState::ATTACK_2) ||
			m_stateMachine->GetCurrentState() == STATE(PlayerState::ATTACK_3) ||
			m_stateMachine->GetCurrentState() == STATE(PlayerState::JUMP) || 
			m_stateMachine->GetCurrentState() == STATE(PlayerState::RUN))
			return false;
		else
		{
			if (instance.attributes.attackWindow2)
			{
				instance.attributes.isAttacking = false;
				instance.attributes.isAttacking2 = true;
				instance.attributes.isAttacking3 = false;
			}

			else if (instance.attributes.attackWindow3)
			{
				instance.attributes.isAttacking = false;
				instance.attributes.isAttacking2 = false;
				instance.attributes.isAttacking3 = true;
			}
			else
			{
				instance.attributes.isAttacking = true;
				instance.attributes.isAttacking2 = false;
				instance.attributes.isAttacking3 = false;
			}
			m_weapon.SetPosition(XMFLOAT3(WEAPON_ON_HANDS_POS_OFFSET_X, WEAPON_ON_HANDS_POS_OFFSET_Y, WEAPON_ON_HANDS_POS_OFFSET_Z));
			m_weapon.SetRotation(XMFLOAT3(WEAPON_ON_HANDS_ROT_OFFSET_X, WEAPON_ON_HANDS_ROT_OFFSET_Y, WEAPON_ON_HANDS_ROT_OFFSET_Z));
			weaponOnBack = false;
			instance.renderProgress.progress = 1.0f;
			instance.attributes.attackWinwdowCnt = 0;
			return true;
		}
	default:
		return false;
	}
}

void Lumine::PlayAttackAnim(void)
{
	FaceToNearestEnemy();

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
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_SWORD_SHIELD_SLASH)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(m_playAnimSpeed * 1.5f);
}

void Lumine::PlayAttack2Anim(void)
{
	FaceToNearestEnemy();
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
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_SWORD_SHIELD_SLASH2)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(m_playAnimSpeed * 1.5f);
}

void Lumine::PlayAttack3Anim(void)
{
	FaceToNearestEnemy();

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

	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_SWORD_SHIELD_SLASH3)
		instance.pModel->SetCurrentAnim(m_stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(m_playAnimSpeed * 1.5f);
}

void Lumine::InitEffect(void)
{
	WaterFluidParams waterParams;
	waterParams.type = EffectType::WaterFluid;
	waterParams.scale = 15.0f; // 粒子の基本スケール（sizeの初期値）
	waterParams.position = XMFLOAT3(0.0f, 0.0f, 0.0f); // 初期生成位置
	waterParams.acceleration = XMFLOAT3(0.0f, -125.0f, 0.0f); // 重力方向（Y軸マイナス）
	waterParams.numParticles = 5000; // 最大粒子数
	waterParams.startColor = XMFLOAT4(0.2f, 0.5f, 1.0f, 0.5f); // 水色
	waterParams.endColor = XMFLOAT4(0.0f, 0.3f, 0.7f, 1.0f);   // 寿命で暗く

	// エミット制御（あまり使わないが、baseクラス必須）
	waterParams.spawnRateMin = 0;
	waterParams.spawnRateMax = 0;
	waterParams.lifeMin = 9999.0f; // 実質無限寿命（蒸発未実装なら）
	waterParams.lifeMax = 9999.0f; // 実質無限寿命（蒸発未実装なら）

	// 水の物理パラメータ
	waterParams.supportRadius = 15.0f;		// サポート半径（粒子影響範囲）
	waterParams.restDensity = 50.0f;		// 静止密度（流体の基準密度）
	waterParams.particleMass = 15.0f;		// 粒子質量
	waterParams.stiffness = 4000.0f;		// 拘束強度（大きいと固くなる）
	waterParams.friction = 0.05f;			// 速度減衰（地面との摩擦的な）
	waterParams.viscosity = 5.0f;			// 粘性係数（流体の粘り気）
	waterParams.restitution = 0.0f;			// 反発係数（衝突時の挙動制御用）

	m_waterFluid = dynamic_cast<WaterFluidParticleRenderer*>(EffectSystem::get_instance().SpawnParticleEffect(waterParams));
}

void Lumine::FaceToNearestEnemy(void)
{
	XMVECTOR playerPosVec = XMLoadFloat3(&instance.transform.pos);
	auto enemyList = EnemyManager::get_instance().GetEnemy();
	Node<Enemy*>* cur = enemyList->getHead();
	Enemy* nearestEnemy = nullptr;
	float minDistSq = FLT_MAX;
	while (cur != nullptr)
	{
		if (cur->data->GetEnemyAttribute().isDead == true)
		{
			cur = cur->next;
			continue;
		}

		XMVECTOR enemyPosVec = XMLoadFloat3(&cur->data->GetTransform().pos);
		XMVECTOR diff = XMVectorSubtract(enemyPosVec, playerPosVec);
		float distSq = XMVectorGetX(XMVector3LengthSq(diff));

		if (distSq < minDistSq)
		{
			minDistSq = distSq;
			nearestEnemy = cur->data;
		}

		cur = cur->next;
	}

	if (nearestEnemy == nullptr)
		return;

	float dx = nearestEnemy->GetTransform().pos.x - instance.transform.pos.x;
	float dz = nearestEnemy->GetTransform().pos.z - instance.transform.pos.z;

	instance.transform.rot.y = -atan2(dz, dx) + XM_PI * 0.5f;
	instance.attributes.dir = instance.transform.rot.y;
	instance.attributes.targetDir = instance.transform.rot.y;

	if (minDistSq > ATTACK_RANGE * ATTACK_RANGE)
	{
		const AnimationClip* currentAnimClip = m_stateMachine->GetCurrentAnimClip();

		// アニメーションの再生進捗を取得
		float attackAnimTime = m_stateMachine->GetCurrentAnimTime();
		// 最大移動距離
		float moveStep = min(minDistSq - ATTACK_RANGE, MAX_ATTACK_STEP);
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
			if (minDistSq <= ATTACK_RANGE * ATTACK_RANGE)
			{
				instance.attributes.spd *= (1.0f - attackPhase * attackPhase); // 速度を急激に減少
			}
		}
		else
		{
			instance.attributes.spd = 0.0f; // 移動を停止
		}
	}
}

void Lumine::UpdateWeapon(void)
{
	if (weaponOnHandTimer > 0)
	{
		weaponOnHandTimer -= timer.GetScaledDeltaTime();
		if (weaponOnHandTimer <= 0)
		{
			storeWeaponOnBack = true;
			instance.renderProgress.progress = 1.0f;
		}
	}

	if (weaponOnBackTimer > 0)
	{
		weaponOnBackTimer -= timer.GetScaledDeltaTime();
		if (weaponOnBackTimer <= 0)
		{
			storeWeapon = true;
		}
	}

	m_weapon.Update();
	m_gun.Update();

	XMMATRIX weaponMtx, gunMtx;

	if (!weaponOnBack)
		weaponMtx = instance.pModel->GetWeaponTransformMtx();
	else
		weaponMtx = instance.pModel->GetBodyTransformMtx();

	weaponMtx = XMMatrixMultiply(m_weapon.GetWorldMatrix(), weaponMtx);
	weaponMtx = XMMatrixMultiply(weaponMtx, instance.transform.mtxWorld);
	m_weapon.SetWorldMatrix(weaponMtx);

	gunMtx = instance.pModel->GetWeaponTransformMtx();
	gunMtx = XMMatrixMultiply(m_gun.GetWorldMatrix(), gunMtx);
	gunMtx = XMMatrixMultiply(gunMtx, instance.transform.mtxWorld);
	m_gun.SetWorldMatrix(gunMtx);

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

bool Lumine::CanWalk(void) const
{
	return instance.attributes.isMoving && 
		!instance.attributes.isAttacking &&
		!instance.attributes.isAttacking2 &&
		!instance.attributes.isAttacking3;
}

bool Lumine::CanStopMoving() const
{
	return !instance.attributes.isMoving;
}

bool Lumine::CanAttack() const
{
	return instance.attributes.isAttacking;
}

bool Lumine::CanAttack2() const
{
	return (instance.attributes.attackWindow2 && instance.attributes.isAttacking2);
}

bool Lumine::CanAttack3() const
{
	return (instance.attributes.attackWindow3 && instance.attributes.isAttacking3);
}

bool Lumine::CanRun(void) const
{
	return instance.attributes.isMoving && instance.attributes.isRunning;
}

bool Lumine::CanStopRunning(void) const
{
	return instance.attributes.isMoving && !instance.attributes.isRunning;
}

bool Lumine::CanHit(void) const
{
	return instance.attributes.isHit1;
}

bool Lumine::CanJump(void) const
{
	return instance.attributes.isJumping;
}

bool Lumine::CanAim(void) const
{
	return instance.attributes.aimMode;
}

bool Lumine::CanWalkAim(void) const
{
	return CanWalk() && CanAim();
}

bool Lumine::CanWalkAimToWalk(void) const
{
	return CanWalk() && CanStopAiming();
}

bool Lumine::CanRunAim(void) const
{
	return CanRun() && CanAim();
}

bool Lumine::CanRunAimToRun(void) const
{
	return CanRun() && CanStopAiming();
}

bool Lumine::CanStopAiming(void) const
{
	return !instance.attributes.aimMode;
}

bool Lumine::CanJumpAim(void) const
{
	return instance.attributes.isJumping && CanAim();
}

bool Lumine::CanRunJumpAim(void) const
{
	return instance.attributes.isJumping && CanRunAim();
}

bool Lumine::CanClimb(void) const
{
	return instance.attributes.climbMode;
}

bool Lumine::CanStopClimbing(void) const
{
	return !instance.attributes.climbMode;
}


void Lumine::OnAttackAnimationEnd(void)
{
	m_weapon.SetColliderEnable(false);
	storeWeaponOnBack = false;
	weaponOnHandTimer = WEAPON_ON_HAND_TIME;
	instance.attributes.isAttacking = false;
	instance.attributes.isAttacking2 = false;
	instance.attributes.isAttacking3 = false;

	if (instance.attributes.attackWindow2 == false && 
		instance.attributes.attackWindow3 == false)
	{
		instance.attributes.attackWindow2 = true;
	}
	else if (instance.attributes.attackWindow3 == false)
	{
		instance.attributes.attackWindow2 = false;
		instance.attributes.attackWindow3 = true;
	}
	else
	{
		instance.attributes.attackWindow2 = false;
		instance.attributes.attackWindow3 = false;
	}

	instance.attributes.attackWinwdowCnt = 0;
}

void Lumine::OnHitAnimationEnd(void)
{
	instance.attributes.isHit1 = false;
}

void Lumine::OnJumpAnimationEnd(void)
{
	instance.attributes.isJumping = false;
}

void Lumine::SetPlayerComponent(void* player)
{
	m_player = player;
}

void* Lumine::GetPlayerComponent(void)
{
	return m_player;
}

void Lumine::OnItemPickupCallback(const CollisionEvent& event, void* context)
{
	Lumine* self = static_cast<Lumine*>(context);
	if (self == nullptr || !self->instance.use || !self->instance.load)
		return;

	const Collider* itemCol = nullptr;
	const Collider* playerCol = nullptr;

	if (event.colliderA->tag == ColliderTag::ITEM && event.colliderB->tag == ColliderTag::PLAYER)
	{
		itemCol = event.colliderA;
		playerCol = event.colliderB;
	}
	else if (event.colliderB->tag == ColliderTag::ITEM && event.colliderA->tag == ColliderTag::PLAYER)
	{
		itemCol = event.colliderB;
		playerCol = event.colliderA;
	}

	if (itemCol && playerCol)
	{
		Item* item = static_cast<Item*>(itemCol->owner);
		if (item && item->GetItemType() == ItemType::Seed)
		{

		}
		else if (item && item->GetItemType() == ItemType::Key)
		{
			self->EnableWeapon(true);
		}
	}

}

//void Lumine::RenderImGui(void)
//{
//	if (ImGui::CollapsingHeader("Weapon Transform"))
//	{
//		ImGui::SliderFloat("Gun PosX: ", &m_gunPosX, -100.0f, 100.0f, "%.1f");
//		ImGui::SliderFloat("Gun PosY: ", &m_gunPosY, -100.0f, 100.0f, "%.1f");
//		ImGui::SliderFloat("Gun PosZ: ", &m_gunPosZ, -100.0f, 100.0f, "%.1f");
//		ImGui::SliderFloat("Gun RotX: ", &m_gunRotX, -XM_PI, XM_PI, "%.1f");
//		ImGui::SliderFloat("Gun RotY: ", &m_gunRotY, -XM_PI, XM_PI, "%.1f");
//		ImGui::SliderFloat("Gun RotZ: ", &m_gunRotZ, -XM_PI, XM_PI, "%.1f");
//	}
//}
