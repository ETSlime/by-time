#include "Sword.h"

Sword::Sword(Transform transform) :Item(ItemType::Weapon, transform)
{
	SkinnedGameObjectConfig config;
	config.modelPath = "data/MODEL/item/Key";
	config.modelName = "key.fbx";
	config.modelType = SkinnedModelType::Weapon;
	config.drawWorldAABB = false;
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.colliderTag = ColliderTag::ITEM;
	Instantiate(config);
}

Sword::~Sword()
{


}

void Sword::Update(void)
{
	Item::Update();
}

void Sword::Draw(void)
{
	Item::Draw();
}
