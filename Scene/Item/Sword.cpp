#include "Sword.h"
#include "Scene/Ground.h"

Sword::Sword(Transform transform) :Item(ItemType::Weapon, transform)
{

	GameObjectConfig config;
	config.modelPath = "data/MODEL/Item/Key/Key.obj";
	config.collisionType = ObjectCollisionType::COLLIDER_BOUNDING_BOX;
	config.flip = AxisFlip::None;
	config.loadAsync = false;
	config.colliderTag = ColliderTag::ITEM;
	config.scale = XMFLOAT3(5.0f, 5.0f, 5.0f);
	config.modelType = ModelType::Default;
	config.rotation = XMFLOAT3(90.0f, 0.0f, 0.0f);
	Instantiate(config);

	instance.collider.enable = false;

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
