#pragma once

#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Core/Timer.h"

enum class ProjectileType
{
	Bullet,
	Max
};

struct ProjectileAttributes
{
	ProjectileType projectileType;
	Transform	initTrans;
	XMFLOAT3 dir;
	float speed;
	float duration;
	XMFLOAT3 potentialHitPos; // 着弾予測位置
};

class Projectile : public GameObject<ModelInstance> {

public:

	Projectile(ProjectileType projectileType, Transform transform, XMFLOAT3 dir, float speed);
	~Projectile();

	virtual void Initialize(void);

	void SetActive(bool enable);
	void SetPotentialHitPos(XMFLOAT3 pos) { m_projectileAttributes.potentialHitPos = pos; }

	void Update(void) override;
	void Draw(void) override;

	inline const ProjectileAttributes& GetProjectileAttributes(void) { return m_projectileAttributes; }

protected:
	// コリジョンイベントコールバック関数
	static void OnHitGroundCallback(const CollisionEvent& event, void* context);

	ProjectileAttributes m_projectileAttributes;


};