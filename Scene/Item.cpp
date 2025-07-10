#include "Item.h"

Item::Item(ItemType itemType, Transform trans)
{
	m_itemAttributes.itemType = itemType;
	instance.use = true;		// true:生きてる

	m_itemAttributes.initTrans.pos = trans.pos;
	m_itemAttributes.initTrans.rot = trans.rot;
	m_itemAttributes.initTrans.scl = trans.scl;
}

Item::~Item()
{
}

void Item::Initialize(void)
{

	instance.transform.pos = m_itemAttributes.initTrans.pos;
	instance.transform.rot = m_itemAttributes.initTrans.rot;
	instance.transform.scl = m_itemAttributes.initTrans.scl;

	instance.castShadow = false; // 影を落とす
}

void Item::Draw(void)
{
	GameObject::Draw();
}

void Item::Use(void)
{
	instance.collider.enable = false;
	m_itemAttributes.used = true;
}

void Item::SetActive(bool enable)
{
	instance.collider.enable = enable;
	instance.attributes.use = enable;
}

void Item::Interact()
{

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

	if (instance.use == true)		// このエネミーが使われている？
	{

		GameObject::Update();

		instance.transform.rot.y = 10 * timer.GetScaledDeltaTime();

	}

}
