//=============================================================================
//
// 見えない枠もきっちり見せちゃう可視化妖精ちゃん [DebugBoundingBoxRenderer.h]
// Author : 
// バウンディングボックスをラインで描いて、モデルや当たり判定の確認に大活躍するデバッグ専用レンダラーですっ！
// ImGuiとも連動して、今どの子を見てるか一目でわかる魔法のサポート係なの!
//
//=============================================================================
#pragma once
#include "Core/Graphics/Renderer.h"
#include "Collision/AABBUtils.h"
#include "Utility/Debugproc.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define DEBUG_BOUNDING_BOX_NAME_LENGTH 64

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct Vertex
{
    XMFLOAT3 position;
};

struct ConstantBuffer
{
    XMMATRIX worldViewProj;
    XMFLOAT4 color;
};

class DebugBoundingBoxRenderer : public IDebugUI
{
public:
    ~DebugBoundingBoxRenderer();

	void Initialize(char* name = nullptr);
	void DrawBox(const BOUNDING_BOX& box, const XMMATRIX& viewProj, const XMFLOAT4& color);

    virtual void RenderImGui(void) override;
    virtual const char* GetPanelName(void) const override { return m_boxName; };

private:
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    ID3D11Buffer* m_constantBuffer;

    XMFLOAT3 m_center = {};
    XMFLOAT3 m_size = {};
    char* m_boxName = nullptr;

    ShaderSet debugShaderSet;

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;


    ShaderResourceBinder& m_ShaderResourceBinder = ShaderResourceBinder::get_instance();
};