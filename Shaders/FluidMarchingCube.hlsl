#include "SharedParticleUtil.hlsl"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_SPH_VOLUM_PARAMS            b11
#define SLOT_CB_FLUID_SURFACE               b12
#define SLOT_SRV_FLUID_THICKNESS            t17
#define SLOT_SRV_VOLUME_SCALAR_FIELD        t18
#define SLOT_SAMPLER_DEFAULT                s0
#define SLOT_UAV_VOLUME_SCALAR_FIELD        u8
#define SLOT_UAV_MARCHING_CUBES_VERTEX      u9
#define SLOT_UAV_MARCHING_CUBES_INDEX       u10
#define SLOT_UAV_MARCHING_CUBES_DRAWARGS    u11
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

// マーチングキューブ用の頂点構造体
struct MarchingCubesVertex
{
    float3 position; // 頂点位置
    float3 normal; // 法線ベクトル
};

struct DrawIndexedInstancedArgs
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
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

cbuffer CBVolumeParams : register(SLOT_CB_SPH_VOLUM_PARAMS)
{
    float3 g_FieldOrigin;
    float g_CellSize;
    uint3 g_GridDim;
    float g_IsoRadius;
    float g_DensityScale;
    float3 VolumParamsPadding;
};


//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D<uint> g_ThicknessTex : register(SLOT_SRV_FLUID_THICKNESS);
SamplerState g_SamplerLinear : register(SLOT_SAMPLER_DEFAULT);

RWStructuredBuffer<float> g_ScalarField_UAV : register(SLOT_UAV_VOLUME_SCALAR_FIELD);
StructuredBuffer<float> g_ScalarField_SRV : register(SLOT_UAV_VOLUME_SCALAR_FIELD);
RWStructuredBuffer<MarchingCubesVertex> g_OutputVertices : register(SLOT_UAV_MARCHING_CUBES_VERTEX);
RWStructuredBuffer<uint> g_OutputIndices : register(SLOT_UAV_MARCHING_CUBES_INDEX);
RWStructuredBuffer<DrawIndexedInstancedArgs> g_DrawArgs : register(SLOT_UAV_MARCHING_CUBES_DRAWARGS);

//=============================================================================
// コンピュートシェーダ
//=============================================================================
[numthreads(8, 8, 8)]
void GenerateScalarFieldCS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_GridDim.x ||
        dispatchThreadId.y >= g_GridDim.y ||
        dispatchThreadId.z >= g_GridDim.z)
        return;

    uint index = dispatchThreadId.x +
                 dispatchThreadId.y * g_GridDim.x +
                 dispatchThreadId.z * g_GridDim.x * g_GridDim.y;

    float3 pos = g_FieldOrigin + g_CellSize * dispatchThreadId;

    // Example: scalar field = sphere SDF
    float d = length(pos - float3(0, 0, 0));
    g_ScalarField_UAV[index] = d - g_IsoRadius;
}

[numthreads(1, 1, 1)]
void MarchingCubesCS(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_GridDim.x - 1 || id.y >= g_GridDim.y - 1 || id.z >= g_GridDim.z - 1)
        return;

    // 1x1x1 voxel に対する 8つの corner scalar を読む
    float corner[8];
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        int3 offset = cornerOffset[i];
        uint3 coord = id + offset;
        uint index = coord.x + coord.y * g_GridDim.x + coord.z * g_GridDim.x * g_GridDim.y;
        corner[i] = g_ScalarField_SRV[index];
    }

    int cubeIndex = 0;
    [unroll]
    for (int i = 0; i < 8; ++i)
        if (corner[i] < g_IsoLevel)
            cubeIndex |= (1 << i);

    int edgeFlags = edgeTable[cubeIndex];
    if (edgeFlags == 0)
        return;

    float3 edgeVertex[12];
    [unroll]
    for (int i = 0; i < 12; ++i)
    {
        if ((edgeFlags & (1 << i)) == 0)
            continue;

        int a = edgeConnection[i][0];
        int b = edgeConnection[i][1];

        float3 p1 = g_FieldOrigin + g_CellSize * (id + cornerOffset[a]);
        float3 p2 = g_FieldOrigin + g_CellSize * (id + cornerOffset[b]);
        float d1 = corner[a];
        float d2 = corner[b];

        float t = saturate((g_IsoLevel - d1) / (d2 - d1 + 1e-6));
        edgeVertex[i] = lerp(p1, p2, t);
    }

    uint triangleCount = 0;
    for (int i = 0; triTable[cubeIndex][i] != -1; i += 3)
    {
        uint baseIndex = InterlockedAdd(g_DrawArgs[0].IndexCountPerInstance, 3);
        [unroll]
        for (int j = 0; j < 3; ++j)
        {
            int edgeIdx = triTable[cubeIndex][i + j];
            float3 pos = edgeVertex[edgeIdx];

            MarchingCubesVertex v;
            v.position = pos;
            v.normal = float3(0, 1, 0); // dummy normal

            uint vertexIdx = baseIndex + j;
            g_OutputVertices[vertexIdx] = v;
            g_OutputIndices[vertexIdx] = vertexIdx;
        }
        triangleCount++;
    }

    g_DrawArgs[0].InstanceCount = 1;
    g_DrawArgs[0].StartIndexLocation = 0;
    g_DrawArgs[0].BaseVertexLocation = 0;
    g_DrawArgs[0].StartInstanceLocation = 0;
}

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
