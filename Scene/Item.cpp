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

	instance.collider.enable = true;
	instance.castShadow = false; // 影を落とす
}

void Item::Draw(void)
{
	GameObject::Draw();
}

void Item::Update(void)
{
	if (!instance.load)
	{
		GameObject::Update();
		return;
	}

	if (instance.use == true)		// このエネミーが使われている？
	{

		GameObject::Update();

		//instance.transform.rot.y = 200 * timer.GetScaledDeltaTime();

	}

}
