//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_FIRE                b5
#define SLOT_UAV_PARTICLE           u0
#define SLOT_SRV_PARTICLE           t6
#define SLOT_TEX_DIFFUSE            t0
#define SLOT_SAMPLER_DEFAULT        s0

//*****************************************************************************
// 定数バッファ
//*****************************************************************************
cbuffer CB_FireEffect : register(SLOT_CB_FIRE)
{
    float g_DeltaTime;
    float g_TotalTime;
    float g_Scale;
    float g_Padding;
    matrix g_World;
    matrix g_ViewProj;
};


struct FireParticle
{
    float3 position;
    float3 velocity;
    float life;
    float size;
    float4 color;
};

struct VS_OUTPUT
{
    uint ParticleID : PARTICLE_ID;
};

struct GS_INPUT
{
    uint ParticleID : PARTICLE_ID;
};

struct GS_OUTPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR0;
};

struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR0;
};

RWStructuredBuffer<FireParticle> g_ParticlesUAV : register(SLOT_UAV_PARTICLE); // 書き込み用
StructuredBuffer<FireParticle> g_ParticlesSRV : register(SLOT_SRV_PARTICLE); // 読み取り用

float Rand(float seed)
{
    return frac(sin(seed * 12.9898) * 43758.5453);
}

//=============================================================================
// コンピュートシェーダ
//=============================================================================
[numthreads(256, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;

    FireParticle p = g_ParticlesUAV[id];

    p.life -= g_DeltaTime * 0.5f;

    if (p.life <= 0)    
    {
        // 粒子を再生成
        p.position = float3(0.0f, 0.0f, 0.0f);
        p.velocity = float3(
            Rand(id + g_TotalTime) * 0.2f - 0.1f,
            1.0f + Rand(id) * 0.5f,
            Rand(id + 100) * 0.2f - 0.1f
        );
        p.life = 1.0f;
        p.size = (0.5f + Rand(id + 128) * 0.5f) * g_Scale; // スケールを適用する！
        p.color = float4(1.0f, 0.5f + Rand(id + 7) * 0.5f, 0.0f, 1.0f);
    }
    else
    {
        // 通常更新
        p.position += p.velocity * g_DeltaTime;
        float noise = sin(g_TotalTime * 5.0f + id) * 0.02f;
        p.position.x += noise;
        p.position.z += noise;
    }

    g_ParticlesUAV[id] = p;
}

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D g_FlameTex : register(SLOT_TEX_DIFFUSE);
SamplerState g_Sampler : register(SLOT_SAMPLER_DEFAULT);

//=============================================================================
// 頂点シェーダ：パーティクルIDをジオメトリシェーダーに渡すだけ
//=============================================================================
void VS(uint vertexID : SV_VertexID, out VS_OUTPUT output)
{
    output.ParticleID = vertexID;
}

//=============================================================================
// ジオメトリシェーダ：粒子をビルボード化して四角形に展開
//=============================================================================
[maxvertexcount(4)]
void GS(point GS_INPUT input[1], inout TriangleStream<GS_OUTPUT> stream)
{
    FireParticle p = g_ParticlesSRV[input[0].ParticleID];

    float3 pos = p.position;
    float size = p.size;

    // ビルボード展開
    float3 right = normalize(float3(g_ViewProj._11, g_ViewProj._21, g_ViewProj._31));
    float3 up = normalize(float3(g_ViewProj._12, g_ViewProj._22, g_ViewProj._32));

    float3 corners[4] =
    {
        pos + (-right + up) * size,
        pos + (right + up) * size,
        pos + (right - up) * size,
        pos + (-right - up) * size
    };

    float2 texcoords[4] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f)
    };

    for (int i = 0; i < 4; ++i)
    {
        GS_OUTPUT o;
        float4 worldPos = mul(float4(corners[i], 1.0f), g_World);
        o.PosH = mul(worldPos, g_ViewProj);
        o.TexCoord = texcoords[i];
        o.Color = p.color * p.life; // 死にかけで透明になる
        stream.Append(o);
    }
}

//=============================================================================
// ピクセルシェーダ：スプライトシートからアニメーションフレームをサンプリング
//=============================================================================
float4 PS(PS_INPUT input) : SV_Target
{
    // 4x4 スプライトシートとして仮定
    const int sheetCols = 4;
    const int sheetRows = 4;
    float frameCount = sheetCols * sheetRows;

    float frame = frac(g_TotalTime * 4.0f) * frameCount;
    int frameIndex = (int) frame;

    int frameX = frameIndex % sheetCols;
    int frameY = frameIndex / sheetCols;

    float2 frameOffset = float2(frameX / (float) sheetCols, frameY / (float) sheetRows);
    float2 frameSize = float2(1.0f / sheetCols, 1.0f / sheetRows);

    float2 uv = input.TexCoord * frameSize + frameOffset;

    float4 texColor = g_FlameTex.Sample(g_Sampler, uv);
    return texColor * input.Color;
}