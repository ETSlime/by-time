//=============================================================================
//
// ビュー／プロジェクション行列および視点制御ユーティリティ [Camera.h]
// Author : 
// シーン／スカイボックスの視点切替、Zレンジ・FOV・Viewport管理、
// 視点移動および行列更新を統一的に制御するカメラ管理クラス
//
//=============================================================================
#pragma once

//*****************************************************************************
// インクルードファイル
//*****************************************************************************
#include "Core/Graphics/Renderer.h"
#include "Utility/SingletonBase.h"
#include "Core/Timer.h"
#include "Utility/InputManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	VIEW_ANGLE				(XMConvertToRadians(45.0f))						// ビュー平面の視野角
#define	VIEW_ASPECT				((float)SCREEN_WIDTH / (float)SCREEN_HEIGHT)	// ビュー平面のアスペクト比	
#define	VIEW_NEAR_Z				(10.0f)											// ビュー平面のNearZ値
#define	VIEW_FAR_Z_SKYBOX		(800000.0f)										// ビュー平面のFarZ値
#define	VIEW_FAR_Z_SCENE		(50000.0f)										// ビュー平面のFarZ値

enum 
{
	TYPE_FULL_SCREEN,
	TYPE_LEFT_HALF_SCREEN,
	TYPE_RIGHT_HALF_SCREEN,
	TYPE_UP_HALF_SCREEN,
	TYPE_DOWN_HALF_SCREEN,
	TYPE_NONE,
};

enum class CameraType
{
	SKYBOX,
	SCENE,
};


//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************

class Camera : public SingletonBase<Camera>
{
public:
	void Init(void);
	void Shutdown(void);
	void Update(void);
	void SetCamera(void);

	void SetViewPort(int type);
	int GetViewPortType(void);

	void SetCameraAT(XMFLOAT3 pos);

	XMFLOAT4X4 GetViewMatrix(void) { return m_mtxView; }
	XMFLOAT4X4 GetProjMatrix(void) { return m_mtxProjection; }
	XMFLOAT3 GetRotation(void) { return m_rot; }
	XMMATRIX GetViewProjMtx(void) { return XMLoadFloat4x4(&m_mtxView) * XMLoadFloat4x4(&m_mtxProjection); }

	void SetCameraType(CameraType type);
	float GetNearZ(void) { return m_nearZ; }
	float GetFarZ(void) { return m_farZ; }
	float GetFov(void) { return m_fov; }
	float GetAspectRatio(void) { return VIEW_ASPECT; }

private:

	XMFLOAT4X4			m_mtxView;		// ビューマトリックス
	XMFLOAT4X4			m_mtxInvView;		// ビューマトリックス
	XMFLOAT4X4			m_mtxProjection;	// プロジェクションマトリックス
	XMMATRIX			m_projScene;
	XMMATRIX			m_projSkybox;

	XMFLOAT3			m_pos;			// カメラの視点(位置)
	XMFLOAT3			m_at;				// カメラの注視点
	XMFLOAT3			m_up;				// カメラの上方向ベクトル
	XMFLOAT3			m_rot;			// カメラの回転

	float				m_len;			// カメラの視点と注視点の距離
	float				m_fov;
	float				m_nearZ;
	float				m_farZ;

	CameraType			m_CameraType = CameraType::SCENE;

	Timer& m_timer = Timer::get_instance();
	DebugProc& m_debugProc = DebugProc::get_instance();
	InputManager& m_inputManager = InputManager::get_instance();
};