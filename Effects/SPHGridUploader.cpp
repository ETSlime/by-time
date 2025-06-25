//=============================================================================
//
//  [SPHGridUploader.cpp]
// Author : 
//
//=============================================================================
#include "Effects/SPHGridUploader.h"
#include "Collision/VoxelGrid.h"

void SPHGridUploader::Initialize(float cellSize, UINT maxParticles)
{
	Clear();

	m_cellSize = cellSize;

	m_gridPageHeaders.reserve(MAX_SPH_GRID_PAGE_SIZE);
	m_gridSortedIndices.reserve(maxParticles);

    ParticleEffectRendererBase::CreateStructuredBuffer(m_device, sizeof(SPHGridPageHeader), MAX_SPH_GRID_PAGE_SIZE, nullptr,
		&m_gridPageBufferSet.buffer, &m_gridPageBufferSet.srv, &m_gridPageBufferSet.uav);
}

void SPHGridUploader::Shutdown(void)
{
	SafeRelease(&m_gridPageBufferSet.buffer);
	SafeRelease(&m_gridPageBufferSet.srv);
	SafeRelease(&m_gridPageBufferSet.uav);
	Clear();
}

void SPHGridUploader::Clear(void)
{
    m_gridPageHeaders.clear();
    m_gridSortedIndices.clear();
    m_mortonMap.clear();
}

void SPHGridUploader::BuildGrid(const SimpleArray<WaterFluidParticle>& particles)
{
    Clear();

    UINT numParicles = particles.getSize();
	if (numParicles == 0) return; // パーティクルがいない場合は何もしない

	// グリッドの最小座標をパーティクルの位置から計算
    XMFLOAT3 newGridMin = m_gridMin;
    for (UINT i = 0; i < numParicles; ++i)
    {
        const auto& p = particles[i];
        newGridMin.x = min(newGridMin.x, p.position.x);
        newGridMin.y = min(newGridMin.y, p.position.y);
        newGridMin.z = min(newGridMin.z, p.position.z);
    }

	// グリッドの最小座標をセルサイズで切り捨て
    newGridMin.x = floorf(newGridMin.x / m_cellSize) * m_cellSize;
    newGridMin.y = floorf(newGridMin.y / m_cellSize) * m_cellSize;
    newGridMin.z = floorf(newGridMin.z / m_cellSize) * m_cellSize;

    // グリッドの最小座標を更新
	m_gridMin = newGridMin;

    // 一度マップにコレクト
    for (UINT i = 0; i < numParicles; ++i)
    {
        const auto& p = particles[i];
        XMFLOAT3 rel = { p.position.x - m_gridMin.x, p.position.y - m_gridMin.y, p.position.z - m_gridMin.z };
        UINT coord[3] = {
            static_cast<UINT>(floorf(rel.x / m_cellSize)),
            static_cast<UINT>(floorf(rel.y / m_cellSize)),
            static_cast<UINT>(floorf(rel.z / m_cellSize))
        };
        uint32_t morton = EncodeMorton3(coord[0], coord[1], coord[2]);
        m_mortonMap[morton].push_back(i);
    }

    // ソート＆start index を付与
    uint32_t currentStart = 0;
    for (const auto& it : m_mortonMap)
    {
        SPHGridPageHeader header;
        header.morton = it.key;
        header.count = it.value.getSize();
        header.startIndex = currentStart;

        m_gridPageHeaders.push_back(header);
        for (int i = 0; i < header.count; i++)
            m_gridSortedIndices.push_back(it.value[i]);

        currentStart += header.count;
    }

}

void SPHGridUploader::UploadAndBindPageBuffers(void)
{
    if (!m_gridPageHeaders.empty())
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        HRESULT hr = m_context->Map(m_gridPageBufferSet.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped.pData, m_gridPageHeaders.data(), m_gridPageHeaders.getSize() * sizeof(SPHGridPageHeader));
            m_context->Unmap(m_gridPageBufferSet.buffer, 0);

            m_shaderResourceBinder.BindShaderResource(ShaderStage::CS, SLOT_SRV_SPH_GRID_PAGE_HEADER, m_gridPageBufferSet.srv);
        }
    }
}

void SPHGridUploader::UploadSortedIndicesTo(ID3D11Buffer* dstBuffer) const
{
    if (m_gridSortedIndices.empty()) return;

    m_context->UpdateSubresource(dstBuffer, 0, nullptr, m_gridSortedIndices.data(), 0, 0);
}
