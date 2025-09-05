#include "Scene/Weapon/Projectile.h"
#include "Effects/DecalRenderer.h"

Projectile::Projectile(ProjectileType projectileType, Transform transform, XMFLOAT3 dir, float speed)
{
	m_projectileAttributes.projectileType = projectileType;
	instance.use = true;		// true:ê∂Ç´ÇƒÇÈ

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

	instance.castShadow = true; // âeÇóéÇ∆Ç∑

	instance.collider.AddCollisionCallback(OnHitGroundCallback, this, CollisionCallbackID::ProjectileHit);
}

void Projectile::SetActive(bool enable)
{
	instance.collider.enable = enable;
	instance.castShadow = enable;
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

	if (instance.use == true)
	{
		float speed = m_projectileAttributes.speed * m_timer.GetScaledDeltaTime();

		instance.transform.pos.x += m_projectileAttributes.dir.x * speed;
		instance.transform.pos.y += m_projectileAttributes.dir.y * speed;
		instance.transform.pos.z += m_projectileAttributes.dir.z * speed;


		GameObject::Update();

		m_projectileAttributes.duration -= m_timer.GetDeltaTime();

		if (m_projectileAttributes.duration < 0) {
			SetActive(false);
		}
		

		//instance.transform.rot.y = 10 * timer.GetScaledDeltaTime();

	}
}

void Projectile::Draw(void)
{
	GameObject::Draw();
}

void Projectile::OnHitGroundCallback(const CollisionEvent& event, void* context)
{
	Projectile* self = static_cast<Projectile*>(context);
	if (self == nullptr || !self->instance.use || !self->instance.load)
		return;

	self->SetActive(false);
	if (!event.additionalData) return;

	// è’ìÀÇµÇΩñ ÇÃñ@ê¸
	XMFLOAT3 hitNormal = *static_cast<const XMFLOAT3*>(event.additionalData);

	DecalRenderer::get_instance().AddDecal(self->m_projectileAttributes.potentialHitPos, hitNormal);
}




