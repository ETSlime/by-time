//=============================================================================
//
// [DebugTriangleRenderer.h]
// Author : 
//
//=============================================================================
#include "Utility/Debug/DebugTriangleRenderer.h"

DebugTriangleRenderer::~DebugTriangleRenderer()
{
    delete[] m_panelName;

    Shutdown();
}

void DebugTriangleRenderer::Initialize(const char* name)
{
    if (name)
        m_panelName = const_cast<char*>(name);

    DebugProc::get_instance().Register(this);

    m_device = Renderer::get_instance().GetDevice();
    m_context = Renderer::get_instance().GetDeviceContext();

    // 初期頂点数を仮で確保（毎フレーム動的に更新可能）
    m_maxVertices = INITIALIZE_DEBUG_TRIANGLE_COUNT * 3;
    m_triangles.reserve(m_maxVertices);
    CreateVertexBuffer(m_maxVertices);

    // 定数バッファ作成
    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(DebugConstantBuffer);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&cbd, nullptr, &m_constantBuffer);

    // デバッグ用シェーダーの読み込み
    m_debugShaderSet = ShaderManager::get_instance().GetShaderSet(ShaderSetID::Debug);
}

void DebugTriangleRenderer::Shutdown(void)
{
    DebugProc::get_instance().Unregister(this);
    SafeRelease(&m_vertexBuffer);
    SafeRelease(&m_constantBuffer);
}

void DebugTriangleRenderer::DrawTriangles(const SimpleArray<Triangle>& triangles, const XMMATRIX& viewProj, const XMFLOAT4& color, bool wireframe)
{
    if (!m_context || !m_vertexBuffer) return;

    UINT vertexCount = triangles.getSize() * 3;
    if (vertexCount > m_maxVertices)
    {
        // 頂点バッファを拡張
        if (m_vertexBuffer) m_vertexBuffer->Release();
        m_maxVertices = vertexCount;
        CreateVertexBuffer(m_maxVertices);
    }

    // 三角形を展開して頂点データを GPU にコピー
	m_triangles.clear();

    for (UINT i = 0; i < triangles.getSize(); ++i)
    {
        const Triangle& tri = triangles[i];
        m_triangles[i * 3 + 0] = DebugVertex{ tri.v0 };
        m_triangles[i * 3 + 1] = DebugVertex{ tri.v1 };
        m_triangles[i * 3 + 2] = DebugVertex{ tri.v2 };
    }

    D3D11_MAPPED_SUBRESOURCE mapped{};
    m_context->Map(m_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, m_triangles.data(), sizeof(XMFLOAT3) * vertexCount);
	m_context->Unmap(m_vertexBuffer, 0);

	// 定数バッファの更新
    D3D11_MAPPED_SUBRESOURCE mappedCB{};
    m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedCB);
    DebugConstantBuffer* cb = reinterpret_cast<DebugConstantBuffer*>(mappedCB.pData);
    cb->worldViewProj = XMMatrixTranspose(viewProj);
    cb->color = color;
    m_context->Unmap(m_constantBuffer, 0);


    // シェーダーとレイアウトをセット
    UINT stride = sizeof(XMFLOAT3);
    UINT offset = 0;
    m_context->IASetInputLayout(m_debugShaderSet.inputLayout);
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_context->VSSetShader(m_debugShaderSet.vs, nullptr, 0);
    m_context->PSSetShader(m_debugShaderSet.ps, nullptr, 0);

    m_ShaderResourceBinder.BindConstantBuffer(ShaderStage::VS, SLOT_CB_DEBUG_BOUNDING_BOX, m_constantBuffer);
    m_ShaderResourceBinder.BindConstantBuffer(ShaderStage::PS, SLOT_CB_DEBUG_BOUNDING_BOX, m_constantBuffer);

	if (wireframe)
	{
		m_Renderer.SetFillMode(D3D11_FILL_WIREFRAME);
        m_context->Draw(vertexCount, 0);
		m_Renderer.SetFillMode(D3D11_FILL_SOLID);
	}
	else
	{
        m_context->Draw(vertexCount, 0);
	}
}

void DebugTriangleRenderer::CreateVertexBuffer(UINT maxVertices)
{
    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = sizeof(DebugVertex) * maxVertices;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&vbd, nullptr, &m_vertexBuffer);
}
