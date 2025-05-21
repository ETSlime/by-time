#pragma once
//=============================================================================
//
// ÉÇÉfÉãèàóù [Lumine.h]
// Author : 
//
//=============================================================================
#include "Scene/GameObject.h"
#include "Effects/SwordTrail.h"
#include "Utility/InputManager.h"

class Lumine : public GameObject<SkinnedMeshModelInstance>, public ISkinnedMeshModelChar
{
public:
	Lumine();
	~Lumine();
	void AddAnimation(char* animPath, char* animName, AnimClipName clipName);
	void LoadWeapon(char* modelPath, char* modelName);

	void Update(void) override;
	void Draw(void) override;
	void DrawEffect(void) override;

	void SetupAnimStateMachine(void);
	void InitAnimInfo(void) override;

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

	void FaceToNearestEnemy(void);
	void UpdateWeapon(void);

	SwordTrail* swordTrail;

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