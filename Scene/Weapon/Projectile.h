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
};

class Projectile : public GameObject<ModelInstance> {

public:

	Projectile(ProjectileType projectileType, Transform transform, XMFLOAT3 dir, float speed);
	~Projectile();

	virtual void Initialize(void);

	void SetActive(bool enable);

	void Update(void) override;
	void Draw(void) override;

	inline const ProjectileAttributes& GetProjectileAttributes(void) { return m_projectileAttributes; }

protected:

	ProjectileAttributes m_projectileAttributes;


};