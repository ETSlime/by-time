//*****************************************************************************
// 定数バッファ
//*****************************************************************************
struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // 変換行列を適用
    output.pos = float4(input.pos, 0.0f, 1.0f);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

//=============================================================================
// ピクセルシェーダ
//=============================================================================
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 texColor = g_Texture.Sample(g_SamplerState, input.uv);
    return texColor * input.color;
}