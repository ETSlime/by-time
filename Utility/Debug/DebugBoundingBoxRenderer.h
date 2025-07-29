#pragma once
//=============================================================================
//
// 見えない枠もきっちり見せちゃう可視化妖精ちゃん [DebugBoundingBoxRenderer.h]
// Author : 
// バウンディングボックスをラインで描いて、モデルや当たり判定の確認に大活躍するデバッグ専用レンダラーですっ！
// ImGuiとも連動して、今どの子を見てるか一目でわかる魔法のサポート係なの!
//
//=============================================================================
#include "Core/Graphics/Renderer.h"
#include "Collision/AABBUtils.h"
#include "Utility/Debug/Debugproc.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct DebugVertex
{
    XMFLOAT3 position;
};

struct DebugConstantBuffer
{
    XMMATRIX worldViewProj;
    XMFLOAT4 color;
};

class DebugBoundingBoxRenderer : public IDebugUI
{
public:
    ~DebugBoundingBoxRenderer();

	void Initialize(const char* name = nullptr);
    void Shutdown(void);
	void DrawBox(const BOUNDING_BOX& box, const XMMATRIX& viewProj, const XMFLOAT4& color);

    virtual void RenderImGui(void) override;
    virtual const char* GetPanelName(void) const override { return m_boxName; };

private:
    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
    ID3D11Buffer* m_constantBuffer = nullptr;

    XMFLOAT3 m_center = {};
    XMFLOAT3 m_size = {};
    char* m_boxName = nullptr;

    ShaderSet m_debugShaderSet;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    ShaderResourceBinder& m_ShaderResourceBinder = ShaderResourceBinder::get_instance();
};