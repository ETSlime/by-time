#include "Projectile.h"

Projectile::Projectile(ProjectileType projectileType, Transform transform, XMFLOAT3 dir, float speed)
{
	m_projectileAttributes.projectileType = projectileType;
	instance.use = true;		// true:生きてる

	m_projectileAttributes.initTrans.pos = transform.pos;
	m_projectileAttributes.initTrans.rot = transform.rot;
	m_projectileAttributes.initTrans.scl = transform.scl;
	m_projectileAttributes.dir = dir;
	m_projectileAttributes.speed = speed;
	m_projectileAttributes.duration = 3;

	Initialize();

}

Projectile::~Projectile()
{

}

void Projectile::Initialize(void)
{
	instance.transform.pos = m_projectileAttributes.initTrans.pos;
	instance.transform.rot = m_projectileAttributes.initTrans.rot;
	instance.transform.scl = m_projectileAttributes.initTrans.scl;

	instance.castShadow = false; // 影を落とす
}

void Projectile::SetActive(bool enable)
{
	instance.collider.enable = enable;
	instance.attributes.use = enable;
}

void Projectile::Update(void)
{
	if (!instance.load)
	{
		GameObject::Update();

		if (instance.load)
			instance.collider.enable = true;

		return;
	}

	if (instance.use == true)		// このエネミーが使われている？
	{

		//instance.transform.pos += m_projectileAttributes.dir; //* m_projectileAttributes.speed;

		float speed = m_projectileAttributes.speed * timer.GetScaledDeltaTime();

		//instance.transform.pos.x += (cos(m_projectileAttributes.dir.x) * cos(m_projectileAttributes.dir.y) * speed);
		//instance.transform.pos.z += (sin(m_projectileAttributes.dir.x) * cos(m_projectileAttributes.dir.y) * speed);
		//instance.transform.pos.y += (sin(m_projectileAttributes.dir.y) * speed);
		instance.transform.pos.x += m_projectileAttributes.dir.x * speed;
		instance.transform.pos.y += m_projectileAttributes.dir.y * speed;
		instance.transform.pos.z += m_projectileAttributes.dir.z * speed;


		GameObject::Update();

		m_projectileAttributes.duration -= timer.GetDeltaTime();

		if (m_projectileAttributes.duration < 0) {
			SetActive(false);
			//SetUse(false);
		}
		

		//instance.transform.rot.y = 10 * timer.GetScaledDeltaTime();

	}
}

void Projectile::Draw(void)
{
	GameObject::Draw();
}




