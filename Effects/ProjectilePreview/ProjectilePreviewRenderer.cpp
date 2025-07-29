//=============================================================================
//
// [ProjectilePreviewRenderer.cpp]
// Author : 
//
//=============================================================================
#include "ProjectilePreviewRenderer.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
constexpr float SPHERE_RADIUS = 1.0f;

ProjectilePreviewRenderer::ProjectilePreviewRenderer()
	: m_device(Renderer::get_instance().GetDevice()), m_context(Renderer::get_instance().GetDeviceContext()),
	m_trajectoryVB(nullptr), m_maxTrajectoryPoints(1000),
	m_sphereVB(nullptr), m_sphereIB(nullptr), m_sphereIndexCount(0),
	m_hitPos(XMFLOAT3(0, 0, 0)), m_hasHit(false), m_breathAnimTime(0.0f) {}

ProjectilePreviewRenderer::~ProjectilePreviewRenderer()
{
	ShutDown();
}

void ProjectilePreviewRenderer::Initialize(void)
{
    if (!LoadShaders())
    {
        assert(false && "Failed to load shaders for ProjectilePreviewRenderer");
        return;
    }

	// 軌跡線の頂点バッファを作成
	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.ByteWidth = sizeof(ProjectilePreviewVertex) * m_maxTrajectoryPoints;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_device->CreateBuffer(&vbDesc, nullptr, &m_trajectoryVB);

	// 半球メッシュを作成 (緯度経度分割)
	CreateSphereMesh(16, 8);
}

void ProjectilePreviewRenderer::ShutDown(void)
{
	SafeRelease(&m_trajectoryVB);
	SafeRelease(&m_sphereVB);
	SafeRelease(&m_sphereIB);
}

void ProjectilePreviewRenderer::Update(const SimpleArray<TrajectoryPoint>& trajectoryPoints, const XMFLOAT3& hitPos, bool hasHit)
{
    m_hitPos = hitPos;
    m_hasHit = hasHit;

    // 軌跡点数を更新
    m_currentTrajectoryPoints = trajectoryPoints.getSize();
    if (m_currentTrajectoryPoints > m_maxTrajectoryPoints)
        m_currentTrajectoryPoints = m_maxTrajectoryPoints;

    // 頂点バッファに書き込み
    if (m_trajectoryVB && m_currentTrajectoryPoints > 0) 
    {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(m_context->Map(m_trajectoryVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) 
        {
            ProjectilePreviewVertex* verts = reinterpret_cast<ProjectilePreviewVertex*>(mapped.pData);
            for (UINT i = 0; i < m_currentTrajectoryPoints; ++i) 
            {
                verts[i].pos = trajectoryPoints[i].pos;
            }
            m_context->Unmap(m_trajectoryVB, 0);
        }
    }
}

void ProjectilePreviewRenderer::Draw(void)
{
    // 軌跡を描画
    if (m_trajectoryVB && m_currentTrajectoryPoints > 1) 
    {
        UINT stride = sizeof(ProjectilePreviewVertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_trajectoryVB, &stride, &offset);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

        m_context->Draw(m_currentTrajectoryPoints, 0);
    }

    // 半球を描画
    if (m_hasHit && m_sphereVB && m_sphereIB) {
        // 半球の呼吸アニメーション (スケール変化など) はシェーダー側で実装予定

        UINT stride = sizeof(ProjectilePreviewVertex);
        UINT offset = 0;
        m_context->IASetVertexBuffers(0, 1, &m_sphereVB, &stride, &offset);
        m_context->IASetIndexBuffer(m_sphereIB, DXGI_FORMAT_R32_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        m_context->DrawIndexed(m_sphereIndexCount, 0, 0);
    }
}

void ProjectilePreviewRenderer::CreateSphereMesh(UINT sliceCount, UINT stackCount)
{
    SimpleArray<ProjectilePreviewVertex> vertices;
    SimpleArray<UINT> indices;

    for (UINT stack = 0; stack <= stackCount; ++stack) {
        float phi = (XM_PI / 2.0f) * (float(stack) / stackCount); // 上半球のみ
        float y = SPHERE_RADIUS * sinf(phi);
        float r = SPHERE_RADIUS * cosf(phi);

        for (UINT slice = 0; slice <= sliceCount; ++slice) {
            float theta = XM_2PI * (float(slice) / sliceCount);
            float x = r * cosf(theta);
            float z = r * sinf(theta);

            ProjectilePreviewVertex v;
            v.pos = XMFLOAT3(x, y, z);
            v.normal = XMFLOAT3(x / SPHERE_RADIUS, y / SPHERE_RADIUS, z / SPHERE_RADIUS);
            vertices.push_back(v);
        }
    }

    // インデックス生成
    for (UINT stack = 0; stack < stackCount; ++stack) {
        for (UINT slice = 0; slice < sliceCount; ++slice) {
            UINT i0 = stack * (sliceCount + 1) + slice;
            UINT i1 = i0 + sliceCount + 1;
            UINT i2 = i0 + 1;
            UINT i3 = i1 + 1;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i3);
        }
    }

    // 頂点バッファ作成
    D3D11_BUFFER_DESC vbDesc{};
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.ByteWidth = UINT(sizeof(ProjectilePreviewVertex) * vertices.getSize());

    D3D11_SUBRESOURCE_DATA vbData{};
    vbData.pSysMem = vertices.data();
    m_device->CreateBuffer(&vbDesc, &vbData, &m_sphereVB);

    // インデックスバッファ作成
    D3D11_BUFFER_DESC ibDesc{};
    ibDesc.Usage = D3D11_USAGE_DEFAULT;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.ByteWidth = UINT(sizeof(UINT) * indices.getSize());

    D3D11_SUBRESOURCE_DATA ibData{};
    ibData.pSysMem = indices.data();
    m_device->CreateBuffer(&ibDesc, &ibData, &m_sphereIB);

    m_sphereIndexCount = indices.getSize();
}

bool ProjectilePreviewRenderer::LoadShaders(void)
{
    bool loadShaders = true;

    loadShaders &= ShaderManager::get_instance().HasShaderSet(ShaderSetID::ProjectilePreview);
    if (loadShaders)
        m_shaderSet = ShaderManager::get_instance().GetShaderSet(ShaderSetID::ProjectilePreview);

    return loadShaders;
}
