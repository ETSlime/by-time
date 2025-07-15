#pragma once
//=============================================================================
//
// [SPHVolumeBuilder.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleStructs.h"
#include "Effects/SPHGridUploader.h"
#include "Collision/VoxelGrid.h"

struct VolumeReconstructionParams
{
	bool useSparseGrid = true; // スパースグリッドを使用するかどうか
    float cellSize = 0.05f;                  // ボクセルのサイズ
    int gridX = 64, gridY = 64, gridZ = 64;  // グリッド解像度
	UINT activeVoxelCount = 0;            // アクティブなボクセル数
    float radius = 0.1f;                     // 粒子半径
    float densityScale = 1.0f;               // 密度スケール
	float isoLevel = 0.5f;                // 等値面のしきい値
    XMFLOAT3 origin = { -1.6f, 0.0f, -1.6f }; // グリッドの原点
};

class SPHVolumeBuilder
{
public:
    void Initialize(UINT maxParticles);
    void InitializeVolume(const VolumeReconstructionParams& params);
    void Clear(void);
    void ClearVolume(void);

	// 粒子からボリュームパラメータを計算
    static VolumeReconstructionParams ComputeVolumeParamsFromParticles(const BOUNDING_BOX& aabb,
        float particleRadius, float densityScale, float isoLevel, float cellSize);

	// 粒子からボリュームを構築
    void AccumulateFromParticles(const SimpleArray<WaterFluidParticle>& particles);
    void AccumulateFromParticlesSparse(const VolumeReconstructionParams& params, const SimpleArray<WaterFluidParticle>& particles);

	const VolumeReconstructionParams GetVolumeReconstructionParams(void) const { return m_params; }
    const SimpleArray<SimpleArray<SimpleArray<float>>>& GetVolume(void) const { return m_volume; }
	const HashMap<VoxelCoord, float, HashVoxelKey64, EqualVoxelKey64>& GetScalarFieldMap(void) const { return m_scalarFieldMap; }

private:
    void RegisterVoxel(const VoxelCoord& coord);

    VolumeReconstructionParams m_params;
    SimpleArray<SimpleArray<SimpleArray<float>>> m_volume;

    UINT m_maxParticles = 0; // 最大パーティクル数

	SimpleArray<VoxelCoord> m_writtenVoxelKeys; // 書き込まれたボクセルキーのリスト（スパースグリッド用）

    // ボクセルキーと三角形のマッピングを保持するハッシュマップ
    HashMap<VoxelCoord, float, HashVoxelKey64, EqualVoxelKey64> m_scalarFieldMap;
};