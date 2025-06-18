#pragma once

#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Core/Timer.h"

enum class ItemType
{
	Weapon,
	GB,
	Key,
	Max
};

struct ItemAttributes
{
	bool used;
	ItemType itemType;

	Transform	initTrans;
};


class Item : public GameObject<ModelInstance> {

public:

	Item(ItemType itemType, Transform trans);
	~Item();

	virtual void Initialize(void);
	inline const ItemAttributes& GetItemAttributes(void) { return m_itemAttributes; }

	void Update(void) override;
	void Draw(void) override;

	void Use(void);

	void SetActive(bool enable);


protected:

	ItemAttributes m_itemAttributes;

};