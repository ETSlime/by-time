#pragma once
//=============================================================================
//
//  [WaterFluidParticleRenderer.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleEffectRendererBase.h"
#include "Effects/SPHSystem.h"
#include "Effects/FluidSurfaceMesh.h"
#include "Collision/VoxelBufferUploader.h"


//*********************************************************
// 構造体
//*********************************************************
struct WaterFluidCB
{
    float supportRadius;      // 影響範囲（半径）
    float restDensity;        // 目標密度（静止状態）
    float particleMass;       // 粒子質量（SPH/PBF共用）

    float stiffness;          // 圧力係数（PBF用）
    float friction;           // 地形との摩擦係数
    float padding1;
    float padding2;
    float padding3;
}; // 水の流体パーティクル用の定数バッファ

struct EmitCB
{
    UINT emitCount;
    XMFLOAT3 emitCenter;
    float  emitRadius;
    XMFLOAT3 _padding; // 16byte alignment
};

class WaterFluidParticleRenderer : public ParticleEffectRendererBase
{
public:
    // 初期化処理
	virtual bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context) override;
    // シャットダウン処理
	virtual void Shutdown(void) override { ParticleEffectRendererBase::Shutdown(); };
    // エフェクトの種類を返す
	EffectType GetEffectType() const override { return EffectType::WaterFluid; }
    // 描画はIndirectで行う
	bool UseDrawIndirect(void) const override { return true; } 
    // パーティクルエフェクトの設定
    virtual void ConfigureEffect(const ParticleEffectParams& params) override;
    // 水の流体パーティクルを指定位置に放出
    void EmitWater(float radius, int emitCount, const XMFLOAT3& center = XMFLOAT3{});
    // 放出するパーティクル数を設定
	void SetEmitCount(UINT count) { m_emitCBData.emitCount = count; }


private:
    virtual void DispatchComputeShader(void) override; // パーティクル更新（Dispatch）
	virtual void DrawParticles(void) override; // パーティクル描画（Draw）

	// パーティクル更新用のCompute Shaderをディスパッチ
    virtual void DispatchEmitParticlesCS(void) override;

	bool CreateWaterFluidCB(void); // 水の流体パーティクル用の定数バッファを作成
	void UploadWaterFluidCB(void); // 水の流体パーティクル用の定数バッファをアップロード
	bool CreateEmitCB(void); // パーティクル放出用の定数バッファを作成
	void UploadEmitCB(void); // パーティクル放出用の定数バッファをアップロード
	void UpdateSurfaceMeshFromSPH(void); // SPHシステムから流体表面メッシュを更新

    float m_supportRadius = 15.0f;     // 近傍半径（PBF）
    float m_restDensity = 300.0f;       // 静止時の密度
    float m_particleMass = 1.0f;      // 粒子質量
    float m_stiffness = 200.0f;         // 圧力（硬さ）
    float m_friction = 0.05f;         // 地形との摩擦
	float m_viscosity = 0.01f;        // 粘性係数（SPH用）
	float m_restitution = 0.5f; // 反発係数（衝突時の挙動制御用）

	SPHSystem m_sphSystem; // SPHシステム（PBF用）
	FluidHeightFieldBuilder m_heightBuilder; // 流体の高さフィールドを構築するためのヘルパー
    FluidSurfaceMesh m_surfaceMesh; // 流体表面メッシュ生成用

	EmitCB m_emitCBData;              // パーティクル放出用の定数バッファデータ
    ID3D11ShaderResourceView* m_waterDotTextureSRV = nullptr;
    ID3D11Buffer* m_cbWaterFluid = nullptr;
    ID3D11Buffer* m_cbEmit = nullptr;

    VoxelBufferUploader& m_voxelBufferUploader = VoxelBufferUploader::get_instance();
};
