#include "SharedParticleUtil.hlsl"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_FLUID_SURFACE              b12

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct VS_INPUT_FLUID_SURFACE
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VS_OUTPUT_FLUID_SURFACE
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoord : TEXCOORD;
    float3 ViewDir : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
};


//*****************************************************************************
// 定数バッファ
//*****************************************************************************

cbuffer CBFluidSurface : register(SLOT_CB_FLUID_SURFACE)
{
    float4 g_BaseColor;
    float g_FresnelPower;
    float g_AlphaScale;
    float2 g_Padding;
}

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D g_DiffuseTexture : register(t0);
SamplerState g_SamplerState : register(s0);

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUTPUT_FLUID_SURFACE VS(VS_INPUT_FLUID_SURFACE input)
{
    VS_OUTPUT_FLUID_SURFACE output;

    float4 worldPos = mul(float4(input.Position, 1.0f), g_World);
    float3 worldNormal = normalize(mul(float4(input.Normal, 0.0f), g_World).xyz);

    output.PosH = mul(worldPos, g_ViewProj);
    output.NormalW = worldNormal;
    output.TexCoord = input.TexCoord;
    output.WorldPos = worldPos.xyz;
    output.ViewDir = normalize(g_CameraPos - worldPos.xyz);

    return output;
}

float4 PS(VS_OUTPUT_FLUID_SURFACE input) : SV_TARGET
{
    // フレネル効果
    float fresnel = pow(1.0f - abs(dot(normalize(input.NormalW), normalize(input.ViewDir))), g_FresnelPower);

    // 色の決定
    float3 baseColor = g_BaseColor.rgb;
    float3 finalColor = baseColor + fresnel * 0.3f;

    // 透明度（高さ or テクスチャに応じて制御も可能）
    float alpha = g_BaseColor.a * g_AlphaScale;

    return float4(finalColor, alpha);
}
