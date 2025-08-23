#include "Scene/Item/Item.h"
#include "Effects/EffectSystem.h"
#include "Effects/SpecialEffects/FireBallEffectRenderer.h"

Item::Item(ItemType itemType, Transform trans)
{
	m_itemAttributes.itemType = itemType;
	instance.use = true;		// true:¶‚«‚Ä‚é

	m_itemAttributes.initTrans.pos = trans.pos;
	m_itemAttributes.initTrans.rot = trans.rot;
	m_itemAttributes.initTrans.scl = trans.scl;
}

Item::~Item()
{
}

void Item::Initialize(void)
{
	if (m_itemAttributes.initialized)
		return;

	GameObjectConfig config;

	switch (m_itemAttributes.itemType)
	{
	case ItemType::Key:
		config.modelPath = "data/MODEL/Item/Key/Key.obj";
		config.scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
		config.rotation = XMFLOAT3(90.0f, 0.0f, 0.0f);
		break;
	case ItemType::Seed:
		config.modelPath = "data/MODEL/Item/Seed/Seed.obj";
		config.scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
		config.rotation = XMFLOAT3(90.0f, 0.0f, 0.0f);
		break;
	}

	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ITEM;
	config.modelType = ModelType::Default;

	Instantiate(config);

	instance.transform.pos = m_itemAttributes.initTrans.pos;
	instance.transform.rot = m_itemAttributes.initTrans.rot;
	instance.transform.scl = m_itemAttributes.initTrans.scl;

	m_itemAttributes.initialized = true;

	instance.collider.AddCollisionCallback(OnItemCollisionCallback, this, CollisionCallbackID::Item);
}

void Item::Draw(void)
{
	GameObject::Draw();
}

void Item::PlayEffect(HitColliderType type)
{
	if (m_itemAttributes.itemType == ItemType::Seed)
	{
		ParticleEffectParams smokeParams;
		smokeParams.type = EffectType::Smoke;
		smokeParams.duration = 7.5f;
		smokeParams.position = instance.transform.pos;
		smokeParams.position.y += 80.0f; // ­‚µã‚É”z’u
		smokeParams.scale = 250.0f;
		smokeParams.acceleration = XMFLOAT3(0.0f, 15.0f, 0.0f);
		smokeParams.spawnRateMin = 2.0f;
		smokeParams.spawnRateMax = 5.0f;
		smokeParams.lifeMin = 2.0f;
		smokeParams.lifeMax = 4.0f;
		smokeParams.startColor = XMFLOAT4(0.6f, 0.6f, 0.6f, 0.3f); // ‰ŒF

		if (type == HitColliderType::Enemy)
		{
			smokeParams.startColor = XMFLOAT4(0.8f, 0.2f, 0.2f, 0.3f); // “G‚É“–‚½‚Á‚½Žž‚ÍÔ‚¢‰Œ
			smokeParams.duration = 3.0f; // ’Z‚¢ŽžŠÔ‚ÅÁ‚¦‚é
			smokeParams.lifeMin = 1.5f;
			smokeParams.lifeMax = 2.0f;
		}

		IEffectRenderer* smoke = EffectSystem::get_instance().SpawnParticleEffect(smokeParams);

		FireBallEffectParams fireparams;
		fireparams.type = EffectType::FireBall;
		fireparams.duration = 5.0f;
		fireparams.position = instance.transform.pos;
		fireparams.scale = 43.0f;
		fireparams.lifeMin = 1.0f;
		fireparams.lifeMax = 1.0f;
		fireparams.spawnRateMin = 0.0f;
		fireparams.spawnRateMax = 10.0f;
		fireparams.acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
		fireparams.startColor = XMFLOAT4(1.0f, 1.0f, 0.3f, 1.0f);
		fireparams.endColor = XMFLOAT4(1.2f, 0.5f, 0.0f, 0.0f);
		fireparams.tilesX = FIREBALL_ANIM_TILE_X;
		fireparams.tilesY = FIREBALL_ANIM_TILE_Y;
		fireparams.coneAngleDegree = 25.0f;
		fireparams.coneRadius = 0.6f;
		fireparams.coneLength = 5.0f;
		fireparams.frameLerpCurve = 1.0f;
		fireparams.rotationSpeed = 0.0f;
		fireparams.startSpeedMin = 2.0f;
		fireparams.startSpeedMax = 3.0f;

		if (type == HitColliderType::Enemy)
		{
			fireparams.startColor = XMFLOAT4(1.0f, 0.5f, 0.2f, 1.0f); // “G‚É“–‚½‚Á‚½Žž‚ÍÔ‚¢‰Î‰Š
			fireparams.endColor = XMFLOAT4(1.0f, 0.2f, 0.2f, 0.0f); // ’Z‚¢Žõ–½
			fireparams.duration = 1.5f; // ’Z‚¢ŽžŠÔ‚ÅÁ‚¦‚é
		}

		IEffectRenderer* effect = EffectSystem::get_instance().SpawnParticleEffect(fireparams);
	}

}

void Item::Pickup(void)
{
	SetActive(false);
	m_itemAttributes.pickedUp = true;
}

void Item::SetActive(bool enable)
{
	instance.collider.enable = enable;
	instance.attributes.use = enable;
	instance.castShadow = enable;
}

void Item::OnItemCollisionCallback(const CollisionEvent& event, void* context)
{
	if (event.colliderA->tag == ColliderTag::PLAYER || event.colliderB->tag == ColliderTag::PLAYER)
	{
		Item* self = static_cast<Item*>(context);
		if (self == nullptr || !self->instance.use || !self->instance.load || self->GetIsThrew())
			return;

		self->Pickup();
	}
	else if (event.colliderA->tag == ColliderTag::ENEMY || event.colliderB->tag == ColliderTag::ENEMY)
	{
		Item* self = static_cast<Item*>(context);
		if (self == nullptr || !self->instance.use || !self->instance.load || !self->GetIsThrew())
			return;

		self->PlayEffect(HitColliderType::Enemy);
		self->SetDestroy(true);
	}

}

void Item::Update(void)
{
	if (!instance.load)
	{
		GameObject::Update();

		if(instance.load)
			instance.collider.enable = true;

		return;
	}

	if (instance.use == true)
	{

		GameObject::Update();

	}

}
