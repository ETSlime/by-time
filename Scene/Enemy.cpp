//=============================================================================
//
// エネミーモデル処理 [Enemy.cpp]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Core/Graphics/Renderer.h"
#include "Model/Model.h"
#include "Utility/InputManager.h"
#include "Utility/Debugproc.h"
#include "Scene/Enemy.h"
#include "Core/Camera.h"
#include "Scene/Character/Hilichurl.h"
#include "Scene/Player.h"
#include "AI/BehaviorTree.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************

#define	VALUE_MOVE			(3.5f)							// 移動量
#define VALUE_RUN			(120.0f)
#define	VALUE_ROTATE		(XM_PI * 0.02f)					// 回転量

#define ENEMY_SHADOW_SIZE	(1.1f)						// 影の大きさ
#define ENEMY_OFFSET_Y		(7.0f)						// エネミーの足元をあわせる
#define ENEMY_SCALE			(1.8f)

#define	TEXTURE_MAX			(2)

#define ROTATION_SPEED		(0.09f)
#define FALLING_SPEED		(5.0f)
#define SPD_DECAY_RATE		(0.93f)

#define	HPGAUGE_PATH		"data/TEXTURE/EnemyHPGauge.png"
#define	HPGAUGE_COVER_PATH	"data/TEXTURE/EnemyHPGauge_bg.png"
#define HPGAUGE_WIDTH_SCL	(0.5f)
#define HPGAUGE_HEIGHT		(5.0f)

//=============================================================================
// 初期化処理
//=============================================================================
Enemy::Enemy(EnemyType enemyType, Transform trans)
{

	//MapEditor::get_instance().AddToList(this);
	m_enemyAttr.enemyType = enemyType;

	instance.load = true;
	m_behaviorTree = nullptr;
	m_player = nullptr;

	instance.attributes.spd = 0.0f;

	switch (m_enemyAttr.enemyType)
	{
	case EnemyType::Hilichurl:
		m_enemyAttr.maxHP = HILI_MAX_HP;
		m_enemyAttr.HP = HILI_MAX_HP;
		m_enemyAttr.viewAngle = HILI_VIEW_ANGLE;
		m_enemyAttr.viewDistance = HILI_VIEW_DISTANCE;
		m_enemyAttr.chaseRange = HILI_CHASING_RANGE;
		m_enemyAttr.attackRange = HILI_ATTACK_RANGE;

		m_behaviorTree = new BehaviorTree(this);
		break;
	default:
		break;
	}

	instance.transform.pos = trans.pos;
	instance.transform.rot = trans.rot;
	instance.transform.scl = trans.scl;
	m_enemyAttr.initTrans.pos = trans.pos;
	m_enemyAttr.initTrans.rot = trans.rot;
	m_enemyAttr.initTrans.scl = trans.scl;

	m_HPGaugeTex = TextureMgr::get_instance().CreateTexture(HPGAUGE_PATH);
	m_HPGaugeCoverTex = TextureMgr::get_instance().CreateTexture(HPGAUGE_COVER_PATH);

	instance.use = true;		// true:生きてる
}

Enemy::~Enemy()
{
	SafeRelease(&m_HPGaugeTex);
	SafeRelease(&m_HPGaugeCoverTex);
	SafeRelease(&m_HPGaugeVertexBuffer);
}

//=============================================================================
// 更新処理
//=============================================================================
void Enemy::Update(void)
{
	if (instance.use == true)		// このエネミーが使われている？
	{

		if (!UpdateAliveState())
			return;

		UpdateHPGauge();

		GameObject::Update();

		BOUNDING_BOX aabb = instance.pModel->GetBoundingBox();
		float height = aabb.maxPoint.y - aabb.minPoint.y;
		m_HPGauge.pos = instance.transform.pos;
		m_HPGauge.pos.y += height;

		m_enemyAttr.timer += timer.GetScaledDeltaTime();

		if (instance.attributes.isGrounded == false)
		{
			instance.transform.pos.y -= FALLING_SPEED * timer.GetScaledDeltaTime();
		}

		if (!CheckAvailableToMove())
			return;

		m_behaviorTree->RunBehaviorTree();

		float deltaDir = instance.attributes.targetDir - instance.attributes.dir;
		if (deltaDir > XM_PI) deltaDir -= XM_2PI;
		if (deltaDir < -XM_PI) deltaDir += XM_2PI;
		instance.attributes.dir += deltaDir * ROTATION_SPEED * timer.GetScaledDeltaTime();
		instance.transform.rot.y = instance.attributes.dir;

		if (instance.attributes.isMoveBlocked)
			return;

		if (m_enemyAttr.fixedDirMove)
		{
			// 角度の増分を計算（速度に基づく回転量）
			float deltaTheta = (instance.attributes.spd / m_enemyAttr.cooldownOrbitRadius)* m_enemyAttr.cooldownMoveDirection;

			XMVECTOR fixedDirVec = XMVectorReplicate(m_enemyAttr.fixedDir);
			XMVECTOR deltaThetaVec = XMVectorReplicate(deltaTheta);

			// 角度がdeltaThetaだけ変化した後のcosとsinを計算
			XMVECTOR cosNew = XMVectorCos(XMVectorAdd(fixedDirVec, deltaThetaVec));
			XMVECTOR cosOld = XMVectorCos(fixedDirVec);
			XMVECTOR sinNew = XMVectorSin(XMVectorAdd(fixedDirVec, deltaThetaVec));
			XMVECTOR sinOld = XMVectorSin(fixedDirVec);

			// X 軸方向の増分
			XMVECTOR dxVec = XMVectorScale(XMVectorSubtract(sinNew, sinOld), m_enemyAttr.cooldownOrbitRadius);
			// Z 軸方向の増分
			XMVECTOR dzVec = XMVectorScale(XMVectorSubtract(cosNew, cosOld), m_enemyAttr.cooldownOrbitRadius);

			float dx = XMVectorGetX(dxVec);
			float dz = XMVectorGetX(dzVec);

			instance.transform.pos.x += dx * timer.GetScaledDeltaTime();
			instance.transform.pos.z += dz * timer.GetScaledDeltaTime();

		}
		else
		{
			instance.transform.pos.x += sinf(instance.transform.rot.y) * instance.attributes.spd * timer.GetScaledDeltaTime();
			instance.transform.pos.z += cosf(instance.transform.rot.y) * instance.attributes.spd * timer.GetScaledDeltaTime();
		}


		instance.attributes.spd *= pow(SPD_DECAY_RATE, timer.GetScaledDeltaTime());

		{
			//UpdateEditorSelect(GetMousePosX(), GetMousePosY());
		}
	}
}

bool Enemy::UpdateAliveState(void)
{
	if (m_enemyAttr.isDead == true)
	{
		m_enemyAttr.respawnTimer -= timer.GetScaledDeltaTime();
		if (m_enemyAttr.respawnTimer <= 0.0f)
		{
			m_enemyAttr.respawnTimer = 0.0f;
			Initialize(); // 敵を初期化
		}
		else
			return false;
	}

	if (instance.renderProgress.progress < 1.0f && !m_enemyAttr.startFadeOut)
	{
		instance.renderProgress.isRandomFade = true;
		instance.renderProgress.progress += 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress > 1.0f)
		{
			instance.renderProgress.isRandomFade = false;
			instance.renderProgress.progress = 1.0f;
		}
	}
	else if (m_enemyAttr.startFadeOut == true)
	{
		instance.renderProgress.isRandomFade = true;
		instance.renderProgress.progress -= 0.01f * timer.GetScaledDeltaTime();
		if (instance.renderProgress.progress <= 0.0f)
		{
			instance.renderProgress.isRandomFade = false;
			instance.renderProgress.progress = 0.0f;
			m_enemyAttr.isDead = true;
			instance.collider.enable = false; // 当たり判定を無効化
			instance.castShadow = false; // 影を落とさない
		}
		return false;
	}

	if (m_enemyAttr.HP <= 0.0f)
	{
		m_enemyAttr.respawnTimer = ENEMY_RESPAWN_TIME;
		m_enemyAttr.die = true;
		return false;
	}

	return true;
}

//=============================================================================
// 描画処理
//=============================================================================
void Enemy::Draw(void)
{
	GameObject::Draw();
}

void Enemy::DrawUI(EnemyUIType type)
{
	switch (type)
	{
	case EnemyUIType::HPGauge:
		if (m_HPGauge.bUse)
			DrawHPGauge();
		break;
	case EnemyUIType::HPGaugeCover:
		if (m_HPGauge.bUse)
			DrawHPGaugeCover();
		break;
	default:
		break;
	}
}

void Enemy::DrawHPGauge(void)
{
	XMMATRIX mtxWorld;

	float ratio = m_enemyAttr.HP / m_enemyAttr.maxHP;

	// 血条のワールド座標を計算
	XMMATRIX worldPosition = XMMatrixTranslation(m_HPGauge.pos.x, m_HPGauge.pos.y, m_HPGauge.pos.z);
	// 左端をローカル座標の原点に移動
	XMMATRIX moveToLeft = XMMatrixTranslation(m_HPGauge.fWidth * 0.5f, 0.0f, 0.0f);
	// 原点を基準にスケーリング
	XMMATRIX scaleMatrix = XMMatrixScaling(ratio, 1.0f, 1.0f);
	// 左端を固定するための補正移動
	XMMATRIX moveBack = XMMatrixTranslation(-m_HPGauge.fWidth * 0.5f, 0.0f, 0.0f);

	// 左端を原点に移動
	mtxWorld = XMMatrixMultiply(moveToLeft, scaleMatrix);
	// 左端を固定するための補正移動
	mtxWorld = XMMatrixMultiply(mtxWorld, moveBack);
	// ビルボードの回転を適用
	mtxWorld = XMMatrixMultiply(mtxWorld, m_HPGauge.rot);
	// 最終的なワールド座標に移動
	mtxWorld = XMMatrixMultiply(mtxWorld, worldPosition);

	// ワールドマトリックスの設定
	Renderer::get_instance().SetCurrentWorldMatrix(&mtxWorld);

	m_HPGauge.material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	Renderer::get_instance().SetMaterial(m_HPGauge.material);

	// プリミティブトポロジ設定
	Renderer::get_instance().GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::get_instance().GetDeviceContext()->IASetVertexBuffers(0, 1, &m_HPGaugeVertexBuffer, &stride, &offset);

	Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &m_HPGaugeTex);

	// ポリゴンの描画
	Renderer::get_instance().GetDeviceContext()->Draw(4, 0);


}

void Enemy::DrawHPGaugeCover(void)
{
	XMMATRIX mtxTranslate, mtxWorld;

	mtxTranslate = XMMatrixTranslation(m_HPGauge.pos.x, m_HPGauge.pos.y, m_HPGauge.pos.z);
	mtxWorld = XMMatrixMultiply(m_HPGauge.rot, mtxTranslate);

	// ワールドマトリックスの設定
	Renderer::get_instance().SetCurrentWorldMatrix(&mtxWorld);

	// マテリアル設定
	m_HPGauge.material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	Renderer::get_instance().SetMaterial(m_HPGauge.material);

	// プリミティブトポロジ設定
	Renderer::get_instance().GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::get_instance().GetDeviceContext()->IASetVertexBuffers(0, 1, &m_HPGaugeVertexBuffer, &stride, &offset);

	// テクスチャ設定
	Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &m_HPGaugeCoverTex);

	// ポリゴンの描画
	Renderer::get_instance().GetDeviceContext()->Draw(4, 0);
}

void Enemy::InitHPGauge(void)
{
	BOUNDING_BOX aabb = instance.pModel->GetBoundingBox();
	float height = aabb.maxPoint.y - aabb.minPoint.y;
	float width = (aabb.maxPoint.x - aabb.minPoint.x) * HPGAUGE_WIDTH_SCL;

	m_HPGauge.pos = instance.transform.pos;
	m_HPGauge.pos.y += height;
	m_HPGauge.fWidth = width;
	m_HPGauge.fHeight = HPGAUGE_HEIGHT;
	MakeVertexHPGauge(m_HPGauge.fWidth, m_HPGauge.fHeight);

	m_HPGauge.bUse = true;
}

void Enemy::UpdateHPGauge(void)
{
	Camera& cam = Camera::get_instance();

	// ビューマトリックスを取得
	XMMATRIX mtxView = XMLoadFloat4x4(&cam.GetViewMatrix());

	// 正方行列（直交行列）を転置行列させて逆行列を作ってる版(速い)
	XMMATRIX billboardRotation = XMMatrixIdentity();
	billboardRotation.r[0] = XMVectorSet(mtxView.r[0].m128_f32[0], mtxView.r[1].m128_f32[0], mtxView.r[2].m128_f32[0], 0.0f);
	billboardRotation.r[1] = XMVectorSet(mtxView.r[0].m128_f32[1], mtxView.r[1].m128_f32[1], mtxView.r[2].m128_f32[1], 0.0f);
	billboardRotation.r[2] = XMVectorSet(mtxView.r[0].m128_f32[2], mtxView.r[1].m128_f32[2], mtxView.r[2].m128_f32[2], 0.0f);
	
	m_HPGauge.rot = billboardRotation;
}

void Enemy::Initialize(void)
{
	m_enemyAttr.Initialize();

	instance.transform.pos = m_enemyAttr.initTrans.pos;
	instance.transform.rot = m_enemyAttr.initTrans.rot;
	instance.transform.scl = m_enemyAttr.initTrans.scl;

	switch (m_enemyAttr.enemyType)
	{
	case EnemyType::Hilichurl:
		m_enemyAttr.maxHP = HILI_MAX_HP;
		m_enemyAttr.HP = HILI_MAX_HP;
		break;
	default:
		break;
	}

	instance.renderProgress.isRandomFade = true;
	instance.renderProgress.progress = 0.0f;
	instance.collider.enable = true;
	instance.castShadow = true; // 影を落とす

}

void Enemy::SetNewPosTarget()
{
	instance.attributes.targetDir = GetRandFloat(0.0f, XM_2PI);
	m_enemyAttr.moveDuration = GetRandFloat(100.0f, 800.0f);  // 移動時間
	m_enemyAttr.moveTimer = 0.0f;
	m_enemyAttr.timer = 0.0f;
}

void  Enemy::StartWaiting() 
{
	m_enemyAttr.isWaiting = true;
	m_enemyAttr.waitTime = GetRandFloat(300.0f, 900.0f);
	m_enemyAttr.timer = 0.0f;
}

void Enemy::CooldownWait(void)
{
	// 敵からプレイヤーへの方向ベクトル
	XMVECTOR enemyPosVec = XMLoadFloat3(&instance.transform.pos);
	XMVECTOR playerPosVec = XMLoadFloat3(&m_player->GetTransform().pos);
	XMVECTOR toPlayerVec = XMVectorSubtract(playerPosVec, enemyPosVec);
	toPlayerVec = XMVector3Normalize(toPlayerVec);

	instance.attributes.targetDir = atan2f(XMVectorGetX(toPlayerVec), XMVectorGetZ(toPlayerVec));
	m_enemyAttr.fixedDirMove = false;
	instance.attributes.spd = 0.0f;
	instance.attributes.isMoving = false;
}

bool Enemy::DetectPlayer(void)
{
	if (m_enemyAttr.isChasingPlayer == true) return true;

	XMVECTOR enemyPosVec = XMLoadFloat3(&instance.transform.pos);
	XMVECTOR playerPosVec = XMLoadFloat3(&m_player->GetTransform().pos);

	// プレイヤーへの方向ベクトル
	XMVECTOR diff = XMVectorSubtract(playerPosVec, enemyPosVec);
	float distSq = XMVectorGetX(XMVector3LengthSq(diff));

	// 視認範囲外なら発見しない
	if (distSq > m_enemyAttr.viewDistance * m_enemyAttr.viewDistance) return false;

	// 正面方向ベクトル（Z軸方向）
	XMVECTOR forward = XMVectorSet(sinf(instance.transform.rot.y), 0.0f, cosf(instance.transform.rot.y), 0.0f);
	diff = XMVector3Normalize(diff); // 正規化

	// 内積で角度を求める
	float dot = XMVectorGetX(XMVector3Dot(forward, diff));
	float angle = acosf(dot);

	return angle <= m_enemyAttr.viewAngle * 0.5f;
}

bool Enemy::CheckAvailableToMove(void)
{
	if (instance.attributes.isHit1 ||
		instance.attributes.isHit2)
	{
		instance.attributes.hitTimer--;
		if (instance.attributes.hitTimer < 0)
		{
			instance.attributes.isHit1 = false;
			instance.attributes.isHit2 = false;
		}
	}

	if (instance.attributes.isHit1 ||
		instance.attributes.isHit2 ||
		instance.attributes.isAttacking)
		return false;
	else
		return true;
}

void Enemy::ChasePlayer(void)
{
	if (m_player == nullptr) return;

	if (m_enemyAttr.isChasingPlayer == false) return;

	Transform playerTransform = m_player->GetTransform();

	// 敵からプレイヤーへの方向ベクトル
	XMVECTOR enemyPosVec = XMLoadFloat3(&instance.transform.pos);
	XMVECTOR playerPosVec = XMLoadFloat3(&playerTransform.pos);

	// プレイヤーまでの距離
	XMVECTOR diff = XMVectorSubtract(playerPosVec, enemyPosVec);
	float distSq = XMVectorGetX(XMVector3LengthSq(diff));
	m_enemyAttr.distPlayerSq = distSq;

	if (distSq < m_enemyAttr.attackRange * m_enemyAttr.attackRange * 1.5f)
	{
		// 攻撃範囲に入ったら攻撃モード
		AttackPlayer();
		return;
	}
	else if (distSq > m_enemyAttr.chaseRange * m_enemyAttr.chaseRange)
	{
		// 追跡範囲を超えたらランダム移動に戻る
		m_enemyAttr.isChasingPlayer = false;
		m_enemyAttr.fixedDirMove = false;
		instance.attributes.isMoving = false;
		SetNewPosTarget();
		return;
	}
	else
		m_enemyAttr.fixedDirMove = false;

	// 目標方向を計算
	float dx = instance.transform.pos.x - playerTransform.pos.x;
	float dz = instance.transform.pos.z - playerTransform.pos.z;

	instance.attributes.targetDir = -atan2(dz, dx) - XM_PI * 0.5f;

	// プレイヤー方向に移動
	instance.attributes.isMoving = true;
	instance.attributes.spd = VALUE_MOVE;
}

void Enemy::AttackPlayer(void)
{
	instance.attributes.isAttacking = true;
}

void Enemy::Patrol(void)
{
	// 通常の移動ロジック
	if (m_enemyAttr.isWaiting)
	{
		if (m_enemyAttr.timer >= m_enemyAttr.waitTime)
		{
			m_enemyAttr.isWaiting = false;
			SetNewPosTarget();
		}
	}
	else
	{
		// 移動処理
		m_enemyAttr.moveTimer += timer.GetScaledDeltaTime();

		// 目標時間に達した場合、待機モードに入る
		if (m_enemyAttr.moveTimer >= m_enemyAttr.moveDuration)
		{
			instance.attributes.isMoving = false;
			StartWaiting();
		}
		else
		{
			// 入力のあった方向へ向かせて移動させる
			instance.attributes.isMoving = true;
			instance.attributes.spd = VALUE_MOVE;
		}
	}
}

void Enemy::CooldownMove(void)
{
	// 敵からプレイヤーへの方向ベクトル
	XMVECTOR enemyPosVec = XMLoadFloat3(&instance.transform.pos);
	XMVECTOR playerPosVec = XMLoadFloat3(&m_player->GetTransform().pos);
	XMVECTOR toPlayerVec = XMVectorSubtract(playerPosVec, enemyPosVec);

	// プレイヤーまでの距離
	float distSq = XMVectorGetX(XMVector3LengthSq(toPlayerVec));
	m_enemyAttr.distPlayerSq = distSq;

	toPlayerVec = XMVector3Normalize(toPlayerVec);
	m_enemyAttr.fixedDir = atan2f(XMVectorGetX(toPlayerVec), XMVectorGetZ(toPlayerVec)) * m_enemyAttr.cooldownMoveDirection;
	m_enemyAttr.fixedDirMove = true;
	m_enemyAttr.cooldownOrbitRadius = sqrtf(m_enemyAttr.distPlayerSq);
	instance.attributes.targetDir = m_enemyAttr.fixedDir;
	instance.attributes.spd = VALUE_MOVE * 0.5f;
	instance.attributes.isMoving = true;
}

//=============================================================================
// 頂点情報の作成
//=============================================================================
HRESULT Enemy::MakeVertexHPGauge(float width, float height)
{
	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_3D) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Renderer::get_instance().GetDevice()->CreateBuffer(&bd, NULL, &m_HPGaugeVertexBuffer);

	// 頂点バッファに値をセットする
	D3D11_MAPPED_SUBRESOURCE msr;
	Renderer::get_instance().GetDeviceContext()->Map(m_HPGaugeVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

	m_HPGauge.fHeight = height;
	m_HPGauge.fWidth = width;
	ZeroMemory(&m_HPGauge.material, sizeof(m_HPGauge.material));

	// 頂点座標の設定
	vertex[0].Position = XMFLOAT3(-m_HPGauge.fWidth / 2.0f, m_HPGauge.fHeight, 0.0f);
	vertex[1].Position = XMFLOAT3(m_HPGauge.fWidth / 2.0f, m_HPGauge.fHeight, 0.0f);
	vertex[2].Position = XMFLOAT3(-m_HPGauge.fWidth / 2.0f, 0.0f, 0.0f);
	vertex[3].Position = XMFLOAT3(m_HPGauge.fWidth / 2.0f, 0.0f, 0.0f);

	// 法線の設定
	vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

	// 拡散光の設定
	vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// テクスチャ座標の設定
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

	renderer.GetDeviceContext()->Unmap(m_HPGaugeVertexBuffer, 0);

	return S_OK;
}