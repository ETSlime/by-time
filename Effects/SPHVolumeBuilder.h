#pragma once
//=============================================================================
//
// [SPHVolumeBuilder.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleStructs.h"

struct VolumeReconstructionParams
{
    float cellSize = 0.05f;                  // ボクセルのサイズ
    int gridX = 64, gridY = 64, gridZ = 64;  // グリッド解像度
    XMFLOAT3 origin = { -1.6f, 0.0f, -1.6f }; // グリッドの原点
    float radius = 0.1f;                     // 粒子半径
    float densityScale = 1.0f;               // 密度スケール
	float isoLevel = 0.5f;                // 等値面のしきい値
};

class SPHVolumeBuilder
{
public:
    void Initialize(const VolumeReconstructionParams& params);
    void Clear(void);

	// 粒子からボリュームパラメータを計算
    VolumeReconstructionParams ComputeVolumeParamsFromParticles(const BOUNDING_BOX& aabb,
        float particleRadius, float densityScale, float isoLevel, float cellSize);

	// 粒子からボリュームを構築
    void AccumulateFromParticles(const SimpleArray<WaterFluidParticle>& particles);

	const VolumeReconstructionParams GetVolumeReconstructionParams(void) const { return m_params; }
    const SimpleArray<SimpleArray<SimpleArray<float>>>& GetVolume(void) const { return m_volume; }

private:
    VolumeReconstructionParams m_params;
    SimpleArray<SimpleArray<SimpleArray<float>>> m_volume;
};