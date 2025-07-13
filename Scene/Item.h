#pragma once

#include "Scene/GameObject.h"
#include "Utility/SimpleArray.h"
#include "Core/Timer.h"

enum class ItemType
{
	Weapon,
	GB,
	Key,
	Interactable,// Interactable‚Ì•û‚ÍŽè‚É“ü‚ê‚È‚¢
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

	virtual void SetActive(bool enable);

	virtual void Interact();

protected:

	struct InitContext {
		Item* owner;
	};

	ItemAttributes m_itemAttributes;

};