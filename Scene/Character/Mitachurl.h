#pragma once
//=============================================================================
//
// ÉÇÉfÉãèàóù [Mitachurl.h]
// Author : 
//
//=============================================================================
#include "GameObject.h"

class Mitachurl : public GameObject<SkinnedMeshModelInstance>, public ISkinnedMeshModelChar
{
public:
	Mitachurl();
	~Mitachurl();
	void AddAnimation(char* animPath, char* animName, AnimClipName clipName);
	void LoadWeapon(char* modelPath, char* modelName);

	void Update(void) override;
	void Draw(void) override;

	void SetupAnimStateMachine();

	AnimStateMachine* GetStateMachine(void) override;

	void PlayWalkAnim(void) override;
	void PlayRunAnim(void) override;
	void PlayIdleAnim(void) override;

	void PlayAttackAnim(void);

	virtual bool CanWalk(void) const override;
	virtual bool CanStopMoving() const override;
	virtual bool CanAttack() const override;
	virtual bool CanRun(void) const override;

	virtual void OnAttackAnimationEnd(void) override;
	virtual void OnDashAnimationEnd(void) override;

private:
	GameObject<SkinnedMeshModelInstance> weapon;
	AnimStateMachine* stateMachine;
	float playAnimSpeed;
	FBXLoader& fbxLoader = FBXLoader::get_instance();
};