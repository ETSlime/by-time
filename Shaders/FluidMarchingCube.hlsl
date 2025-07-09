#include "SharedParticleUtil.hlsl"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_FLUID_SURFACE              b12
#define SLOT_SRV_FLUID_THICKNESS           t17
#define SLOT_SAMPLER_DEFAULT               s0
//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct VS_INPUT_MARCHING_CUBES
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};


struct VS_OUTPUT_MARCHING_CUBES
{
    float4 Position : SV_POSITION;
    float3 NormalW : TEXCOORD0;
    float3 ViewDir : TEXCOORD1;
    float2 ScreenUV : TEXCOORD2;
};


//*****************************************************************************
// 定数バッファ
//*****************************************************************************
cbuffer CBMarchingCubes : register(SLOT_CB_FLUID_SURFACE)
{
    float3 g_ShallowColor;
    float g_FresnelPower;
    
    float3 g_DeepColor;
    float g_AlphaScale;
    
    float g_ThicknessScale; // 厚度強度の倍率
    float g_ThicknessBias; // 最小透明度調整用
    float g_ThicknessNormalizeFactor; // 厚度の正規化係数（テクスチャの最大値で割る）
    float g_IsoLevel;
    float2 g_ScreenSize;
    float2 g_InvScreenSize;
}

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D<uint> g_ThicknessTex : register(SLOT_SRV_FLUID_THICKNESS);
SamplerState g_SamplerLinear : register(SLOT_SAMPLER_DEFAULT);

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUTPUT_MARCHING_CUBES VS(VS_INPUT_MARCHING_CUBES input)
{
    VS_OUTPUT_MARCHING_CUBES output;
    
    float4 worldPos = mul(float4(input.Position, 1.0f), g_World);
    output.Position = mul(worldPos, g_ViewProj);

    output.NormalW = mul(float4(input.Normal, 0.0f), g_World).xyz;
    output.ViewDir = g_CameraPos - worldPos.xyz;

    output.ScreenUV = output.Position.xy / output.Position.w;
    output.ScreenUV = output.ScreenUV * 0.5f + 0.5f;

    return output;
}

//-----------------------------------------------------------------------------
// 厚度 + フレネル + アルファ合成用 PS
//-----------------------------------------------------------------------------
float4 PS(VS_OUTPUT_MARCHING_CUBES input) : SV_TARGET
{
    // 法線と視線方向を正規化
    float3 N = normalize(input.NormalW);
    float3 V = normalize(input.ViewDir);
    
    // フレネル効果
    float fresnel = pow(1.0f - abs(dot(N, V)), g_FresnelPower);

    // 厚度テクスチャから値を取得（整数形式）
    int2 pixelCoord = int2(input.ScreenUV * g_ScreenSize);
    uint thicknessInt = g_ThicknessTex.Load(int3(pixelCoord, 0)).r;
    float thickness = float(thicknessInt) * g_ThicknessNormalizeFactor;

    // 厚度に応じた透明度と色の補間
    float t = saturate(thickness * g_ThicknessScale + g_ThicknessBias);
    float thicknessAlpha = smoothstep(0.0f, 1.0f, t);

    float3 thicknessColor = lerp(g_ShallowColor, g_DeepColor, 1.0f - thicknessAlpha);

    // 色にフレネルを加算
    float3 finalColor = thicknessColor + fresnel * 0.3f;

    // アルファ（厚度ベース、カスタムスケーリング）
    float alpha = g_AlphaScale * pow(thicknessAlpha, 1.5f);
    return float4(finalColor, alpha);
}
