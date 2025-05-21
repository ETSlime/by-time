#pragma once
//=============================================================================
//
// ÉÇÉfÉãèàóù [Sigewinne.h]
// Author : 
//
//=============================================================================
#include "GameObject.h"

class Sigewinne : public GameObject<SkinnedMeshModelInstance>, public ISkinnedMeshModelChar
{
public:
	Sigewinne();
	~Sigewinne();
	void AddAnimation(char* animPath, char* animName, AnimClipName clipName);
	void LoadWeapon(char* modelPath, char* modelName);

	void Update(void) override;
	void Draw(void) override;

	void SetupAnimStateMachine();

	AnimStateMachine* GetStateMachine(void) override;

	void PlayWalkAnim(void) override;
	void PlayRunAnim(void) override;
	void PlayJumpAnim(void) override;
	void PlayIdleAnim(void) override;
	void PlayStandingAnim(void) override;

	void PlayAttackAnim(void) override;
	void PlayHitAnim(void) override;

	virtual bool CanWalk(void) const override;
	virtual bool CanStopMoving() const override;
	virtual bool CanAttack() const override;
	virtual bool CanRun(void) const override;
	virtual bool CanHit(void) const override;

	virtual void OnAttackAnimationEnd(void) override;
	virtual void OnHitAnimationEnd(void) override;

private:
	GameObject<SkinnedMeshModelInstance> weapon;
	AnimStateMachine* stateMachine;
	float playAnimSpeed;
	FBXLoader& fbxLoader = FBXLoader::get_instance();
};