//*****************************************************************************
// 定数バッファ
//*****************************************************************************

struct WorldMatrixBuffer
{
    matrix world;
    matrix invWorld;
};

struct LightViewProjBuffer
{
    matrix ViewProj[5];
    int LightIndex;
    int padding[3];
};

// マトリクスバッファ
cbuffer WorldBuffer : register(b0)
{
    WorldMatrixBuffer WorldBuffer;
}

cbuffer ViewBuffer : register(b1)
{
    matrix View;
}

cbuffer ProjectionBuffer : register(b2)
{
    matrix Projection;
}

cbuffer ProjViewBuffer : register(b8)
{
    LightViewProjBuffer lightViewProj;
}


cbuffer PerFrameBuffer : register(b12)
{
    float Time;
    float3 WindDirection;
    float WindStrength;
    float padding1;
    float2 NoiseTextureResolution;
    float2 padding2;
}

struct VS_INPUT
{
    float3 Position : POSITION; // 頂点位置
    float3 Normal : NORMAL; // 法線
    float2 TexCoord : TEXCOORD; // テクスチャ座標
    float Weight : TEXCOORD1; // 風影響重み
    float3 Tangent : TANGENT;

    float3 OffsetPosition : POSITION1; // インスタンス位置オフセット
    float4 Rotation : TEXCOORD2; // クォータニオンでの回転
    float4 initialBillboardRot : TEXCOORD3; // 初期ビルボード回転角度
    float Scale : TEXCOORD4; // インスタンススケール
    float Type : TEXCOORD5; // インスタンスタイプ
};


struct VS_OUTPUT
{
    float4 Position : SV_POSITION;  // 出力位置 (クリップ空間)
    float2 TexCoord : TEXCOORD;     // テクスチャ座標
    float4 ShadowCoord : TEXCOORD1; // 影計算用座標
    float3 Normal : NORMAL;         // 出力法線
};

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D DiffuseTexture : register(t0);
Texture2D NoiseTexture : register(t7); // ノイズテクスチャ (風アニメーション用)
Texture2D g_NormalMap : register(t9);
Texture2D g_BumpMap : register(t10);
Texture2D g_OpacityMap : register(t11);
Texture2D g_ReflectMap : register(t12);
Texture2D g_TranslucencyMap : register(t13);
Texture2D ShadowMap : register(t2);
SamplerState SampleType : register(s0); // サンプラーステート
SamplerComparisonState ShadowSampler : register(s1);
//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUTPUT VS(VS_INPUT input)
{
    matrix WorldViewProjection;
    WorldViewProjection = mul(WorldBuffer.world, View);
    WorldViewProjection = mul(WorldViewProjection, Projection);
    
    VS_OUTPUT output;
    
     // クォータニオンを使用して回転を適用
    float4 q = input.Rotation;
    float3 rotatedPosition = input.Position + 2.0 * cross(q.xyz, cross(q.xyz, input.Position) + q.w * input.Position);
    
     // スケールを適用
    rotatedPosition *= input.Scale;

    // 風の影響計算
    //float noiseValue = NoiseTexture.Sample(SampleType, input.TexCoord).r; // ノイズ取得
    int2 texCoordInt = int2(input.TexCoord * float2(256.0f, 256.0f));
    float noiseValue = NoiseTexture.Load(int3(texCoordInt, 0)).r;
    
    float3 windOffset = float3(0, 0, 0);
    // インスタンスタイプが0の場合のみ風アニメーションを適用
    if (input.Type == 0)
    {
        float windEffect = sin(Time * 0.002 + input.Position.x * 0.000002 + input.Position.z * 0.000002 + noiseValue * 0.001); // 風アニメーション計算
        windOffset = input.Weight * WindStrength * windEffect * WindDirection * input.Scale * 25; // 風偏移
    }

    
    // 頂点位置に風の偏移 + インスタンス位置オフセット + スケール適用
    float3 finalPosition = rotatedPosition + windOffset + input.OffsetPosition;

    // 射影空間への変換
    output.Position = mul(float4(finalPosition, 1.0f), WorldViewProjection);

    // 影計算用のライト空間変換
    output.ShadowCoord = mul(float4(finalPosition, 1.0f), lightViewProj.ViewProj[0]);

    // 出力テクスチャ座標と法線
    output.TexCoord = input.TexCoord;
    output.Normal = input.Normal;

    return output;
}

//=============================================================================
// ピクセルシェーダ
//=============================================================================
float CalculateShadow(float4 shadowCoord)
{
    float2 shadowTexCoord = shadowCoord.xy / shadowCoord.w;
    float depth = shadowCoord.z / shadowCoord.w;
    if (shadowTexCoord.x < 0.0 || shadowTexCoord.x > 1.0 || shadowTexCoord.y < 0.0 || shadowTexCoord.y > 1.0)
        return 1.0;
    return ShadowMap.SampleCmpLevelZero(ShadowSampler, shadowTexCoord, depth);
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 baseColor = DiffuseTexture.Sample(SampleType, input.TexCoord);
    
    if (baseColor.a == 0.0f)
        discard;
    
    float3 lightDir = normalize(float3(0.5, -1.0, 0.5));
    float NdotL = saturate(dot(input.Normal, -lightDir));
    float shadowFactor = CalculateShadow(input.ShadowCoord);
    float3 lighting = baseColor.rgb * (0.3 + 0.7 * NdotL * shadowFactor);
    return float4(lighting, baseColor.a);
}


//----------------------------------
// シャドウデプスパス (HLSL)
//----------------------------------

//=============================================================================
// 頂点シェーダ
//=============================================================================
VS_OUTPUT VSShadow(VS_INPUT input)
{
    VS_OUTPUT output;

    //// 影パスではパフォーマンス向上のため風偏移を省略可能
    //float4 a = float4((input.Position * input.Scale + input.OffsetPosition), 1.f); // 最終頂点位置計算

    // ライト空間への変換
    output.Position = mul(float4((input.Position * input.Scale + input.OffsetPosition), 1.f), lightViewProj.ViewProj[0]);
    return output;
}