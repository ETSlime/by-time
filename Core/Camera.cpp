//=============================================================================
//
// カメラ処理 [Camera.cpp]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Utility/InputManager.h"
#include "Core/Camera.h"
#include "Utility/Debugproc.h"
#include "Core/GameSystem.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	POS_X_CAM			(0.0f)			// カメラの初期位置(X座標)
#define	POS_Y_CAM			(50.0f)			// カメラの初期位置(Y座標)
#define	POS_Z_CAM			(-480.0f)		// カメラの初期位置(Z座標)


#define	VALUE_MOVE_CAMERA	(2.0f)										// カメラの移動量
#define	VALUE_ROTATE_CAMERA	(XM_PI * 0.01f)								// カメラの回転量

//*****************************************************************************
// グローバル変数
//*****************************************************************************
static int				g_ViewPortType = TYPE_FULL_SCREEN;
static BOOL isDragging = FALSE;
static long startX = 0, startY = 0;
static long currentX = 0, currentY = 0;
static long deltaX = 0, deltaY = 0;

static Renderer& renderer = Renderer::get_instance();
//=============================================================================
// 初期化処理
//=============================================================================
void Camera::Init(void)
{
	m_pos = { POS_X_CAM, POS_Y_CAM, POS_Z_CAM };
	m_at  = { 0.0f, 0.0f, 0.0f };
	m_up  = { 0.0f, 1.0f, 0.0f };
	m_rot = { 0.0f, 0.0f, 0.0f };
	m_fov = VIEW_ANGLE;

	// 視点と注視点の距離を計算
	float vx, vz;
	vx = m_pos.x - m_at.x;
	vz = m_pos.z - m_at.z;
	m_len = sqrtf(vx * vx + vz * vz);
	
	m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
	m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
	m_pos.y = m_at.y + sinf(m_rot.x) * m_len;

	m_nearZ = VIEW_NEAR_Z;
	m_farZ = VIEW_FAR_Z_SCENE;

	// ビューポートタイプの初期化
	SetViewPort(g_ViewPortType);


	// プロジェクションマトリックス設定
	m_projScene = XMMatrixPerspectiveFovLH(VIEW_ANGLE, VIEW_ASPECT, m_nearZ, VIEW_FAR_Z_SCENE);
	m_projSkybox = XMMatrixPerspectiveFovLH(VIEW_ANGLE, VIEW_ASPECT, m_nearZ, VIEW_FAR_Z_SKYBOX);

	SetCameraType(CameraType::SCENE);
}


//=============================================================================
// カメラの終了処理
//=============================================================================
void Camera::Shutdown(void)
{

}


//=============================================================================
// カメラの更新処理
//=============================================================================
void Camera::Update(void)
{
	if (!GetWindowActive() 
		|| GameSystem::get_instance().IsPaused()
		|| GameSystem::get_instance().IsAltDown()) 
		return; // 非アクティブならカメラ更新スキップ

	// パラメータ設定
	const float MOUSE_SENSITIVITY = 0.003f;
	const float ZOOM_SENSITIVITY = 0.25f;
	const float MIN_CAMERA_DISTANCE = 320.0f;
	const float MAX_CAMERA_DISTANCE = 860.0f;
	const float CAMERA_APPROACH_SPEED = 3.0f;
	const float CAMERA_RETREAT_SPEED = 5.0f;
	const float GROUND_LIMIT_ANGLE = 0.3f;

	// 垂直回転角度の制限 (正値は上方向、負値は下方向)
	const float MAX_VERTICAL_ANGLE = XM_PIDIV2 - 0.1f;
	const float MIN_VERTICAL_ANGLE = -0.1f;

	// マウスの前回位置を保持する静的変数
	static long prevMouseX = 0;
	static long prevMouseY = 0;
	// 元のカメラ距離を保存
	static float originalDistance = m_len;
	static bool rotated = false;


	if (m_inputManager.IsMouseRecentered())
	{
		prevMouseX = GetMousePosX();
		prevMouseY = GetMousePosY();
		m_inputManager.SetMouseRecentered(false);  // 次のフレームから有効
	}
	else
	{
		long currentMouseX = GetMousePosX();
		long currentMouseY = GetMousePosY();

		// マウスの移動量（フレーム間の差分）を計算
		float deltaX = static_cast<float>(currentMouseX - prevMouseX);
		float deltaY = static_cast<float>(currentMouseY - prevMouseY);

		// 前回のマウス位置を更新
		prevMouseX = currentMouseX;
		prevMouseY = currentMouseY;

		// 水平方向の回転処理 (マウス左右移動)
		m_rot.y += deltaX * MOUSE_SENSITIVITY * m_timer.GetScaledDeltaTime();

		// 水平方向の回転角度を -π 〜 π に正規化
		if (m_rot.y > XM_PI)
			m_rot.y -= XM_2PI;
		if (m_rot.y < -XM_PI)
			m_rot.y += XM_2PI;

		// 垂直方向の回転処理 (マウス上下移動)
		float desiredRotX = m_rot.x + deltaY * MOUSE_SENSITIVITY * m_timer.GetScaledDeltaTime();
		if (desiredRotX > MIN_VERTICAL_ANGLE && desiredRotX <= MAX_VERTICAL_ANGLE)
		{
			if (m_len < originalDistance && deltaY > 0)
			{
				m_len += CAMERA_RETREAT_SPEED * m_timer.GetScaledDeltaTime();
				if (m_len > originalDistance) m_len = originalDistance;
			}
			else
				m_rot.x = desiredRotX;
		}
		else if (desiredRotX <= MIN_VERTICAL_ANGLE)
		{
			// 地面に近づきすぎた場合の処理
			if (deltaY < 0)
			{
				m_len -= CAMERA_APPROACH_SPEED * m_timer.GetScaledDeltaTime();
				//if (len > MIN_CAMERA_DISTANCE)
				//	rot.x += deltaY* MOUSE_SENSITIVITY * 0.3f;
				if (m_len < MIN_CAMERA_DISTANCE)
					m_len = MIN_CAMERA_DISTANCE;
			}
			else if (deltaY > 0 && m_len < originalDistance)
			{

				m_len += CAMERA_RETREAT_SPEED * m_timer.GetScaledDeltaTime();
				//if (len < originalDistance)
				//	rot.x -= deltaY * MOUSE_SENSITIVITY * 0.6f;
				if (m_len > originalDistance)
					m_len = originalDistance;
			}
		}
		else if (desiredRotX > MAX_VERTICAL_ANGLE)
		{
			// 最大仰角を超えないように制限
			m_rot.x = MAX_VERTICAL_ANGLE;
		}

		// マウスホイールによるズーム処理
		long wheelDelta = m_inputManager.GetMouseZ();
		if (wheelDelta != 0)
		{
			m_len -= wheelDelta * ZOOM_SENSITIVITY * m_timer.GetScaledDeltaTime();

			if (m_len < MIN_CAMERA_DISTANCE) m_len = MIN_CAMERA_DISTANCE;
			if (m_len > MAX_CAMERA_DISTANCE) m_len = MAX_CAMERA_DISTANCE;

			originalDistance = m_len;
		}


		// マウス位置を中央に戻す
		//if (rotated)
		m_inputManager.SetMousePosCenter();
	}



	// カメラ位置の更新
	m_pos.x = m_at.x - sinf(m_rot.y) * cosf(m_rot.x) * m_len;
	m_pos.z = m_at.z - cosf(m_rot.y) * cosf(m_rot.x) * m_len;
	m_pos.y = m_at.y + sinf(m_rot.x) * m_len;

	//if (!IsMouseRecentered())
	//	SetMousePosCenter();

#ifdef _DEBUG
	if (m_inputManager.GetKeyboardPress(DIK_Z))
	{// 視点旋回「左」
		m_rot.y += VALUE_ROTATE_CAMERA;
		if (m_rot.y > XM_PI)
		{
			m_rot.y -= XM_2PI;
		}

		m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
		m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
	}

	if (m_inputManager.GetKeyboardPress(DIK_C))
	{// 視点旋回「右」
		m_rot.y -= VALUE_ROTATE_CAMERA;
		if (m_rot.y < -XM_PI)
		{
			m_rot.y += XM_2PI;
		}

		m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
		m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
	}

	if (m_inputManager.GetKeyboardPress(DIK_Y))
	{// 視点移動「上」
		m_pos.y += VALUE_MOVE_CAMERA;
	}

	if (m_inputManager.GetKeyboardPress(DIK_N))
	{// 視点移動「下」
		m_pos.y -= VALUE_MOVE_CAMERA;
	}

	if (m_inputManager.GetKeyboardPress(DIK_Q))
	{// 注視点旋回「左」
		m_rot.y -= VALUE_ROTATE_CAMERA;
		if (m_rot.y < -XM_PI)
		{
			m_rot.y += XM_2PI;
		}

		m_at.x = m_pos.x + sinf(m_rot.y) * m_len;
		m_at.z = m_pos.z + cosf(m_rot.y) * m_len;
	}

	if (m_inputManager.GetKeyboardPress(DIK_E))
	{// 注視点旋回「右」
		m_rot.y += VALUE_ROTATE_CAMERA;
		if (m_rot.y > XM_PI)
		{
			m_rot.y -= XM_2PI;
		}

		m_at.x = m_pos.x + sinf(m_rot.y) * m_len;
		m_at.z = m_pos.z + cosf(m_rot.y) * m_len;
	}

	if (m_inputManager.GetKeyboardPress(DIK_LCONTROL))
	{
		if (m_inputManager.IsMouseLeftTriggered())
		{
			isDragging = TRUE;
			startX = GetMousePosX();
			startY = GetMousePosY();
			currentX = startX;
			currentY = startY;
			deltaX = 0;
			deltaY = 0;
		}
		else if (m_inputManager.IsMouseLeftPressed() && isDragging == TRUE)
		{
			long newX = GetMousePosX();
			long newY = GetMousePosY();
			deltaX = newX - currentX;
			deltaY = newY - currentY;
			currentX = newX;
			currentY = newY;

		}
		else if (!m_inputManager.IsMouseLeftPressed())
		{
			isDragging = FALSE;
			deltaX = 0L;
			deltaY = 0L;
		}

		m_rot.x -= deltaY * VALUE_ROTATE_CAMERA * 0.1f * m_timer.GetScaledDeltaTime();
		m_rot.y -= deltaX * VALUE_ROTATE_CAMERA * 0.1f * m_timer.GetScaledDeltaTime();
		if (m_rot.y < -XM_PI)
		{
			m_rot.y += XM_2PI;
		}
		else if (m_rot.y > XM_PI)
		{
			m_rot.y -= XM_2PI;
		}

		m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
		m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
		m_pos.y = m_at.y + sinf(m_rot.x) * m_len;
	}
	

	if (m_inputManager.GetKeyboardPress(DIK_T))
	{// 注視点移動「上」
		m_at.y += VALUE_MOVE_CAMERA;
	}

	if (m_inputManager.GetKeyboardPress(DIK_B))
	{// 注視点移動「下」
		m_at.y -= VALUE_MOVE_CAMERA;
	}

	if (m_inputManager.GetKeyboardPress(DIK_U))
	{// 近づく
		m_len -= VALUE_MOVE_CAMERA;
		m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
		m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
	}

	if (m_inputManager.GetKeyboardPress(DIK_M))
	{// 離れる
		m_len += VALUE_MOVE_CAMERA;
		m_pos.x = m_at.x - sinf(m_rot.y) * m_len;
		m_pos.z = m_at.z - cosf(m_rot.y) * m_len;
	}

	// カメラを初期に戻す
	if (m_inputManager.GetKeyboardPress(DIK_R))
	{
		Shutdown();
		Init();
	}

#endif



#ifdef _DEBUG	// デバッグ情報を表示する
	m_debugProc.PrintDebugProc("Camera:ZC QE TB YN UM R\n");
#endif
}


//=============================================================================
// カメラの更新
//=============================================================================
void Camera::SetCamera(void)
{
	// ビューマトリックス設定
	XMMATRIX mtxView;
	mtxView = XMMatrixLookAtLH(XMLoadFloat3(&m_pos), XMLoadFloat3(&m_at), XMLoadFloat3(&m_up));
	renderer.SetViewMatrix(&mtxView);
	XMStoreFloat4x4(&this->m_mtxView, mtxView);

	XMMATRIX mtxInvView;
	mtxInvView = XMMatrixInverse(nullptr, mtxView);
	XMStoreFloat4x4(&this->m_mtxInvView, mtxInvView);

	renderer.SetShaderCamera(m_pos);
}

//=============================================================================
// ビューポートの設定
//=============================================================================
void Camera::SetViewPort(int type)
{
	ID3D11DeviceContext *g_ImmediateContext = renderer.GetDeviceContext();
	D3D11_VIEWPORT vp;

	g_ViewPortType = type;

	// ビューポート設定
	switch (g_ViewPortType)
	{
	case TYPE_FULL_SCREEN:
		vp.Width = (FLOAT)SCREEN_WIDTH;
		vp.Height = (FLOAT)SCREEN_HEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		break;

	case TYPE_LEFT_HALF_SCREEN:
		vp.Width = (FLOAT)SCREEN_WIDTH / 2;
		vp.Height = (FLOAT)SCREEN_HEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		break;

	case TYPE_RIGHT_HALF_SCREEN:
		vp.Width = (FLOAT)SCREEN_WIDTH / 2;
		vp.Height = (FLOAT)SCREEN_HEIGHT;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = (FLOAT)SCREEN_WIDTH / 2;
		vp.TopLeftY = 0;
		break;

	case TYPE_UP_HALF_SCREEN:
		vp.Width = (FLOAT)SCREEN_WIDTH;
		vp.Height = (FLOAT)SCREEN_HEIGHT / 2;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		break;

	case TYPE_DOWN_HALF_SCREEN:
		vp.Width = (FLOAT)SCREEN_WIDTH;
		vp.Height = (FLOAT)SCREEN_HEIGHT / 2;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = (FLOAT)SCREEN_HEIGHT / 2;
		break;


	}
	g_ImmediateContext->RSSetViewports(1, &vp);

}


int Camera::GetViewPortType(void)
{
	return g_ViewPortType;
}



// カメラの視点と注視点をセット
void Camera::SetCameraAT(XMFLOAT3 pos)
{
	// カメラの注視点を引数の座標にしてみる
	m_at = pos;

	// カメラの視点をカメラのY軸回転に対応させている
	pos.x = m_at.x - sinf(m_rot.y) * m_len;
	pos.z = m_at.z - cosf(m_rot.y) * m_len;

}

void Camera::SetCameraType(CameraType type)
{
	m_CameraType = type;

	switch (type)
	{
	case CameraType::SKYBOX:
		m_farZ = VIEW_FAR_Z_SKYBOX;
		XMStoreFloat4x4(&m_mtxProjection, m_projSkybox);
		break;
	case CameraType::SCENE:
		m_farZ = VIEW_FAR_Z_SCENE;
		XMStoreFloat4x4(&m_mtxProjection, m_projScene);
		break;
	default:
		break;
	}

	XMMATRIX mtxProjection = XMLoadFloat4x4(&this->m_mtxProjection);
	renderer.SetProjectionMatrix(&mtxProjection);
}

