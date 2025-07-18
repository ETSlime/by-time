#include "ProjectileManager.h"
#include "Projectile.h"
#include "Bullet.h"

void ProjectileManager::Init(void)
{



}

void ProjectileManager::InitializeObjectsStatus(void)
{
	Node<Projectile*>* cur = m_projectileList.getHead();
	while (cur != nullptr)
	{
		cur->data->Initialize();
		//cur->data->SetDrawWorldAABB(true);
		cur = cur->next;
	}
}

void ProjectileManager::Draw(void)
{

	// TODO もっと綺麗にドロウしましょう
	// カリング無効
	m_renderer.SetCullingMode(CULL_MODE_NONE);

	Node<Projectile*>* cur = m_projectileList.getHead();
	while (cur != nullptr)
	{
		// モデル描画
		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
			m_renderer.SetRenderProgress(cur->data->GetInstance()->renderProgress);

		if (cur->data->GetAttributes().use)
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

void ProjectileManager::Update(void)
{

	// TODO もっと綺麗にアップデートしましょう
	Node<Projectile*>* cur = m_projectileList.getHead();
	while (cur != nullptr)
	{
		cur->data->Update();

		if (m_currentProjectileLoadCnt < m_totalProjectileLoadCnt)
		{
			ProjectileType projType = cur->data->GetProjectileAttributes().projectileType;
			if (projectileLoadMap.contains(TO_UINT64(projType)))
			{
				if (cur->data->GetLoad() && projectileLoadMap[TO_UINT64(projType)] == 0)
				{
					projectileLoadMap[TO_UINT64(projType)] = 1;
					m_currentProjectileLoadCnt++;
				}
			}
		}

		if (cur->data->GetUse() == FALSE)
		{
			Node<Projectile*>* toDelete = cur;
			cur = cur->next;
			SAFE_DELETE(toDelete->data);
			m_projectileList.remove(toDelete);
		}
		else
			cur = cur->next;
	}

}

Projectile* ProjectileManager::SpawnProjectile(ProjectileType projectileType, Transform trans, XMFLOAT3 dir, float speed)
{
	Projectile* projectile;

	switch (projectileType)
	{
	case ProjectileType::Bullet:
		projectile = new Bullet(trans, dir, speed);
		break;
	default:
		return nullptr;
	}

	if (!projectileLoadMap.contains(TO_UINT64(projectileType)))
	{
		projectileLoadMap[TO_UINT64(projectileType)] = 0;
		m_totalProjectileLoadCnt++;
	}

	//item->SetDrawWorldAABB(m_drawBoundingBox);
	m_projectileList.push_back(projectile);

	return projectile;
}

void ProjectileManager::RenderImGui(void)
{
	if (ImGui::Checkbox("Draw Enemy Bounding Box", &m_drawBoundingBox))
	{
		Node<Projectile*>* cur = m_projectileList.getHead();
		while (cur != nullptr)
		{
			cur->data->SetDrawWorldAABB(m_drawBoundingBox);
			cur = cur->next;
		}
	}
}
