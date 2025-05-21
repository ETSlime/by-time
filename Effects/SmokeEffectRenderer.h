//=============================================================================
//
// スモークエフェクトちゃん [SmokeEffectRenderer.h]
// Author : 
// - もくもくエモエモな煙を描くよ
// - Compute Shaderでパーティクル更新！GPUの力でふわっと演出
// - DrawIndirectで超大量スモークもサクサク描画
//
//=============================================================================
#pragma once
#include "ParticleEffectRendererBase.h"
#include "Utility/SimpleArray.h"


class SmokeEffectRenderer : public ParticleEffectRendererBase
{
public:

    virtual bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
    virtual void Shutdown(void) override;
    EffectType GetEffectType() const override { return EffectType::Smoke; }
    bool UseDrawIndirect() const override { return true; }

private:
    virtual void DispatchComputeShader(void) override; // パーティクル更新（Dispatch）
    virtual void DrawParticles(void) override;

    bool CreateDiffuseTex(void);

    ID3D11ShaderResourceView* m_diffuseTextureSRV = nullptr; 
};

