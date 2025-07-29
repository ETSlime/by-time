//=============================================================================
//
// ƒŒƒ“ƒ_ƒŠƒ“ƒOˆ— [DebugBoundingBoxRenderer.cpp]
// Author : 
//
//=============================================================================
#include "Utility/Debug/DebugBoundingBoxRenderer.h"


DebugBoundingBoxRenderer::~DebugBoundingBoxRenderer()
{
    delete[] m_boxName;

    Shutdown();
}

void DebugBoundingBoxRenderer::Initialize(const char* name)
{
    if (name)
        m_boxName = const_cast<char*>(name);

    DebugProc::get_instance().Register(this);

    m_device = Renderer::get_instance().GetDevice();
    m_context = Renderer::get_instance().GetDeviceContext();

    // 8 corner vertices
    DebugVertex vertices[] = 
    {
        {{-0.5f, -0.5f, -0.5f}},
        {{-0.5f, +0.5f, -0.5f}},
        {{+0.5f, +0.5f, -0.5f}},
        {{+0.5f, -0.5f, -0.5f}},
        {{-0.5f, -0.5f, +0.5f}},
        {{-0.5f, +0.5f, +0.5f}},
        {{+0.5f, +0.5f, +0.5f}},
        {{+0.5f, -0.5f, +0.5f}},
    };

    uint16_t indices[] = 
    {
        0,1, 1,2, 2,3, 3,0, // bottom
        4,5, 5,6, 6,7, 7,4, // top
        0,4, 1,5, 2,6, 3,7  // sides
    };

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.ByteWidth = sizeof(vertices);
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = vertices;
    m_device->CreateBuffer(&vbDesc, &vbData, &m_vertexBuffer);

    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.ByteWidth = sizeof(indices);
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = indices;
    m_device->CreateBuffer(&ibDesc, &ibData, &m_indexBuffer);

    m_debugShaderSet = ShaderManager::get_instance().GetShaderSet(ShaderSetID::Debug);

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.ByteWidth = sizeof(DebugConstantBuffer);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
}

void DebugBoundingBoxRenderer::Shutdown(void)
{
    DebugProc::get_instance().Unregister(this);
    SafeRelease(&m_vertexBuffer);
    SafeRelease(&m_indexBuffer);
    SafeRelease(&m_constantBuffer);
}

void DebugBoundingBoxRenderer::DrawBox(const BOUNDING_BOX& box, const XMMATRIX& viewProj, const XMFLOAT4& color)
{
    XMVECTOR minPt = XMLoadFloat3(&box.minPoint);
    XMVECTOR maxPt = XMLoadFloat3(&box.maxPoint);
    XMVECTOR centerVec = (minPt + maxPt) * 0.5f;
    XMVECTOR sizeVec = (maxPt - minPt);

    XMStoreFloat3(&m_center, centerVec);
    XMStoreFloat3(&m_size, sizeVec);

    XMMATRIX world = XMMatrixScalingFromVector(sizeVec) * XMMatrixTranslationFromVector(centerVec);
    XMMATRIX wvp = XMMatrixTranspose(world * viewProj);

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    m_context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    DebugConstantBuffer* cb = reinterpret_cast<DebugConstantBuffer*>(mapped.pData);
    cb->worldViewProj = wvp;
    cb->color = color;
    m_context->Unmap(m_constantBuffer, 0);

    UINT stride = sizeof(DebugVertex);
    UINT offset = 0;
    m_context->IASetInputLayout(m_debugShaderSet.inputLayout);
    m_context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    m_context->VSSetShader(m_debugShaderSet.vs, nullptr, 0);
    m_context->PSSetShader(m_debugShaderSet.ps, nullptr, 0);
    m_ShaderResourceBinder.BindConstantBuffer(ShaderStage::VS, SLOT_CB_DEBUG_BOUNDING_BOX, m_constantBuffer);
    m_ShaderResourceBinder.BindConstantBuffer(ShaderStage::PS, SLOT_CB_DEBUG_BOUNDING_BOX, m_constantBuffer);

    m_context->DrawIndexed(24, 0, 0);
}

void DebugBoundingBoxRenderer::RenderImGui(void)
{
    ImGui::Text("Box Pos: %.1f %.1f %.1f", m_center.x, m_center.y, m_center.z);
    ImGui::Text("Box Size: %.1f %.1f %.1f", m_size.x, m_size.y, m_size.z);
}