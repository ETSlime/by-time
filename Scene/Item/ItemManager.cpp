#include "Scene/Item/ItemManager.h"
#include "Scene/Item/Item.h"
#include "Scene/Ground.h"
#include "Core/SaveSystem.h"



void ItemManager::Init(void)
{

}

void ItemManager::InitializeItemStatus(void)
{

	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		cur->data->Initialize();
		cur = cur->next;
	}
}

void ItemManager::Draw(void)
{
	// TODO もっと綺麗にドロウしましょう
	// カリング無効
	m_renderer.SetCullingMode(CULL_MODE_NONE);
	
	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		// モデル描画
		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
			m_renderer.SetRenderProgress(cur->data->GetInstance()->renderProgress);

		if (cur->data->GetAttributesConst().use)
			cur->data->Draw();

		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
		{
			RenderProgressCBuffer defaultRenderProgress;
			defaultRenderProgress.isRandomFade = false;
			defaultRenderProgress.progress = 1.0f;
			m_renderer.SetRenderProgress(defaultRenderProgress);
		}

		cur = cur->next;
	}

	// カリング設定を戻す
	m_renderer.SetCullingMode(CULL_MODE_BACK);
}

void ItemManager::Update(void)
{
	// TODO もっと綺麗にアップデートしましょう
	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		cur->data->Update();

		if (m_currentItemLoadCnt < m_totalItemLoadCnt)
		{
			ItemType itemType = cur->data->GetItemAttributes().itemType;
			if (itemLoadMap.contains(TO_UINT64(itemType)))
			{
				if (cur->data->GetLoad() && itemLoadMap[TO_UINT64(itemType)] == 0)
				{
					itemLoadMap[TO_UINT64(itemType)] = 1;
					m_currentItemLoadCnt++;
				}
			}
		}

		if (cur->data->GetDestroy())
		{
			Node<Item*>* toDelete = cur;
			cur = cur->next;
			// アイテムを削除
			toDelete->data->Destroy();
			m_itemList.remove(toDelete);
		}
		else
			cur = cur->next;
	}

}

Item* ItemManager::SpawnItem(ItemType itemType, const Transform& trans, bool enableCollider)
{

	Item* item = new Item(itemType, trans);
	item->Initialize(enableCollider);

	if (!itemLoadMap.contains(TO_UINT64(itemType)))
	{
		itemLoadMap[TO_UINT64(itemType)] = 0;
		m_totalItemLoadCnt++;
	}

	m_itemList.push_back(item);

	return item;
}

Item* ItemManager::GetItem(ItemType itemType) const
{
	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		if (cur->data->GetItemType() == itemType)
		{
			return cur->data;
		}
		cur = cur->next;
	}
	return nullptr;
}

Item* ItemManager::GetPickedItem(ItemType itemType) const
{
	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		if (cur->data->GetItemType() == itemType && cur->data->GetPickedUp() && !cur->data->GetUsed())
		{
			return cur->data;
		}
		cur = cur->next;
	}
	return nullptr;
}

void ItemManager::LoadSaveData(const SaveData& saveData)
{
	for (int i = 0; i < saveData.seedCount; i++)
	{
		Transform trans;
		trans.scl = XMFLOAT3(SEED_SIZE, SEED_SIZE, SEED_SIZE);
		Item* seed = SpawnItem(ItemType::Seed, trans, false);
		seed->Pickup();
	}
}

void ItemManager::RenderImGui(void)
{


}
