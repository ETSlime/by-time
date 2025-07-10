#include "Interactable.h"

Interactable::Interactable(Transform transform) :Item(ItemType::Weapon, transform)
{
	GameObjectConfig config;
	config.modelPath = "data/MODEL/Item/Key/Key.obj";
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ITEM;
	config.scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
	config.modelType = ModelType::Default;
	config.rotation = XMFLOAT3(90.0f, 0.0f, 0.0f);
	Instantiate(config);

	instance.collider.enable = false;

	/*m_triggerCollider.tag = ColliderTag::INTERACTABLE;
	m_triggerCollider.owner = this;
	m_triggerCollider.enable = true;

	XMFLOAT3 minPoint = m_triggerCollider.aabb.minPoint;
	XMFLOAT3 maxPoint = m_triggerCollider.aabb.maxPoint;

	minPoint.z -= 3;
	maxPoint.z -= 3;

	m_triggerCollider.aabb = BOUNDING_BOX(minPoint, maxPoint);
	CollisionManager::get_instance().RegisterDynamicCollider(&m_triggerCollider);*/
}

Interactable::~Interactable()
{

}

void Interactable::Update(void)
{
	Item::Update();
}

void Interactable::Draw(void)
{
	Item::Draw();
}
