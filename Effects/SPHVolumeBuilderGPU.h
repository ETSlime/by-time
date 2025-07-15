#pragma once
//=============================================================================
//
// [SPHVolumeBuilderGPU.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleStructs.h"
#include "Effects/SPHVolumeBuilder.h"
#include "Effects/ParticleEffectRendererBase.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct CBVolumeParams
{
    XMFLOAT3 volumeOrigin;
    float cellSize;

    XMUINT3 gridDim;
    float densityScale;

    float isoRadius;
	float invIsoRadius;
    XMFLOAT2 padding;
};

class SPHVolumeBuilderGPU
{
public:
    SPHVolumeBuilderGPU() {};
    ~SPHVolumeBuilderGPU() { Release(); }

    void Initialize(UINT maxParticles);
    void Release(void);
    void ReleaseScalarFieldBuffer(void);

    void BuildScalarField(ID3D11ShaderResourceView* particleBufferSRV, const VolumeReconstructionParams& params);

    ID3D11UnorderedAccessView* GetScalarFieldUAV(void) const { return m_scalarFieldBufferSet.uav; }
    ID3D11ShaderResourceView* GetScalarFieldSRV(void) const { return m_scalarFieldBufferSet.srv; }

private:
    void EnsureScalarFieldBufferCapacity(const VolumeReconstructionParams& params);
    void UpdateVolumeParamsCB(void);
	bool m_isInitialized = false;

    ParticleBufferSet m_scalarFieldBufferSet;

    ID3D11Buffer* m_cbVolumeParams = nullptr;
    UINT m_scalarFieldCapacity = 0;
	UINT m_maxParticles = 0; // 最大パーティクル数
    ComputeShaderSet m_shaderSet;

    VolumeReconstructionParams m_lastParams;

    ID3D11Device* m_device = Renderer::get_instance().GetDevice();
    ID3D11DeviceContext* m_context = Renderer::get_instance().GetDeviceContext();
    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();
};