#pragma once
//=============================================================================
//
// モデル処理 [Lumine.h]
// Author : 
//
//=============================================================================
#include "Scene/GameObject.h"
#include "Effects/SwordTrail.h"
#include "Utility/InputManager.h"
#include "Effects/WaterFluidParticleRenderer.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************

class Lumine : public GameObject<SkinnedMeshModelInstance>, public ISkinnedMeshModelChar
{
public:
	Lumine();
	~Lumine();

	void InitializeStatus(void);

	// アニメーションやスクリプトなど、モデルに依存する処理をここで設定する
	virtual void PostInit(void) override;
	virtual void InitializeAnimation(void) override;

	void Update(void) override;
	void Draw(void) override;
	void DrawEffect(void) override;

	bool LoadFinished(void) { return this->GetLoad() && m_weapon.GetLoad(); }

	AnimStateMachine* GetStateMachine(void) override;

	void PlayWalkAnim(void) override;
	void PlayRunAnim(void) override;
	void PlayJumpAnim(void) override;
	void PlayIdleAnim(void) override;
	void PlayDashAnim(void) override;
	void PlayStandingAnim(void) override;

	virtual bool ExecuteAction(ActionEnum action) override;

	void PlayAttackAnim(void) override;
	void PlayAttack2Anim(void);
	void PlayAttack3Anim(void);

	void PlayHitAnim(void) override;

	void InitEffect(void);

	virtual bool CanWalk(void) const override;
	virtual bool CanStopMoving(void) const override;
	virtual bool CanAttack(void) const override;
	virtual bool CanAttack2(void) const override;
	virtual bool CanAttack3(void) const override;
	virtual bool CanRun(void) const override;
	virtual bool CanHit(void) const override;
	virtual bool CanJump(void) const override;

	virtual void OnAttackAnimationEnd(void) override;
	virtual void OnHitAnimationEnd(void) override;
	virtual void OnJumpAnimationEnd(void) override;

private:
	struct InitContext
	{
		Lumine* owner;
	};
	void AddAnimation(char* animPath, char* animName, AnimClipName clipName);
	void LoadWeapon(char* modelPath, char* modelName);
	void SetupAnimStateMachine(void);
	void InitAnimInfo(void) override;

	void FaceToNearestEnemy(void);
	void UpdateWeapon(void);

	SwordTrail* swordTrail;
	WaterFluidParticleRenderer* m_waterFluid = nullptr;

	GameObject<SkinnedMeshModelInstance> m_weapon;
	AnimStateMachine* m_stateMachine;
	float m_playAnimSpeed;
	float weaponOnBackTimer;
	float weaponOnHandTimer;
	bool weaponOnBack;
	bool storeWeaponOnBack;
	bool storeWeapon;
	FBXLoader& m_fbxLoader = FBXLoader::get_instance();
	InputManager& m_inputManager = InputManager::get_instance();
};