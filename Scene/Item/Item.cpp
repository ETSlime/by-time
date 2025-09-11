#include "Scene/Item/Item.h"

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
	case ItemType::Interactable:
		config.modelPath = "data/MODEL/Item/Vines/vines.obj";
		config.scale = XMFLOAT3(20.0f, 20.0f, 20.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntSoil:
		config.modelPath = "data/MODEL/Item/Soil/soil.obj";
		config.scale = XMFLOAT3(20.0f, 20.0f, 20.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntPSeed:
		config.modelPath = "data/MODEL/Item/Seed/Seed.obj";
		config.scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
		config.rotation = XMFLOAT3(90.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntVines:
		config.modelPath = "data/MODEL/Item/Vines/vines.obj";
		config.scale = XMFLOAT3(20.0f, 20.0f, 20.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntAutumnDoorL:
		config.modelPath = "data/MODEL/Item/Door/door_left.obj";
		config.scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntAutumnDoorR:
		config.modelPath = "data/MODEL/Item/Door/door_right.obj";
		config.scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	case ItemType::IntGate:
		config.modelPath = "data/MODEL/Item/Gate/final_door.obj";
		config.scale = XMFLOAT3(2.0f, 2.0f, 2.0f);
		config.rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
		break;
	}

	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ITEM;
	config.modelType = ModelType::Default;

	if (m_itemAttributes.itemType >= ItemType::Interactable)
		config.colliderTag = ColliderTag::DEFAULT;

	Instantiate(config);

	instance.transform.pos = m_itemAttributes.initTrans.pos;
	instance.transform.rot = m_itemAttributes.initTrans.rot;
	instance.transform.scl = m_itemAttributes.initTrans.scl;

	m_itemAttributes.initialized = true;

	instance.collider.AddCollisionCallback(OnItemPickupCallback, this, CollisionCallbackID::PickUpItem);
}

void Item::Draw(void)
{
	GameObject::Draw();
}

void Item::Use(void)
{
	m_itemAttributes.used = true;
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

void Item::OnItemPickupCallback(const CollisionEvent& event, void* context)
{
	Item* self = static_cast<Item*>(context);
	if (self == nullptr || !self->instance.use || !self->instance.load)
		return;

	self->Pickup();
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