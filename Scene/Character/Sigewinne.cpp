//=============================================================================
//
// Sigewinneˆ— [Sigewinne.cpp]
// Author : 
//
//=============================================================================
#include "Sigewinne.h"
#include "InputManager.h"
#include "Debugproc.h"
#include "Player.h"
#include "CursorManager.h"

//*****************************************************************************
// ƒ}ƒNƒ’è‹`
//*****************************************************************************
#define PLAY_ANIM_SPD				1.0f
#define ANIM_BLEND_SPD				0.032f

#define WEAPON_SIZE						30.0f
#define WEAPON_ON_HANDS_POS_OFFSET_X	25.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Y	65.0f
#define WEAPON_ON_HANDS_POS_OFFSET_Z	-5.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_X	0.0f
#define WEAPON_ON_HANDS_ROT_OFFSET_Y	XM_PI
#define WEAPON_ON_HANDS_ROT_OFFSET_Z	0.0f
#define WEAPON_ON_BACK_ROT_OFFSET_X		0.0f
#define WEAPON_ON_BACK_ROT_OFFSET_Y		XM_PI
#define WEAPON_ON_BACK_ROT_OFFSET_Z		0.0f

Sigewinne::Sigewinne()
{
	Instantiate("data/MODEL/character/Sigewinne", "Character_output.fbx", ModelType::Sigewinne);
	instance.collider.type = ColliderType::PLAYER;
	instance.collider.owner = this;
	instance.collider.enable = true;
	CollisionManager::get_instance().RegisterDynamicCollider(&instance.collider);

	LoadWeapon("data/MODEL/character/Sigewinne", "bow.fbx");

	instance.pModel->SetDrawBoundingBox(false);
	weapon.GetSkinnedMeshModel()->SetDrawBoundingBox(false);


	AddAnimation("data/MODEL/character/Sigewinne/", "Breathing Idle.fbx", AnimClipName::ANIM_STANDING);
	AddAnimation("data/MODEL/character/Sigewinne/", "Standing Draw Arrow.fbx", AnimClipName::ANIM_STANDING_DRAW_ARROW);
	AddAnimation("data/MODEL/character/Sigewinne/", "Standing Aim Walk Forward.fbx", AnimClipName::ANIM_STANDING_AIM_WALK_FORWARD);
	AddAnimation("data/MODEL/character/Sigewinne/", "Standing Aim Walk Left.fbx", AnimClipName::ANIM_STANDING_AIM_WALK_LEFT);
	AddAnimation("data/MODEL/character/Sigewinne/", "Walking.fbx", AnimClipName::ANIM_WALK);
	AddAnimation("data/MODEL/character/Sigewinne/", "Running.fbx", AnimClipName::ANIM_RUN);
	AddAnimation("data/MODEL/character/Sigewinne/", "Breakdance Uprock Var 2.fbx", AnimClipName::ANIM_BREAKDANCE_UPROCK);

	instance.pModel->SetBodyDiffuseTexture("data/MODEL/character/Sigewinne/texture_0.png");
	//instance.pModel->SetBodyLightMapTexture("data/MODEL/character/Sigewinne/Avatar_Loli_Bow_Sigewinne_Tex_Body_Lightmap.png");
	//instance.pModel->SetBodyNormalMapTexture("data/MODEL/character/Sigewinne/Avatar_Loli_Bow_Sigewinne_Tex_Body_Normalmap.png");
	instance.pModel->SetHairDiffuseTexture("data/MODEL/character/Sigewinne/Avatar_Loli_Bow_Sigewinne_Tex_Hair_Diffuse.png");
	//instance.pModel->SetHairLightMapTexture("data/MODEL/character/Sigewinne/Avatar_Loli_Bow_Sigewinne_Tex_Hair_Lightmap.png");
	instance.pModel->SetFaceDiffuseTexture("data/MODEL/character/Sigewinne/face.png");
	//instance.pModel->SetFaceLightMapTexture("data/MODEL/character/Sigewinne/Avatar_Loli_Tex_FaceLightmap.png");

	playAnimSpeed = PLAY_ANIM_SPD;

	SetupAnimStateMachine();
}

Sigewinne::~Sigewinne()
{
	SAFE_DELETE(stateMachine);
}

void Sigewinne::AddAnimation(char* animPath, char* animName, AnimClipName clipName)
{
	if (instance.pModel)
		fbxLoader.LoadAnimation(renderer.GetDevice(), *instance.pModel, animPath, animName, clipName);
}

void Sigewinne::LoadWeapon(char* modelPath, char* modelName)
{
	weapon.Instantiate(modelPath, modelName, ModelType::Weapon);
	weapon.SetScale(XMFLOAT3(WEAPON_SIZE, WEAPON_SIZE, WEAPON_SIZE));
	weapon.SetPosition(XMFLOAT3(WEAPON_ON_HANDS_POS_OFFSET_X, WEAPON_ON_HANDS_POS_OFFSET_Y, WEAPON_ON_HANDS_POS_OFFSET_Z));
	weapon.SetRotation(XMFLOAT3(WEAPON_ON_HANDS_ROT_OFFSET_X, WEAPON_ON_HANDS_ROT_OFFSET_Y, WEAPON_ON_HANDS_ROT_OFFSET_Z));
}

void Sigewinne::Update(void)
{

	if (stateMachine->GetCurrentState() == STATE(PlayerState::IDLE))
	{
		if (IsMouseLeftTriggered() && !CursorManager::get_instance().IsMouseFreeMode())
		{
			instance.attributes.isAttacking = true;
		}
	}

	if (!GetKeyboardPress(DIK_W)
		&& !GetKeyboardPress(DIK_S)
		&& !GetKeyboardPress(DIK_A)
		&& !GetKeyboardPress(DIK_D))
		instance.attributes.isMoving = false;

	

	stateMachine->Update(ANIM_BLEND_SPD, dynamic_cast<ISkinnedMeshModelChar*>(this));

	switch (stateMachine->GetCurrentState())
	{
	case STATE(PlayerState::STANDING):
		PlayStandingAnim();
		break;
	case STATE(PlayerState::IDLE):
		PlayIdleAnim();
		break;
	case STATE(PlayerState::WALK):
		PlayWalkAnim();
		break;
	case STATE(PlayerState::RUN):
		PlayRunAnim();
		break;
	case STATE(PlayerState::DASH):
		PlayDashAnim();
		break;
	case STATE(PlayerState::JUMP):
		break;
	case STATE(PlayerState::ATTACK_1):
		PlayAttackAnim();
		break;
	default:
		break;
	}

	instance.pModel->UpdateBoneTransform(stateMachine->GetBoneMatrices());

	GameObject::Update();
	weapon.Update();

	XMMATRIX weaponMtx = instance.pModel->GetWeaponTransformMtx();
	weaponMtx = XMMatrixMultiply(weapon.GetWorldMatrix(), weaponMtx);
	weaponMtx = XMMatrixMultiply(weaponMtx, instance.transform.mtxWorld);

	weapon.SetWorldMatrix(weaponMtx);
}

void Sigewinne::Draw(void)
{
	GameObject::Draw();
	weapon.Draw();
}

AnimStateMachine* Sigewinne::GetStateMachine(void)
{
	return stateMachine;
}

void Sigewinne::SetupAnimStateMachine()
{
	stateMachine = new AnimStateMachine(dynamic_cast<ISkinnedMeshModelChar*>(this));

	stateMachine->AddState(STATE(PlayerState::STANDING), instance.pModel->GetAnimationClip(AnimClipName::ANIM_STANDING));
	stateMachine->AddState(STATE(PlayerState::IDLE), instance.pModel->GetAnimationClip(AnimClipName::ANIM_IDLE));
	stateMachine->AddState(STATE(PlayerState::WALK), instance.pModel->GetAnimationClip(AnimClipName::ANIM_WALK));
	stateMachine->AddState(STATE(PlayerState::RUN), instance.pModel->GetAnimationClip(AnimClipName::ANIM_RUN));
	stateMachine->AddState(STATE(PlayerState::ATTACK_1), instance.pModel->GetAnimationClip(AnimClipName::ANIM_STANDING_DRAW_ARROW));

	//ó‘Ô‘JˆÚ
	stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::WALK), &ISkinnedMeshModelChar::CanWalk);
	stateMachine->AddTransition(STATE(PlayerState::STANDING), STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::CanAttack);
	stateMachine->AddTransition(STATE(PlayerState::IDLE), STATE(PlayerState::WALK), &ISkinnedMeshModelChar::CanWalk);
	stateMachine->AddTransition(STATE(PlayerState::IDLE), STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::CanAttack);
	stateMachine->AddTransition(STATE(PlayerState::RUN), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopMoving);
	stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::RUN), &ISkinnedMeshModelChar::CanRun);
	stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::CanStopMoving);
	stateMachine->AddTransition(STATE(PlayerState::WALK), STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::CanAttack);
	stateMachine->AddTransition(STATE(PlayerState::ATTACK_1), STATE(PlayerState::STANDING), &ISkinnedMeshModelChar::AlwaysTrue, true);

	stateMachine->SetEndCallback(STATE(PlayerState::ATTACK_1), &ISkinnedMeshModelChar::OnAttackAnimationEnd);

	stateMachine->SetCurrentState(STATE(PlayerState::STANDING));
}
void Sigewinne::PlayWalkAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_WALK)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}

void Sigewinne::PlayRunAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_RUN)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}

void Sigewinne::PlayJumpAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_JUMP)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}

void Sigewinne::PlayIdleAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_IDLE)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}

void Sigewinne::PlayStandingAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_STANDING)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}


void Sigewinne::PlayAttackAnim(void)
{
	if (instance.pModel->GetCurrentAnim() != AnimClipName::ANIM_STANDING_DRAW_ARROW)
		instance.pModel->SetCurrentAnim(stateMachine->GetCurrentAnimClip());
	instance.pModel->PlayCurrentAnim(playAnimSpeed);
}

void Sigewinne::PlayHitAnim(void)
{
}

bool Sigewinne::CanWalk(void) const
{
	return instance.attributes.isMoving && !instance.attributes.isAttacking;
}

bool Sigewinne::CanStopMoving() const
{
	return !instance.attributes.isMoving;
}

bool Sigewinne::CanAttack() const
{
	return instance.attributes.isAttacking;
}

bool Sigewinne::CanRun(void) const
{
	return instance.attributes.isMoving && GetKeyboardPress(DIK_LSHIFT);
}

bool Sigewinne::CanHit(void) const
{
	return instance.attributes.isHit1;
}

void Sigewinne::OnAttackAnimationEnd(void)
{
	instance.attributes.isAttacking = false;
}

void Sigewinne::OnHitAnimationEnd(void)
{
	instance.attributes.isHit1 = false;
}
