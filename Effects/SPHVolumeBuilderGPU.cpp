//=============================================================================
//
// [SPHVolumeBuilderGPU.h]
// Author : 
//
//=============================================================================
#include "SPHVolumeBuilderGPU.h"

void SPHVolumeBuilderGPU::Initialize(void)
{
	m_shaderSet = ShaderManager::get_instance().GetComputeShader(ParticleComputeGroup::WaterFluid, ComputePassType::SPH_Volume_Field);

    // 定数バッファの作成
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBVolumeParams);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;

    HRESULT hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_cbVolumeParams);
    if (FAILED(hr))
    {
        OutputDebugStringA("[FluidThicknessRenderer] Constant Buffer 作成失敗にゃ…！\n");
    }

	m_isInitialized = true;
}

void SPHVolumeBuilderGPU::Release(void)
{
	ReleaseScalarFieldBuffer();
	SafeRelease(&m_cbVolumeParams);
	m_isInitialized = false;
}

void SPHVolumeBuilderGPU::ReleaseScalarFieldBuffer(void)
{
	SafeRelease(&m_scalarFieldBufferSet.buffer);
	SafeRelease(&m_scalarFieldBufferSet.srv);
	SafeRelease(&m_scalarFieldBufferSet.uav);
}

void SPHVolumeBuilderGPU::BuildScalarField(ID3D11ShaderResourceView* particleBufferSRV, const VolumeReconstructionParams& params)
{
	if (!m_isInitialized || !m_shaderSet.cs)
		return;

    CreateResources(params);
	UpdateVolumeParamsCB();

	// コンピュートシェーダーのバインド
	m_context->CSSetShader(m_shaderSet.cs, nullptr, 0);
	// 定数バッファのバインド
	m_shaderResourceBinder.BindConstantBuffer(ShaderStage::CS, SLOT_CB_SPH_VOLUM_PARAMS, m_cbVolumeParams);
	// パーティクルバッファSRVのバインド
	m_shaderResourceBinder.BindShaderResource(ShaderStage::CS, SLOT_SRV_PARTICLE, particleBufferSRV);
	// スカラー場UAVのバインド
	m_shaderResourceBinder.BindUnorderedAccessView(SLOT_UAV_VOLUME_SCALAR_FIELD, m_scalarFieldBufferSet.uav);

	// スレッドグループ数を計算
	UINT threadGroupCountX = (params.gridX + THREAD_COUNT - 1) / THREAD_COUNT;
	UINT threadGroupCountY = (params.gridY + THREAD_COUNT - 1) / THREAD_COUNT;
	UINT threadGroupCountZ = (params.gridZ + THREAD_COUNT - 1) / THREAD_COUNT;
	// コンピュートシェーダーのディスパッチ
	m_context->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	// UAV のリセット
	ID3D11UnorderedAccessView* nullUAV = nullptr;
	m_shaderResourceBinder.BindUnorderedAccessView(SLOT_UAV_VOLUME_SCALAR_FIELD, nullUAV);
}

void SPHVolumeBuilderGPU::CreateResources(const VolumeReconstructionParams& params)
{
    if (params.gridX == m_lastParams.gridX &&
        params.gridY == m_lastParams.gridY &&
        params.gridZ == m_lastParams.gridZ)
        return;

    m_lastParams = params;

    // リソースを解放
	ReleaseScalarFieldBuffer();

    UINT totalCells = params.gridX * params.gridY * params.gridZ;
	// スカラー場バッファの作成
    ParticleEffectRendererBase::CreateStructuredBuffer(
        m_device, 
        sizeof(float),                          // 構造体サイズ（float）
		totalCells,                             // 要素数（グリッドセル数）
        nullptr,                                // 初期データはなし（全て 0）
        &m_scalarFieldBufferSet.buffer,         // バッファ
        &m_scalarFieldBufferSet.srv,            // SRV
        &m_scalarFieldBufferSet.uav,            // UAV
        false,                                  // Append UAV ではない
		false);                                 // CPU 書き込み不可（CS 専用）
}

void SPHVolumeBuilderGPU::UpdateVolumeParamsCB(void)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	m_context->Map(m_cbVolumeParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	CBVolumeParams* params = reinterpret_cast<CBVolumeParams*>(mapped.pData);
	params->volumeOrigin = m_lastParams.origin;
	params->cellSize = m_lastParams.cellSize;
	params->gridDim = XMUINT3(m_lastParams.gridX, m_lastParams.gridY, m_lastParams.gridZ);
	params->radius = m_lastParams.radius;
	params->densityScale = m_lastParams.densityScale;
	params->padding = XMFLOAT3(0.0f, 0.0f, 0.0f); // パディングを追加して構造体のサイズを16バイト境界に揃える

	m_context->Unmap(m_cbVolumeParams, 0);
}


