//=============================================================================
//
// player処理 [Player.cpp]
// Author : 
//
//=============================================================================

#include "Core/Graphics/Renderer.h"
#include "Scene/Player.h"
#include "Utility/InputManager.h"
#include "Core/Camera.h"
#include "Utility/Debugproc.h"
#include "Collision/OctreeNode.h"
#include "Core/CursorManager.h"
#include "Scene/Weapon/ProjectileManager.h"


//#define PLAYER_INIT_POS		XMFLOAT3(12160.0f, -2110.0f, -20320.0f)

//=============================================================================
// 初期化処理
//=============================================================================
Player::Player()
{
#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // DEBUG

	//sigewinne = new Sigewinne();

	//klee =  new Klee();

	lumine = new Lumine();
	lumine->SetPlayerComponent(this);

	//hilichurl = new Hilichurl();

	//mitachurl = new Mitachurl();

	Transform transform;
	transform.pos = PLAYER_INIT_POS;
	lumine->SetTransform(transform);
	lumine->SetPlayerAttributes(&m_playerAttr);
	m_playerGO = lumine;
	m_playerGO->SetDrawWorldAABB(m_drawBoundingBox);

	m_light = new DirectionalLight();
	m_light->SetEnable(true);
	m_light->BindToTransform(m_playerGO->GetTransformP());
	m_lightMgr.AddLight(m_light);
}

Player::~Player()
{
	SAFE_DELETE(sigewinne);
	SAFE_DELETE(klee);
	SAFE_DELETE(lumine);
}

void Player::Update(void)
{
	if (!m_playerGO->GetInstance()->load)
	{
		m_playerGO->Update();
		return;
	}

	if (m_currentModelLoadCnt < m_totalModelLoadCnt && lumine->LoadFinished())
	{
		m_currentModelLoadCnt++;
	}

	
	m_attributes = m_playerGO->GetAttributes();
	m_transform = m_playerGO->GetTransform();

	if (m_inputManager.GetKeyboardTrigger(KEY_SWITCH_WEAPON))
	{
		m_playerAttr.isMeleeMode = !m_playerAttr.isMeleeMode;
	}

	if (m_attributes.climbMode)
		UpdateClimbMode();
	else
		UpdateNormalMode();
	
	HandlePlayerMove(m_transform);
	m_playerGO->SetTransform(m_transform);

	m_attributes.spd *= pow(SPD_DECAY_RATE,  m_playerGO->timer.GetScaledDeltaTime());
	m_playerGO->UpdateAttributes(m_attributes);
	m_playerGO->Update();

	// プレイヤー視点
	//transform = m_playerGO->GetTransform();
	XMFLOAT3 pos = m_transform.pos;
	pos.y += 70.0f;
	Camera::get_instance().SetCameraAT(pos);
	Camera::get_instance().SetCamera();

#ifdef _DEBUG
	m_debugProc.PrintDebugProc("PosX: %f PosY: %f, PosZ: %f\n", m_transform.pos.x, m_transform.pos.y, m_transform.pos.z);
#endif
}

void Player::Draw(void)
{
	m_playerGO->Draw();
}

void Player::DrawEffect(void)
{
	m_playerGO->DrawEffect();
}

void Player::HandlePlayerMove(Transform& transform)
{
	float deltaDir = m_attributes.targetDir - m_attributes.dir;
	if (deltaDir > XM_PI) deltaDir -= XM_2PI;
	if (deltaDir < -XM_PI) deltaDir += XM_2PI;
	m_attributes.dir += deltaDir * ROTATION_SPEED * m_playerGO->timer.GetScaledDeltaTime();

	transform.rot.y = m_attributes.dir;

	if (m_attributes.isMoveBlocked)
		return;

	// 入力のあった方向へプレイヤーを向かせて移動させる
	float speed = m_attributes.spd;
	if (m_attributes.isRunning)
		speed *= VALUE_RUN_FACTOR;

	if (m_attributes.climbMode)
	{
		transform.pos.y += speed * m_playerGO->timer.GetScaledDeltaTime();
	}
	else
	{
		transform.pos.x += sinf(transform.rot.y) * speed * m_playerGO->timer.GetScaledDeltaTime();
		transform.pos.z += cosf(transform.rot.y) * speed * m_playerGO->timer.GetScaledDeltaTime();
	}


#ifdef _DEBUG
	if (m_inputManager.GetKeyboardPress(DIK_TAB))
	{
		transform.pos.x += sinf(transform.rot.y) * m_attributes.spd * 5 * m_playerGO->timer.GetScaledDeltaTime();
		transform.pos.z += cosf(transform.rot.y) * m_attributes.spd * 5 * m_playerGO->timer.GetScaledDeltaTime();
	}
#endif // DEBUG

}

void Player::RenderImGui(void)
{
	if (ImGui::Checkbox("Draw Player Bounding Box", &m_drawBoundingBox))
	{
		if (m_playerGO)
		{
			m_playerGO->SetDrawWorldAABB(m_drawBoundingBox);
		}
	}
}

Transform Player::GetTransform(void) const
{
	return m_playerGO->GetTransform();
}

void Player::InitializePlayerStatus(void)
{
	if (lumine)
	{
		Transform transform;
		transform.pos = PLAYER_INIT_POS;
		lumine->SetTransform(transform);
		lumine->InitializeStatus();
	}

	Update();
}

void Player::PickItem(Item* obj)
{
	// TODO ちゃんと色々なアイテムが手に入れるように、直す
	// 現在、下のコードがダメだ　動かない　直せ
	if(obj->GetItemAttributes().itemType == ItemType::Weapon)
		m_hasWeapon = true;

	obj->Use();

}

void Player::SetClimbMode(bool enable)
{
	// クライムモードを有効にする
	m_attributes.climbMode = enable;
	// プレイヤーの属性を更新
	m_playerGO->UpdateAttributes(m_attributes);
}

void Player::UpdateNormalMode(void)
{
	UpdateActionQueue();

	if (lumine && m_inputManager.GetKeyboardTrigger(DIK_1))
	{
		lumine->SetTransform(m_transform);
		//lumine->SetRenderProgress(0.0f);
		//lumine->SetSwitchCharEffect(true);
		m_playerGO = lumine;
		m_light->BindToTransform(m_playerGO->GetTransformP());
	}
	if (klee && m_inputManager.GetKeyboardTrigger(DIK_2))
	{
		klee->SetTransform(m_transform);
		//klee->SetRenderProgress(0.0f);
		//klee->SetSwitchCharEffect(true);
		m_playerGO = klee;
		m_light->BindToTransform(m_playerGO->GetTransformP());
	}
	if (sigewinne && m_inputManager.GetKeyboardTrigger(DIK_3))
	{
		sigewinne->SetTransform(m_transform);
		//sigewinne->SetRenderProgress(0.0f);
		//sigewinne->SetSwitchCharEffect(true);
		m_playerGO = sigewinne;
		m_light->BindToTransform(m_playerGO->GetTransformP());
	}

	Camera& camera = Camera::get_instance();

	if (m_inputManager.GetKeyboardPress(DIK_UP))
	{
		m_transform.pos.y += 50;
	}
	if (m_inputManager.GetKeyboardPress(DIK_DOWN))
	{
		m_transform.pos.y -= 50;
	}
#ifdef _DEBUG

#endif

	//if (m_inputManager.IsMouseLeftTriggered() && !CursorManager::get_instance().IsMouseFreeMode())
	//{
	//	if (m_attributes.hasWeapon) {

	//		PlayerAction action;
	//		action.actionType = ActionEnum::ATTACK;
	//		action.liveTime = 0;
	//		m_playerAttr.actionQueue[m_playerAttr.actionQueueEnd] = action;
	//		m_playerAttr.actionQueueEnd = (m_playerAttr.actionQueueEnd + 1) % ACTION_QUEUE_SIZE;

	//		// キューのオーバーフローを防止
	//		if (m_playerAttr.actionQueueEnd == m_playerAttr.actionQueueStart)
	//		{
	//			m_playerAttr.actionQueueStart = (m_playerAttr.actionQueueStart + 1) % ACTION_QUEUE_SIZE; // 最も古いアクションを破棄
	//		}

	//	}
	//}

	if (m_inputManager.GetKeyboardTrigger(DIK_SPACE))
	{
		m_attributes.isJumping = true;
	}

	if (m_inputManager.GetKeyboardTrigger(DIK_F)) {
		if (m_linkedInteractable != nullptr)
		{
			if (m_linkedInteractable->GetItemType() == ItemType::IntSoil && m_attributes.hasSeed) {
				m_linkedInteractable->Use();
				m_linkedInteractable->SetActive(false);
			}
			else if (m_linkedInteractable->GetItemType() == ItemType::IntVines) {
				m_transform.pos.y += 500;
			}
		}
		m_linkedInteractable = nullptr;
	}

	// 移動させちゃう
	if (!m_attributes.isAttacking && !m_attributes.isAttacking2 && !m_attributes.isAttacking3 && !m_attributes.isHit1)
	{
		if (m_inputManager.GetKeyboardPress(DIK_A))
		{	// 左へ移動
			m_attributes.spd = VALUE_MOVE;

			m_attributes.isMoving = true;

			if (m_inputManager.GetKeyboardPress(DIK_W))
				m_attributes.targetDir = -XM_PI * 3 / 4 + camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_S))
				m_attributes.targetDir = -XM_PI / 4 + camera.GetRotation().y;
			else
				m_attributes.targetDir = -XM_PI / 2 + camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_D))
		{	// 右へ移動
			m_attributes.spd = VALUE_MOVE;

			m_attributes.isMoving = true;
			if (m_inputManager.GetKeyboardPress(DIK_W))
				m_attributes.targetDir = XM_PI * 3 / 4 + camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_S))
				m_attributes.targetDir = XM_PI / 4 + camera.GetRotation().y;
			else
				m_attributes.targetDir = XM_PI / 2 + camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_S))
		{	// 下へ移動
			m_attributes.spd = VALUE_MOVE;

			m_attributes.isMoving = true;
			if (m_inputManager.GetKeyboardPress(DIK_A))
				m_attributes.targetDir = -XM_PI * 3 / 4 + camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_D))
				m_attributes.targetDir = -XM_PI * 5 / 4 + camera.GetRotation().y;
			else
				m_attributes.targetDir = -XM_PI + camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_W))
		{	// 上へ移動
			m_attributes.spd = VALUE_MOVE;

			m_attributes.isMoving = true;
			if (m_inputManager.GetKeyboardPress(DIK_A))
				m_attributes.targetDir = -XM_PI / 4 + camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_D))
				m_attributes.targetDir = XM_PI / 4 + camera.GetRotation().y;
			else
				m_attributes.targetDir = 0.0f + camera.GetRotation().y;
		}

	}

	// 発射
	if (m_inputManager.IsMouseLeftTriggered() && !CursorManager::get_instance().IsMouseFreeMode())
	{

		if (m_attributes.hasWeapon || m_hasWeapon) {

			Transform trans;
			trans.pos = m_playerGO->GetTransform().pos;
			trans.pos.y += 70;
			trans.rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
			trans.scl = XMFLOAT3(1.0f, 1.0f, 1.0f);

			XMFLOAT3 bulletDir;
			bulletDir.x = sinf(m_transform.rot.y) * 5;
			bulletDir.z = cosf(m_transform.rot.y) * 5;
			bulletDir.y = 0;

			ProjectileManager::get_instance().SpawnProjectile(ProjectileType::Bullet, trans, bulletDir, 5);

		}

	}

	if (m_attributes.isHit1 || m_attributes.hitTimer > 0)
	{
		m_attributes.hitTimer -= m_playerGO->timer.GetScaledDeltaTime();
		if (m_attributes.hitTimer < 0)
		{
			m_attributes.isHit1 = false;
		}
	}

	if (m_attributes.isGrounded == false)
	{
		m_transform.pos.y -= FALLING_SPEED * m_playerGO->timer.GetScaledDeltaTime();
	}
}

void Player::UpdateClimbMode(void)
{
	if (m_inputManager.GetKeyboardPress(DIK_S))
	{	
		// 下へ移動
		m_attributes.spd = -VALUE_CLIMB;
		m_attributes.climbDown = true;
		m_attributes.climbUp = false;

	}
	else if (m_inputManager.GetKeyboardPress(DIK_W))
	{
		// 上へ移動
		m_attributes.spd = VALUE_CLIMB;
		m_attributes.climbUp = true;
		m_attributes.climbDown = false;
	}
	else
	{
		m_attributes.climbUp = false;
		m_attributes.climbDown = false;
	}
}

void Player::LinkInteractable(Interactable* interactable)
{
	m_linkedInteractable = interactable;
}

void Player::UpdateActionQueue(void)
{
	m_playerAttr.actionQueueClearTime++;
	for (UINT i = m_playerAttr.actionQueueStart; i < m_playerAttr.actionQueueEnd; i++)
	{
		m_playerAttr.actionQueue[i].liveTime += m_playerGO->timer.GetScaledDeltaTime();
		if (m_playerAttr.actionQueue[i].liveTime >= ACTION_QUEUE_CLEAR_WAIT)
		{
			m_playerAttr.actionQueueStart++;
			continue;
		}

		if (dynamic_cast<ISkinnedMeshModelChar*>(m_playerGO)->ExecuteAction(m_playerAttr.actionQueue[i].actionType))
		{
			m_playerAttr.actionQueueStart++;
		}
		else
		{
			break;
		}
	}
	if (m_playerAttr.actionQueueStart >= m_playerAttr.actionQueueEnd)
	{
		m_playerAttr.actionQueueStart = m_playerAttr.actionQueueEnd = 0;
	}
}
