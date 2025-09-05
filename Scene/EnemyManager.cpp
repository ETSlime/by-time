//=============================================================================
//
// EnemyManager処理 [EnemyManager.cpp]
// Author : 
//
//=============================================================================
#include "Scene/EnemyManager.h"
#include "Scene/Character/Hilichurl.h"
#include "Scene/Character/GBMonster.h"
#include "Scene/Character/Enemy_NPC.h"

EnemyManager::EnemyManager()
{
	m_player = nullptr;
}

void EnemyManager::Init(const Player* player)
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // DEBUG

	m_player = player;
	Transform trans;


	//for (int i = 0; i < 25; i++)
	//{
	//	float radius = 20000.f;
	//	float r = GetRandFloat(2000.0f, radius);
	//	float angle = GetRandFloat(0.0f, 2.0f * 3.14159265359f);
	//	float x = trans.pos.x + r * cos(angle);
	//	float z = trans.pos.z + r * sin(angle);
	//	Transform t = trans;
	//	t.pos.x = x;
	//	t.pos.z = z;

	//	SpawnEnemy(EnemyType::Hilichurl, t, EnemyState::IDLE);
	//}

	trans.pos = XMFLOAT3(4024.0f, 6270.0f, 3453.0f);
	trans.scl = GBMONSTER_SIZE;
	SpawnEnemy(EnemyType::GBMonster, trans, EnemyState::IDLE, true);


	trans.pos = XMFLOAT3(6846.0f, 5919.0f, 4453.0f);
	trans.scl = XMFLOAT3(1.0f, 1.0f, 1.0f);
	trans.rot = XMFLOAT3(0.0f, -0.8f, 0.0f);
	SpawnEnemy(EnemyType::NPC_Fish, trans, EnemyState::IDLE);

	trans.pos = XMFLOAT3(7900.0f, 5881.0f, 4453.0f);
	trans.scl = XMFLOAT3(1.0f, 1.0f, 1.0f);
	trans.rot = XMFLOAT3(0.0f, 1.0f, 0.0f);
	SpawnEnemy(EnemyType::NPC_Fish, trans, EnemyState::IDLE);

	trans.pos = XMFLOAT3(7222.0f, 5956.0f, 2916.0f);
	trans.scl = XMFLOAT3(1.0f, 1.0f, 1.0f);
	trans.rot = XMFLOAT3(0.0f, 2.2f, 0.0f);
	SpawnEnemy(EnemyType::NPC_Fish, trans, EnemyState::IDLE);

	trans.pos = XMFLOAT3(7673.0f, 5956.0f, 1527.0f);
	trans.scl = XMFLOAT3(1.0f, 1.0f, 1.0f);
	trans.rot = XMFLOAT3(0.0f, 2.2f, 0.0f);
	SpawnEnemy(EnemyType::NPC_Fish, trans, EnemyState::IDLE);

	//trans.pos = XMFLOAT3(12404.5f, -2110.0f, -19177.56f);
	//SpawnEnemy(EnemyType::Hilichurl, trans, EnemyState::HILI_DANCE);

	//trans.pos = XMFLOAT3(12227.0f, -2384.0f, -19485.0f);
	//SpawnEnemy(EnemyType::Hilichurl, trans, EnemyState::HILI_DANCE);

	//trans.pos = XMFLOAT3(12404.5f, -2384.0f, -19792.43f);
	//SpawnEnemy(EnemyType::Hilichurl, trans, EnemyState::HILI_DANCE);

	//trans.pos = XMFLOAT3(12759.5f, -2384.0f, -19792.43f);
	//SpawnEnemy(EnemyType::Hilichurl, trans, EnemyState::HILI_DANCE);
}



Enemy* EnemyManager::SpawnEnemy(EnemyType enemyType, Transform trans, EnemyState initState, bool initialMove)
{
	Enemy* enemy = nullptr;
	switch (enemyType)
	{
	case EnemyType::Hilichurl:
		enemy = new Hilichurl(trans, initState);
		break;
	case EnemyType::GBMonster:
		enemy = new GBMonster(trans, initState);
		break;
	case EnemyType::NPC_Fish:
		enemy = new Enemy_NPC(trans, EnemyType::NPC_Fish, initState);
		break;
	default:
		return nullptr;
	}

	enemy->GetEnemyAttribute().initialMove = initialMove;

	if (!enemyLoadMap.contains(TO_UINT64(enemyType)))
	{
		enemyLoadMap[TO_UINT64(enemyType)] = 0;
		m_totalEnemyLoadCnt++;
	}

	enemy->SetPlayer(m_player);
	enemy->SetDrawWorldAABB(m_drawBoundingBox);
	m_enemyList.push_back(enemy);

	return enemy;
}

void EnemyManager::InitializeEnemyStatus(void)
{
	Node<Enemy*>* cur = m_enemyList.getHead();
	while (cur != nullptr)
	{
		cur->data->Initialize();
		cur = cur->next;
	}
}

void EnemyManager::Draw(void)
{
	// カリング無効
	m_renderer.SetCullingMode(CULL_MODE_NONE);

	Node<Enemy*>* cur = m_enemyList.getHead();
	while (cur != nullptr)
	{
		// モデル描画
		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
			m_renderer.SetRenderProgress(cur->data->GetInstance()->renderProgress);

		if (!cur->data->GetEnemyAttribute().isDead)
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

void EnemyManager::DrawUI(EnemyUIType type)
{
	Node<Enemy*>* cur = m_enemyList.getHead();
	while (cur != nullptr)
	{
		// モデル描画
		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
			m_renderer.SetRenderProgress(cur->data->GetInstance()->renderProgress);

		if (!cur->data->GetEnemyAttribute().isDead)
			cur->data->DrawUI(type);

		if (cur->data->GetInstance()->renderProgress.progress < 1.0f)
		{
			RenderProgressCBuffer defaultRenderProgress;
			defaultRenderProgress.isRandomFade = false;
			defaultRenderProgress.progress = 1.0f;
			m_renderer.SetRenderProgress(defaultRenderProgress);
		}

		cur = cur->next;
	}
}

void EnemyManager::Update(void)
{
	Node<Enemy*>* cur = m_enemyList.getHead();
	while (cur != nullptr)
	{
		cur->data->Update();

		if (m_currentEnemyLoadCnt < m_totalEnemyLoadCnt)
		{
			EnemyType enemyType = cur->data->GetEnemyAttribute().enemyType;
			if (enemyLoadMap.contains(TO_UINT64(enemyType)))
			{
				if (cur->data->GetLoad() && enemyLoadMap[TO_UINT64(enemyType)] == 0)
				{
					enemyLoadMap[TO_UINT64(enemyType)] = 1;
					m_currentEnemyLoadCnt++;
				}
			}
		}

		if (cur->data->GetUse() == FALSE)
		{
			Node<Enemy*>* toDelete = cur;
			cur = cur->next;
			SAFE_DELETE(toDelete->data);
			m_enemyList.remove(toDelete);
		}
		else
			cur = cur->next;
	}

#if _DEBUG
	if (debugEnemy)
	{
		debugEnemy->SetPosition(XMFLOAT3(debugEnemyPosX, debugEnemyPosY, debugEnemyPosZ));
		debugEnemy->SetScale(XMFLOAT3(debugEnemyScaleX, debugEnemyScaleY, debugEnemyScaleZ));
		debugEnemy->SetRotation(XMFLOAT3(0.0f, debugEnemyRotY, 0.0f));
	}
#endif // _DEBUG

}

void EnemyManager::RenderImGui(void)
{
	if (ImGui::Checkbox("Draw Enemy Bounding Box", &m_drawBoundingBox))
	{
		Node<Enemy*>* cur = m_enemyList.getHead();
		while (cur != nullptr)
		{
			cur->data->SetDrawWorldAABB(m_drawBoundingBox);
			cur = cur->next;
		}
	}

	if (ImGui::CollapsingHeader("Debug Enemy"))
	{
		ImGui::SliderFloat("PosX: ", &debugEnemyPosX, 1133.67f, 11111.67f, "%.1f");
		ImGui::SliderFloat("PosY: ", &debugEnemyPosY, 1131.0f, 11111.0f, "%.1f");
		ImGui::SliderFloat("PosZ: ", &debugEnemyPosZ, 1418.0f, 11111.0f, "%.1f");
		ImGui::SliderFloat("ScaleX: ", &debugEnemyScaleX, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("ScaleY: ", &debugEnemyScaleY, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("ScaleZ: ", &debugEnemyScaleZ, 0.1f, 5.0f, "%.1f");
		ImGui::SliderFloat("RotY: ", &debugEnemyRotY, -3, 3, "%.1f");
	}
}