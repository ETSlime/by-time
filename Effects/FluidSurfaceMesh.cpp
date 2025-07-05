//=============================================================================
//
// [FluidSurfaceMesh.cpp]
// Author : 
//
//=============================================================================
#include "FluidSurfaceMesh.h"


FluidSurfaceMesh::FluidSurfaceMesh()
    : m_device(Renderer::get_instance().GetDevice()), 
    m_context(Renderer::get_instance().GetDeviceContext()) 
{
	m_vertices.reserve(m_vertexCapacity);
	m_indices.reserve(m_indexCapacity);
}

void FluidSurfaceMesh::Initialize(void)
{
	if (!LoadShaders())
	{
		assert(false && "Failed to load shaders for FluidSurfaceMesh");
		return;
	}

	HRESULT hr = S_OK;

	// 頂点バッファとインデックスバッファの初期化
    CreateBuffers();

	// 定数バッファの初期化
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.ByteWidth = sizeof(CBFluidSurface);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_cbFluidSurface);
    assert(SUCCEEDED(hr));
}

void FluidSurfaceMesh::GenerateSurfaceMesh(const FluidHeightFieldBuilder& heightField)
{
    m_vertices.clear();
    m_indices.clear();

    const int gridX = heightField.GetGridX();
    const int gridZ = heightField.GetGridZ();
    const float minX = heightField.GetMinX();
    const float minZ = heightField.GetMinZ();
    const float dx = heightField.GetCellSizeX();
    const float dz = heightField.GetCellSizeZ();

	// グリッドの頂点数とインデックス数を計算
    UINT targetVertexCount = gridX * gridZ;
    UINT targetIndexCount = (gridX - 1) * (gridZ - 1) * 6;

    // バッファの容量を確認
    EnsureBufferCapacity(targetVertexCount, targetIndexCount);
    m_vertices.setSize(static_cast<UINT>(targetVertexCount));
    m_indices.setSize(static_cast<UINT>(targetIndexCount));

	// グリッドの各頂点を生成
    UINT indexCursor = 0;
    for (int gz = 0; gz < gridZ; ++gz)
    {
        for (int gx = 0; gx < gridX; ++gx)
        {
            int idx = gz * gridX + gx;

            float x = minX + gx * dx;
            float z = minZ + gz * dz;
            float y = heightField.GetHeight(gx, gz);

            // 頂点生成
            FluidSurfaceVertex v;
            v.position = XMFLOAT3(x, y, z);
            v.texcoord = XMFLOAT2(gx / float(gridX - 1), gz / float(gridZ - 1));
            v.normal = ComputeNormal(heightField, gx, gz);

            m_vertices[idx] = v;

            // インデックス生成
            if (gx < gridX - 1 && gz < gridZ - 1)
            {
                int i0 = idx;
                int i1 = idx + 1;
                int i2 = idx + gridX;
                int i3 = idx + gridX + 1;

                const auto& v0 = m_vertices[i0];
                const auto& v1 = m_vertices[i1];
                const auto& v2 = m_vertices[i2];
                const auto& v3 = m_vertices[i3];

                if (!isnan(v0.position.y) && !isnan(v1.position.y) &&
                    !isnan(v2.position.y) && !isnan(v3.position.y))
                {
                    m_indices[indexCursor++] = i0;
                    m_indices[indexCursor++] = i2;
                    m_indices[indexCursor++] = i1;

                    m_indices[indexCursor++] = i1;
                    m_indices[indexCursor++] = i2;
                    m_indices[indexCursor++] = i3;
                }
            }
        }
    }

	m_indices.setSize(indexCursor); // 実際のインデックス数に合わせてサイズを調整

    // GPUにアップロード
    UpdateBuffers();
}

void FluidSurfaceMesh::UpdateBuffers(void)
{
    if (m_vertexBuffer && !m_vertices.empty())
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, m_vertices.data(), sizeof(FluidSurfaceVertex) * m_vertices.getSize());
            m_context->Unmap(m_vertexBuffer, 0);
        }
    }
    else
        return;

    if (m_indexBuffer && !m_indices.empty())
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (SUCCEEDED(m_context->Map(m_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, m_indices.data(), sizeof(UINT) * m_indices.getSize());
            m_context->Unmap(m_indexBuffer, 0);
        }
    }
    else
        return;

	UpdateCBFluidSurface();
}

void FluidSurfaceMesh::EnsureBufferCapacity(UINT requiredVertexCount, UINT requiredIndexCount)
{
    bool needResize = false;

    if (requiredVertexCount > m_vertexCapacity)
    {
        m_vertexCapacity = max(requiredVertexCount, m_vertexCapacity * 2);
		m_vertices.reserve(static_cast<UINT>(m_vertexCapacity)); // 頂点バッファの容量を確保
        needResize = true;
    }

    if (requiredIndexCount > m_indexCapacity)
    {
        m_indexCapacity = max(requiredIndexCount, m_indexCapacity * 2);
		m_indices.reserve(static_cast<UINT>(m_indexCapacity)); // インデックスバッファの容量を確保
        needResize = true;
    }

    if (needResize)
    {
        ReleaseBuffers(); // GPUバッファを解放
        CreateBuffers();  // 新しい容量で再生成
    }
}

void FluidSurfaceMesh::UpdateCBFluidSurface(void)
{
    CBFluidSurface cbData = {};
    cbData.baseColor = m_baseColor;
    cbData.fresnelPower = m_fresnelPower;
    cbData.alphaScale = m_alphaScale;

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    m_context->Map(m_cbFluidSurface, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &cbData, sizeof(CBFluidSurface));
    m_context->Unmap(m_cbFluidSurface, 0);
}

void FluidSurfaceMesh::Draw(void)
{
    if (!m_vertexBuffer || !m_indexBuffer || m_indices.empty()) return;

    // InputLayout と Shader を切り替える（他の粒子と違うから！）
    m_context->IASetInputLayout(m_shaderSet.inputLayout);
    m_context->VSSetShader(m_shaderSet.vs, nullptr, 0);
    m_context->GSSetShader(nullptr, nullptr, 0); // 必ず GS は切る
    m_context->PSSetShader(m_shaderSet.ps, nullptr, 0);

	// シェーダーリソースのバインド
    m_shaderResourceBinder.BindConstantBuffer(ShaderStage::VS, SLOT_CB_FLUID_SURFACE, m_cbFluidSurface);
    m_shaderResourceBinder.BindConstantBuffer(ShaderStage::PS, SLOT_CB_FLUID_SURFACE, m_cbFluidSurface);

    // 頂点バッファとインデックスバッファの設定
    UINT stride = sizeof(FluidSurfaceVertex);
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 描画実行
    m_context->DrawIndexed(static_cast<UINT>(m_indices.getSize()), 0, 0);
}

bool FluidSurfaceMesh::LoadShaders(void)
{
    bool loadShaders = true;

    loadShaders &= ShaderManager::get_instance().HasShaderSet(ShaderSetID::FluidSurface);
    if (loadShaders)
        m_shaderSet = ShaderManager::get_instance().GetShaderSet(ShaderSetID::FluidSurface);

    return loadShaders;
}

void FluidSurfaceMesh::CreateBuffers(void)
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(FluidSurfaceVertex) * m_vertexCapacity);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&vbDesc, nullptr, &m_vertexBuffer);
    assert(SUCCEEDED(hr) && "Failed to create vertex buffer");

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_DYNAMIC;
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(UINT) * m_indexCapacity);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&ibDesc, nullptr, &m_indexBuffer);
    assert(SUCCEEDED(hr) && "Failed to create index buffer");
}

XMFLOAT3 FluidSurfaceMesh::ComputeNormal(const FluidHeightFieldBuilder& heightField, int gx, int gz)
{
    const int gridX = heightField.GetGridX();
    const int gridZ = heightField.GetGridZ();
    const float dx = heightField.GetCellSizeX();
    const float dz = heightField.GetCellSizeZ();

    float hl = heightField.GetHeightClamped(gx - 1, gz);
    float hr = heightField.GetHeightClamped(gx + 1, gz);
    float hd = heightField.GetHeightClamped(gx, gz - 1);
    float hu = heightField.GetHeightClamped(gx, gz + 1);

    XMFLOAT3 normal;
    normal.x = -(hr - hl) / (2.0f * dx);
    normal.y = 1.0f;
    normal.z = -(hu - hd) / (2.0f * dz);

    XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normal));
    XMStoreFloat3(&normal, n);
    return normal;
}

void FluidSurfaceMesh::ReleaseBuffers(void)
{
	SafeRelease(&m_vertexBuffer);
	SafeRelease(&m_indexBuffer);
}
