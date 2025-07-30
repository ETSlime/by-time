//=============================================================================
//
// player処理 [Player.cpp]
// Author : 
//
//=============================================================================
#include "Utility/InputManager.h"
#include "Collision/OctreeNode.h"
#include "Core/Camera.h"
#include "Core/CursorManager.h"
#include "Core/Graphics/Renderer.h"
#include "Scene/Player.h"
#include "Scene/Weapon/ProjectileManager.h"
#include "Scene/PlayerEnterExitBehavior.h"
#include "Core/GameSystem.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define ROTATION_SPEED				(0.18f)
#define FALLING_SPEED				(12.50f)
#define SPD_DECAY_RATE				(0.93f)

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

	//hilichurl = new Hilichurl();

	//mitachurl = new Mitachurl();

	m_playerAttr.allowControl = true;
	m_playerAttr.isMeleeMode = true; // 初期は近接モード
	m_playerAttr.seedCount = 0; // 初期は種を持っていない

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

	m_previewRenderer = new ProjectilePreviewRenderer();
	m_predictor.SetCollisionVoxelGrid(CollisionManager::get_instance().GetVoxelGrid()); // 地形ボクセルをセット
}

Player::~Player()
{
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

	if (m_playerAttr.climbMode)
		UpdateClimbMode();
	else
		UpdateNormalMode();
	
	HandlePlayerMove(m_transform);
	m_playerGO->SetTransform(m_transform);

	m_attributes->spd *= SPD_DECAY_RATE;
	m_playerGO->Update();

	//For debugging : Automatic HP reduction(1 per second)
		if (m_autoHPDrain)
		{
			float deltaTime = 1.0f / 60.0f;
				m_hpDrainTimer += deltaTime;

			if (m_hpDrainTimer >= 1.0f)
			{
				TakeDamage(1);
				m_hpDrainTimer = 0.0f;
			}
		}
	//For Debugging : HP Recovery
		if (m_inputManager.GetKeyboardPress(DIK_L))
		{
			m_currentHP = 100.0f;
		}


#ifdef _DEBUG
	m_debugProc.PrintDebugProc("PosX: %f PosY: %f, PosZ: %f\n", m_transform.pos.x, m_transform.pos.y, m_transform.pos.z);
	m_debugProc.PrintDebugProc("HP: %f\n", m_currentHP);
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
	float deltaDir = m_attributes->targetDir - m_attributes->dir;
	if (deltaDir > XM_PI) deltaDir -= XM_2PI;
	if (deltaDir < -XM_PI) deltaDir += XM_2PI;
	m_attributes->dir += deltaDir * ROTATION_SPEED * m_playerGO->timer.GetScaledDeltaTime();

	transform.rot.y = m_attributes->dir;

	if (m_attributes->isMoveBlocked)
		return;

	if (m_playerAttr.aimMode || m_playerAttr.throwMode)
	{
		// カメラ基準のローカル移動方向を計算
		XMFLOAT3 moveDir = { 0, 0, 0 };

		if (m_inputManager.GetKeyboardPress(DIK_W)) moveDir.z += 1.0f; // 前
		if (m_inputManager.GetKeyboardPress(DIK_S)) moveDir.z -= 1.0f; // 後
		if (m_inputManager.GetKeyboardPress(DIK_D)) moveDir.x += 1.0f; // 右
		if (m_inputManager.GetKeyboardPress(DIK_A)) moveDir.x -= 1.0f; // 左

		// 正規化（斜め移動で速くならないように）
		XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&moveDir));

		// カメラのright/forwardを使ってワールド方向に変換
		XMVECTOR forward = XMLoadFloat3(&m_playerGO->GetTransform().GetForward());
		forward = XMVectorSetY(forward, 0.0f); // 水平に限定
		forward = XMVector3Normalize(forward);

		XMVECTOR right = XMVector3Normalize(XMVector3Cross(XMVectorSet(0, 1, 0, 0), forward));

		XMVECTOR worldMove = right * moveDir.x + forward * moveDir.z;
		worldMove = XMVector3Normalize(worldMove);

		// 速度適用
		XMFLOAT3 finalMove;
		XMStoreFloat3(&finalMove, worldMove * m_attributes->spd);

		transform.pos.x += finalMove.x;
		transform.pos.z += finalMove.z;

	}
	else
	{
		// 入力のあった方向へプレイヤーを向かせて移動させる
		float speed = m_attributes->spd;
		if (m_attributes->isRunning)
			speed *= VALUE_PLAYEER_RUN_FACTOR;

		if (m_playerAttr.climbMode)
		{
			transform.pos.y += speed;
		}
		else
		{
			transform.pos.x += sinf(transform.rot.y) * speed;
			transform.pos.z += cosf(transform.rot.y) * speed;
		}
	}



#ifdef _DEBUG
	if (m_inputManager.GetKeyboardPress(DIK_TAB))
	{
		transform.pos.x += sinf(transform.rot.y) * m_attributes->spd * 5 * m_playerGO->timer.GetScaledDeltaTime();
		transform.pos.z += cosf(transform.rot.y) * m_attributes->spd * 5 * m_playerGO->timer.GetScaledDeltaTime();
	}
#endif // DEBUG

}

void Player::AlignPlayerYawToCamera(void)
{
	// カメラの向き
	float targetYaw = m_camera.GetRotation().y;
	// 補間速度（値を上げると追従が速い）
	float lerpSpeed = 10.0f;
	float t = min(1.0f, m_playerGO->timer.GetDeltaTime() * lerpSpeed);

	// 線形補間で滑らかに回転を追従
	m_attributes->dir = SmoothAngleLerp(m_attributes->dir, targetYaw, t);

	// targetDir は常に目標値として更新
	m_attributes->targetDir = targetYaw;
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

	m_maxHP = 100.0f;
	m_currentHP = 100.0f;
	SetHP(1.0f);

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

void Player::SetClimbMode(PlayerState climbState)
{
	if (climbState == PlayerState::START_CLIMBING_UP ||
		climbState == PlayerState::START_CLIMBING_DOWN)
	{
		// クライムモードを有効にする
		m_playerAttr.climbMode = true;
		m_playerAttr.enableClimbUp = false;
		m_playerAttr.enableClimbDown = false;

		// クライムモードではコリジョンを無効化
		m_playerGO->SetStaticCollisionEnable(false);
		m_playerGO->SetDynamicCollisionEnable(false);

		DeferredTaskOptions options{};
		options.async = false; // 同期で実行
		options.debugName = "ClimbFade"; // デバッグ用の名前
		options.delay = 2.5f; // 遅延時間を設定
		options.pauseDelay = true; // 一時停止中でも遅延を実行する

		RawDeferredTask::Add(
			nullptr,
			[](void* ptr)
			{
				UIManager::get_instance().GetFader().StartFadeOut();
			},
			nullptr,
			options
		);
	}
	else if (climbState == PlayerState::STOP_CLIMBING_UP ||
		climbState == PlayerState::STOP_CLIMBING_DOWN)
	{
		// クライムモードを無効にする
		m_playerAttr.climbMode = false;
		m_attributes->isMoveBlocked = true;

		DeferredTaskOptions options{};
		options.async = false; // 同期で実行
		options.debugName = "ClimbFade"; // デバッグ用の名前
		options.delay = 2.5f; // 遅延時間を設定
		options.pauseDelay = true; // 一時停止中でも遅延を実行する

		RawDeferredTask::Add(
			nullptr,
			[](void* ptr)
			{
				UIManager::get_instance().GetFader().StartFadeOut();
			},
			nullptr,
			options
		);
	}
	else
		return;

	// クライム状態を設定
	dynamic_cast<Lumine*>(m_playerGO)->SetClimb(climbState);
}

void Player::PlaceItem(ItemType type)
{
	Item* item = m_itemManager.GetItem(type);
	if (item == nullptr || item->GetUsed())
		return;

	item->Use();
	lumine->PlaceItem(type);
}

void Player::OnClimbPromptTriggered(void* userData)
{
	ClimbContext* ctx = static_cast<ClimbContext*>(userData);
	if (ctx == nullptr || ctx->player == nullptr)
		return;

	PlayerState climbState = ctx->climbState;
	Player* player = ctx->player;

	player->SetClimbMode(climbState);

	// コールバック関数が設定されている場合は実行
	if (ctx->postCallback)
	{
		ctx->postCallback(ctx);
	}
}

void Player::OnPlaceItemPromptTriggered(void* userData)
{
	PlaceItemContext* ctx = static_cast<PlaceItemContext*>(userData);
	if (ctx == nullptr || ctx->player == nullptr)
		return;

	ItemType itemType = ctx->itemType;
	Player* player = ctx->player;
	player->PlaceItem(itemType);
}

void Player::UpdateNormalMode(void)
{
	// アクションキューの更新
	UpdateActionQueue();

	if (m_playerAttr.allowControl)
	{
		// キャラクターの切り替え
		UpdateSwitchCharacter();

		// 移動させちゃう
		UpdatePlayerMove();
		
		// 攻撃の更新
		UpdatePlayerAttack();

		// 入力の処理
		HandleInput();
	}

	// プレイヤーの状態を更新
	UpdatePlayerStatus();

#ifdef _DEBUG
	if (m_inputManager.GetKeyboardPress(DIK_UP))
	{
		m_transform.pos.y += 50;
	}
	if (m_inputManager.GetKeyboardPress(DIK_DOWN))
	{
		m_transform.pos.y -= 50;
	}
#endif
}

void Player::UpdateClimbMode(void)
{
	if (m_playerAttr.enableClimbDown && m_inputManager.GetKeyboardPress(DIK_S))
	{	
		// 下へ移動
		m_attributes->spd = -VALUE_PLAYER_CLIMB;
		m_playerAttr.climbDown = true;
		m_playerAttr.climbUp = false;

	}
	else if (m_playerAttr.enableClimbUp && m_inputManager.GetKeyboardPress(DIK_W))
	{
		// 上へ移動
		m_attributes->spd = VALUE_PLAYER_CLIMB;
		m_playerAttr.climbUp = true;
		m_playerAttr.climbDown = false;
	}
	else
	{
		m_playerAttr.climbUp = false;
		m_playerAttr.climbDown = false;
	}
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

void Player::UpdatePlayerStatus(void)
{
	// 地面にいるかどうか
	if (!m_attributes->isGrounded)
	{
		// 滑空時間を更新
		m_attributes->glideDuration += m_playerGO->timer.GetDeltaTime();
		// 落下処理
		m_transform.pos.y -= FALLING_SPEED;
	}

	// ヒット中かどうか
	if (m_attributes->isHit1 || m_attributes->hitTimer > 0)
	{
		m_attributes->hitTimer -= m_playerGO->timer.GetScaledDeltaTime();
		if (m_attributes->hitTimer < 0)
		{
			m_attributes->isHit1 = false;
		}
	}

	if (m_attributes->lastFireTimeCountDown > 0)
	{
		m_attributes->lastFireTimeCountDown -= m_playerGO->timer.GetDeltaTime();
	}

	if (m_playerAttr.throwMode)
		AlignPlayerYawToCamera();
}

void Player::UpdatePlayerMove(void)
{
	if (!m_attributes->isAttacking && !m_attributes->isAttacking2 && !m_attributes->isAttacking3 && !m_attributes->isHit1)
	{
		if (m_inputManager.GetKeyboardPress(DIK_A))
		{	// 左へ移動
			m_attributes->spd = VALUE_PLAYER_MOVE;
			m_attributes->isMoving = true;

			if (m_inputManager.GetKeyboardPress(DIK_W))
				m_attributes->targetDir = -XM_PI * 3 / 4 + m_camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_S))
				m_attributes->targetDir = -XM_PI / 4 + m_camera.GetRotation().y;
			else
				m_attributes->targetDir = -XM_PI / 2 + m_camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_D))
		{	// 右へ移動
			m_attributes->spd = VALUE_PLAYER_MOVE;
			m_attributes->isMoving = true;

			if (m_inputManager.GetKeyboardPress(DIK_W))
				m_attributes->targetDir = XM_PI * 3 / 4 + m_camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_S))
				m_attributes->targetDir = XM_PI / 4 + m_camera.GetRotation().y;
			else
				m_attributes->targetDir = XM_PI / 2 + m_camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_S))
		{	// 下へ移動
			m_attributes->spd = VALUE_PLAYER_MOVE;
			m_attributes->isMoving = true;

			if (m_inputManager.GetKeyboardPress(DIK_A))
				m_attributes->targetDir = -XM_PI * 3 / 4 + m_camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_D))
				m_attributes->targetDir = -XM_PI * 5 / 4 + m_camera.GetRotation().y;
			else
				m_attributes->targetDir = -XM_PI + m_camera.GetRotation().y;
		}
		if (m_inputManager.GetKeyboardPress(DIK_W))
		{	// 上へ移動
			m_attributes->spd = VALUE_PLAYER_MOVE;
			m_attributes->isMoving = true;

			if (m_inputManager.GetKeyboardPress(DIK_A))
				m_attributes->targetDir = -XM_PI / 4 + m_camera.GetRotation().y;
			else if (m_inputManager.GetKeyboardPress(DIK_D))
				m_attributes->targetDir = XM_PI / 4 + m_camera.GetRotation().y;
			else
				m_attributes->targetDir = 0.0f + m_camera.GetRotation().y;
		}
	}

	if (m_inputManager.GetKeyboardTrigger(DIK_SPACE))
	{
		m_attributes->isJumping = true;
	}
}

void Player::UpdatePlayerAttack(void)
{
	// 発射
	if (m_inputManager.IsMouseLeftTriggered() && !CursorManager::get_instance().IsMouseFreeMode())
	{

		if (m_attributes->hasWeapon) {

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

		if (m_playerAttr.throwMode)
			ConfirmThrow();

	}

	if (m_inputManager.IsMouseRightTriggered())
	{
		if (m_playerAttr.throwMode)
			CancelThrow();
	}


	// 攻撃
	//if (m_inputManager.IsMouseLeftTriggered() && !CursorManager::get_instance().IsMouseFreeMode())
	//{
	//	if (m_attributes->hasWeapon)
	//	{

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
}

void Player::UpdateSwitchCharacter(void)
{
	if (lumine && m_inputManager.GetKeyboardTrigger(DIK_1))
	{
		lumine->SetTransform(m_transform);
		//lumine->SetRenderProgress(0.0f);
		//lumine->SetSwitchCharEffect(true);
		m_playerGO = lumine;
		m_light->BindToTransform(m_playerGO->GetTransformP());
	}
	//if (klee && m_inputManager.GetKeyboardTrigger(DIK_2))
	//{
	//	klee->SetTransform(m_transform);
	//	//klee->SetRenderProgress(0.0f);
	//	//klee->SetSwitchCharEffect(true);
	//	m_playerGO = klee;
	//	m_light->BindToTransform(m_playerGO->GetTransformP());
	//}
	//if (sigewinne && m_inputManager.GetKeyboardTrigger(DIK_3))
	//{
	//	sigewinne->SetTransform(m_transform);
	//	//sigewinne->SetRenderProgress(0.0f);
	//	//sigewinne->SetSwitchCharEffect(true);
	//	m_playerGO = sigewinne;
	//	m_light->BindToTransform(m_playerGO->GetTransformP());
	//}
}

void Player::HandleInput(void)
{
	if (!m_inputManager.GetKeyboardPress(DIK_W)
		&& !m_inputManager.GetKeyboardPress(DIK_S)
		&& !m_inputManager.GetKeyboardPress(DIK_A)
		&& !m_inputManager.GetKeyboardPress(DIK_D))
		m_attributes->isMoving = false;

	if (m_inputManager.IsMouseRightPressed())
	{
		if (!m_playerAttr.throwMode)
		{
			m_playerAttr.aimMode = true;
			m_camera.SetAimMode(CameraMode::Aiming, true);

			AlignPlayerYawToCamera();
		}
	}
	else
	{
		m_playerAttr.aimMode = false;
		m_camera.SetAimMode(CameraMode::Aiming, false);
	}

	if (m_inputManager.GetKeyboardRelease(KEY_ENTER_THROW_MODE) && !m_playerAttr.throwMode)
	{
		if (!m_playerAttr.aimMode)
		{
			m_playerAttr.throwMode = true;
			m_camera.SetAimMode(CameraMode::ProjectileAiming, true);
		}
	}
	else if (m_inputManager.GetKeyboardRelease(KEY_ENTER_THROW_MODE) && m_playerAttr.throwMode)
	{
		ConfirmThrow();
	}
	else if (m_inputManager.GetKeyboardRelease(KEY_EXIT_THROW_MODE) && m_playerAttr.throwMode)
	{
		CancelThrow();
	}

	if (m_inputManager.GetKeyboardPress(KEY_RUN))
	{
		m_attributes->isRunning = true;
	}
	else
	{
		m_attributes->isRunning = false;
	}
}

void Player::ConfirmThrow(void)
{
	m_playerAttr.throwMode = false;
	m_playerAttr.confirmThrow = true;
	m_playerAttr.allowControl = false;
	m_camera.SetAimMode(CameraMode::ProjectileAiming, false);
}

void Player::CancelThrow(void)
{
	m_playerAttr.throwMode = false;
	m_playerAttr.confirmThrow = false;
	m_camera.SetAimMode(CameraMode::ProjectileAiming, false);
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


void Player::InitHPBar(UIManager* uiManager)
{
	if (m_hpBar)
	{
		delete m_hpBar;
		m_hpBar = nullptr;
	}

	m_hpBar = new HPBarUI();

	// Please adjust the position and size of the HP bar accordingly.
	m_hpBar->SetPosition({ 180.0f, 120.0f });
	m_hpBar->SetSize({ 440.0f, 50.0f });
	m_hpBar->SetHP(1.0f);


	ID3D11ShaderResourceView* decoTex = TextureMgr::get_instance().CreateTexture("data/TEXTURE/ui2hp_.png");
	m_hpBar->SetDecorationTexture(decoTex);

	if (uiManager)
	{
		uiManager->AddElement(m_hpBar);
	}
}

void Player::SetHP(float hp)
{
	if (m_hpBar)
	{
		m_hpBar->SetHP(hp);
	}
}

void Player::TakeDamage(int damage)
{
	m_currentHP -= damage;
	if (m_currentHP < 0) m_currentHP = 0;

	float hpRatio = static_cast<float>(m_currentHP) / static_cast<float>(m_maxHP);
	SetHP(hpRatio);  // HPバー更新
}

void Player::Heal(int amount)
{
	m_currentHP += amount;
	if (m_currentHP > m_maxHP) m_currentHP = m_maxHP;

	float hpRatio = static_cast<float>(m_currentHP) / static_cast<float>(m_maxHP);
	SetHP(hpRatio);  // HPバー更新
}