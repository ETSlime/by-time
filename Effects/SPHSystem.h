#pragma once
//=============================================================================
//
//  [SPHSystem.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleEffectRendererBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
constexpr UINT MAX_PARTICLES_PER_CELL = 64;

//*********************************************************
// 構造体
//*********************************************************
struct CBSPHSimParams
{
    float    smoothingRadius;       // h (SPH 平滑半径)
    float    particleMass;          // 質量 m
    float    restDensity;           // 目標密度 ρ0
    float    pressureMultiplier;    // 圧力係数 k (圧力 = k * (密度 - ρ0))

	float    viscosity;             // 粘性係数 ν (粘性力 = ν * (速度差 / 距離))
    XMFLOAT3 acceleration;          // 外力 (重力など)

    XMUINT3  gridDim;               // グリッド分割数 (x,y,z)
	UINT     gridCellStart;         // グリッド開始インデックス (0 から始まる)

    XMFLOAT3 gridMin;               // ワールド空間でのグリッド最小座標
	UINT     maxParticleCount;      // 総粒子数

    float    cellSize;              // 1 セルの大きさ (= h 推奨)
    UINT     maxParticlesPerCell;   // 1 セルあたりの最大パーティクル数

	float    deltaTime;             // タイムステップ (Δt)
	float    padding;               // 16バイトアライメント用のパディング
};

// SPH シミュレーションの設定
struct SPHConfig
{
    UINT     maxParticles = 0;  // 最大パーティクル数
    XMFLOAT3 gridMin = { -100.0f, -100.0f, -100.0f }; // グリッドの最小座標
    float    cellSize = 1.0f; // グリッドセルのサイズ
    XMUINT3  gridDim = { 128, 128, 128 };  // グリッドの分割数 (x, y, z)
    UINT     maxParticlesPerCell = MAX_PARTICLES_PER_CELL; // セルあたりの最大パーティクル数
	float    smoothingRadius = 1.0f; // SPH 平滑半径 (h)
    float    particleMass = 1.0f; // パーティクル質量
    float    restDensity = 1.0f; // 静止時の密度
    float    pressureMultiplier = 0.1f; // 圧力係数
    float    viscosity = 0.01f; // 粘性係数
    XMFLOAT3 acceleration = { 0.0f, -9.81f, 0.0f }; // 重力などの外力
};

class SPHSystem
{
public:
    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, const SPHConfig& config, ParticleBufferSet particleBufferSet);
    void Update(void);
    void Shutdown(void);
    void SetConfig(const SPHConfig& config); // 設定を更新
    void SwapGridBuffers(void) { m_currentGridIndex = 1 - m_currentGridIndex; }

private:
	bool CreateConstantBuffer(void); // 定数バッファの作成
	void UpdateSPHSimParamsCB(void); // SPH シミュレーションパラメータの更新
    bool LoadComputeShaders(void); // ComputeShaderの読み込み
    void BindResources_BuildGrid(void); // グリッド構築ステージに必要なリソースをバインド
    void BindResources_Simulate(void); // SPH シミュレーションステージに必要なリソースをバインド


	ID3D11Buffer* m_cbSPHParams = nullptr; // SPH シミュレーションパラメータ用定数バッファ

    ParticleBufferSet m_sortedIndicesBufferSet; // パーティクルソート用インデックスバッファセット
	ParticleBufferSet m_gridCounterBufferSet; // グリッドカウンターバッファセット
    ParticleBufferSet m_gridStartIndexBufferSet[2]; // グリッド開始インデックスバッファセット
    int m_currentGridIndex = 0; // 現在のグリッドバッファインデックス (0 または 1)

	SPHConfig m_config; // SPH シミュレーション設定

    UINT m_totalGridCells = 0; // 総グリッドセル数
    UINT m_gridCellStart = 0; // グリッド開始インデックス (0 から始まる)

    // パーティクルバッファセット
    ParticleBufferSet m_particleBufferSet;

    ComputeShaderSet m_buildGridCS;
    ComputeShaderSet m_simulateCS;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    Timer& m_timer = Timer::get_instance();
    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();
};
