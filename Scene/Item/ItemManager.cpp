#include "Scene/Item/ItemManager.h"
#include "Scene/Item/Item.h"
#include "Scene/Ground.h"
#include "Scene/Item/Interactable.h"

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

		if (!cur->data->GetItemAttributes().used && cur->data->GetAttributes().use)
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

			long id = cur->data->GetID();
			if (itemLoadMap.contains(TO_UINT64(id)))
			{
				if (cur->data->GetLoad() && itemLoadMap[TO_UINT64(id)] == 0)
				{
					itemLoadMap[TO_UINT64(id)] = 1;
					m_currentItemLoadCnt++;
				}
			}
		}

		if (cur->data->GetUse() == FALSE)
		{
			Node<Item*>* toDelete = cur;
			cur = cur->next;
			SAFE_DELETE(toDelete->data);
			m_itemList.remove(toDelete);
		}
		else
			cur = cur->next;
	}


}

Item* ItemManager::SpawnItem(ItemType itemType, Transform trans)
{

	Item* item;
	
	
	// = new Item(itemType, trans);

	if (itemType < ItemType::Interactable) {
	
		item = new Item(itemType, trans);
	}
	else {
	
		item = new Interactable(trans);
	
	}

	item->Initialize();

	if (!itemLoadMap.contains(TO_UINT64(item->GetID())))
	{
		itemLoadMap[TO_UINT64(item->GetID())] = 0;
		m_totalItemLoadCnt++;
	}

	m_itemList.push_back(item);

	return item;
}

void ItemManager::RenderImGui(void)
{

	if (ImGui::Checkbox("Draw Enemy Bounding Box", &m_drawBoundingBox))
	{
		Node<Item*>* cur = m_itemList.getHead();
		while (cur != nullptr)
		{
			cur->data->SetDrawWorldAABB(m_drawBoundingBox);
			cur = cur->next;
		}
	}

}
