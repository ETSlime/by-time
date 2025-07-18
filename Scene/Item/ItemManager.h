#pragma once
//=============================================================================
//
// ƒAƒCƒeƒ€‚½‚¿‚Ì‚Ü‚Æ‚ß–ð‚¿‚á‚ñ [ItemManager.h]
// Author : 
//
//
//=============================================================================
#include "Core/Graphics/ConstantBufferStructs.h"
#include "Scene/Item/Item.h"
#include "Utility/DoubleLinkedList.h"
#include "Utility/SingletonBase.h"

class ItemManager: public SingletonBase<ItemManager>, public IDebugUI {

public:
	void Init(void);
	void InitializeItemStatus(void);
	void Draw(void);
	void Update(void);

	Item* SpawnItem(ItemType itemType, Transform trans);

	int GetTotalItemLoadCnt(void) const { return m_totalItemLoadCnt; }
	int GetItemLoadedCnt(void) const { return m_currentItemLoadCnt; }

private:

	HashMap<uint64_t, int, HashUInt64, EqualUInt64> itemLoadMap =
		HashMap<uint64_t, int, HashUInt64, EqualUInt64>(
			static_cast<int>(ItemType::Max),
			HashUInt64(),
			EqualUInt64()
		);

	int m_totalItemLoadCnt = 0;
	int m_currentItemLoadCnt = 0;

	bool m_drawBoundingBox = true;

	DoubleLinkedList<Item*> m_itemList;

	Renderer& m_renderer = Renderer::get_instance();

	virtual void RenderImGui(void) override;


};