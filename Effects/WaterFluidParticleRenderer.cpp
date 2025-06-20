//=============================================================================
//
//  [WaterFluidParticleRenderer.h]
// Author : 
//
//=============================================================================
#include "WaterFluidParticleRenderer.h"
#include "EffectSystem.h"
#include "ParticleStructs.h"

bool WaterFluidParticleRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // ブレンドモードをアルファブレンドに設定
    m_blendMode = BLEND_MODE_ALPHABLEND;

    // 基底クラスの初期化
    if (!ParticleEffectRendererBase::Initialize(device, context))
        return false;

    // シェーダー読み込み
    if (!LoadShaders())
        return false;

    // WaterFluid 専用のCBを作成
	if (!CreateWaterFluidCB())
        return false;

	// EmitCBの作成
	if (!CreateEmitCB())
		return false;

    return true;
}

void WaterFluidParticleRenderer::ConfigureEffect(const ParticleEffectParams& params)
{
    // 基底クラスの共通設定（色、寿命、スケールなど）
    ParticleEffectRendererBase::ConfigureEffect(params);

    // WaterFluidParams型への安全なダウンキャスト
    const WaterFluidParams* wfParams = dynamic_cast<const WaterFluidParams*>(&params);
    if (!wfParams) return;

    // パラメータのコピー
    m_supportRadius = wfParams->supportRadius;
    m_restDensity = wfParams->restDensity;
    m_particleMass = wfParams->particleMass;
    m_stiffness = wfParams->stiffness;
    m_friction = wfParams->friction;
}

void WaterFluidParticleRenderer::EmitWater(float radius, int emitCount, const XMFLOAT3& center)
{
    m_particlesToEmitThisFrame = emitCount;

    // EmitCB 設定
    m_emitCBData.emitCount = emitCount;
    m_emitCBData.emitCenter = center;
    m_emitCBData.emitRadius = radius;

    // Emit専用定数バッファのアップロード
    UploadEmitCB();
}

void WaterFluidParticleRenderer::DispatchComputeShader(void)
{
    // PBF専用のCBをアップロード（例：deltaTime、supportRadiusなど）
    UploadWaterFluidCB();

    // PBF粒子更新用のComputeShaderをバインド
    m_context->CSSetShader(m_updateParticlesCS.cs, nullptr, 0);

	// UpdateParticlesCSのリソースをバインド
    m_voxelBufferUploader.BindVoxelResources();

	// 定数バッファをバインド
    m_shaderResourceBinder.BindConstantBuffer(ShaderStage::CS, SLOT_CB_EFFECT_DRAW, m_cbParticleDraw);

    // スレッドグループ数を計算（1グループ = 64粒子 と仮定）
    UINT threadGroupCount = GET_THREAD_GROUP_COUNT(m_maxParticles, 64);
    m_context->Dispatch(threadGroupCount, 1, 1);

    // Emit処理（新しい水の注入）を基底クラスに任せる
    DispatchEmitParticlesCS();
}

void WaterFluidParticleRenderer::DrawParticles(void)
{
    // デバッグ用の水粒子テクスチャをピクセルシェーダーにバインド
    // 例：ぼやけた半透明な青い円形テクスチャ
    m_shaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, m_waterDotTextureSRV);

    // 必要なら、定数バッファ（例：可視化設定）をバインド
    m_shaderResourceBinder.BindConstantBuffer(ShaderStage::GS, SLOT_CB_EFFECT_PARTICLE, m_cbWaterFluid);

    // DrawIndirect によって粒子を一括描画（PointSprite or Billboard）
    DrawIndirect();
}

void WaterFluidParticleRenderer::DispatchEmitParticlesCS(void)
{
    // 発射数がゼロならスキップ（EmitWater呼び出されていない）
    if (m_emitCBData.emitCount == 0 || !m_emitParticlesCS.cs)
        return;

    // Emit用CSの準備
    PrepareForEmitCS();

    // Emit用CSのバインド
    m_context->CSSetShader(m_emitParticlesCS.cs, nullptr, 0);

    // スレッドグループ数を計算（Emitは1スレッド = 1粒子）
    UINT threadGroupCount = GET_THREAD_GROUP_COUNT(m_emitCBData.emitCount, 64); // スレッドグループ数計算
    m_context->Dispatch(threadGroupCount, 1, 1);

    // 使用済みなのでゼロリセット（次フレームまた呼び出される）
    m_emitCBData.emitCount = 0;
}

bool WaterFluidParticleRenderer::CreateWaterFluidCB(void)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(WaterFluidCB);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;

    HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_cbWaterFluid);
    if (FAILED(hr))
        return false;

    return true;
}

void WaterFluidParticleRenderer::UploadWaterFluidCB(void)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_context->Map(m_cbWaterFluid, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        WaterFluidCB* cb = reinterpret_cast<WaterFluidCB*>(mapped.pData);
        cb->supportRadius = m_supportRadius;        // 近傍探索半径（PBF）
        cb->restDensity = m_restDensity;            // 目標密度（初期状態）
        cb->particleMass = m_particleMass;          // 粒子の質量
        cb->stiffness = m_stiffness;                // 弾性係数
        cb->friction = m_friction;                  // 地形接触時の摩擦
        cb->padding1 = 0.0f;
        cb->padding2 = 0.0f;
        cb->padding3 = 0.0f;

        m_context->Unmap(m_cbWaterFluid, 0);

        // CS に定数バッファをセット
        m_shaderResourceBinder.BindConstantBuffer(ShaderStage::CS, SLOT_CB_EFFECT_PARTICLE, m_cbWaterFluid);
    }
}

bool WaterFluidParticleRenderer::CreateEmitCB(void)
{
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(EmitCB);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;

    HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_cbEmit);
    if (FAILED(hr))
        return false;

    return true;
}

void WaterFluidParticleRenderer::UploadEmitCB(void)
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_context->Map(m_cbEmit, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return;

    // 構造体にデータ書き込み
    EmitCB* cb = reinterpret_cast<EmitCB*>(mapped.pData);
    *cb = m_emitCBData;

    m_context->Unmap(m_cbEmit, 0);

    // CSにバインド
    m_shaderResourceBinder.BindConstantBuffer(ShaderStage::CS, SLOT_CB_EFFECT_EMIT, m_cbEmit);
}
