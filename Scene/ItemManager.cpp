#include "ItemManager.h"
#include "Scene/Item/Sword.h"
#include "Scene/Ground.h"

void ItemManager::Init(void)
{

}

void ItemManager::InitializeItemStatus(void)
{

	Node<Item*>* cur = m_itemList.getHead();
	while (cur != nullptr)
	{
		cur->data->Initialize();
		//cur->data->SetDrawWorldAABB(true);
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
			RenderProgressBuffer defaultRenderProgress;
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
	switch (itemType)
	{
	case ItemType::Weapon:
		item = new Sword(trans);
		break;
	default:
		return nullptr;
	}

	if (!itemLoadMap.contains(TO_UINT64(itemType)))
	{
		itemLoadMap[TO_UINT64(itemType)] = 0;
		m_totalItemLoadCnt++;
	}

	//item->SetDrawWorldAABB(m_drawBoundingBox);
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
