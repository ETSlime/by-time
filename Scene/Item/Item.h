#pragma once

#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Core/Timer.h"

enum class ItemType
{
	Weapon,
	GB,
	Key,
	Seed,
	Max
};

struct ItemAttributes
{
	bool used;
	bool pickedUp;
	bool initialized;
	ItemType itemType;

	Transform initTrans;
};


class Item : public GameObject<ModelInstance> {

public:

	Item(ItemType itemType, Transform trans);
	~Item();

	virtual void Initialize(void);
	inline const ItemAttributes& GetItemAttributes(void) { return m_itemAttributes; }

	void Update(void) override;
	void Draw(void) override;

	void Use(void) {};
	void Pickup(void);
	bool GetUsed(void) { return m_itemAttributes.used; }
	bool GetPickedUp(void) { return m_itemAttributes.pickedUp; }
	void SetActive(bool enable);
	void SetColliderEnable(bool enable) { instance.collider.enable = enable; }
	ItemType GetItemType(void) const { return m_itemAttributes.itemType; }

protected:
	// プレイヤーがアイテムを拾ったときのコールバック
	static void OnItemPickupCallback(const CollisionEvent& event, void* context);
	ItemAttributes m_itemAttributes;

};