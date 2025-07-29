#pragma once
//=============================================================================
//
// [DebugTriangleRenderer.h]
// Author : 
//
//=============================================================================
#include "Core/Graphics/Renderer.h"
#include "Collision/AABBUtils.h"
#include "Utility/Debug/Debugproc.h"
#include "Utility/Debug/DebugBoundingBoxRenderer.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define INITIALIZE_DEBUG_TRIANGLE_COUNT 64 // 初期三角形数

// デバッグ用三角形描画クラス
class DebugTriangleRenderer : public IDebugUI
{
public:
    ~DebugTriangleRenderer();

    void Initialize(const char* name = nullptr);
    void Shutdown(void);

    // 三角形を描画する関数（単一または複数）
    void DrawTriangles(const SimpleArray<Triangle>& triangles,
        const XMMATRIX& viewProj, const XMFLOAT4& color,
        bool wireframe = true);

    virtual const char* GetPanelName(void) const override { return m_panelName; };

private:
    void CreateVertexBuffer(UINT maxVertices);

    char* m_panelName = nullptr;
    UINT m_maxVertices = 0;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
	SimpleArray<DebugVertex> m_triangles;
    ShaderSet m_debugShaderSet;
    ShaderResourceBinder& m_ShaderResourceBinder = ShaderResourceBinder::get_instance();
	Renderer& m_Renderer = Renderer::get_instance();
};