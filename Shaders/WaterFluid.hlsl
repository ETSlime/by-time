//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_EFFECT_UPDATE       b5
#define SLOT_CB_EFFECT_DRAW         b6
#define SLOT_CB_EFFECT_PARTICLE     b7
#define SLOT_CB_EFFECT_EMIT         b10
#define SLOT_UAV_PARTICLE           u0
#define SLOT_UAV_ALIVE_LIST         u1
#define SLOT_UAV_FREE_LIST          u2
#define SLOT_UAV_FREE_LIST_CONSUME  u3
#define SLOT_SRV_PARTICLE           t6
#define SLOT_SRV_ALIVE_LIST         t7
#define SLOT_TEX_DIFFUSE            t0
#define SLOT_SAMPLER_DEFAULT        s0
#define SLOT_CB_VIEW_MATRIX         b1
//*****************************************************************************
// 定数バッファ
//*****************************************************************************
cbuffer CBParticleUpdate : register(SLOT_CB_EFFECT_UPDATE)
{
    float3 g_Acceleration; // 加速度（例：float3(0, 1.5f, 0)）
    float g_Scale; // サイズスケール
    
    float g_DeltaTime; // 経過時間
    float g_TotalTime; // 全体経過時間
    float g_LifeMin; // 最小ライフ
    float g_LifeMax; // 最大ライフ
    float g_SpawnRateMin; // 毎秒発射数
    float g_SpawnRateMax; // 毎秒発射数
    
    uint g_MaxParticleCount; // 最大パーティクル数
    uint g_ParticlesToEmitThisFrame; // このフレームに発射するパーティクル数
    
    float4 g_startColor;
    float4 g_endColor;
};

cbuffer CBParticleDraw : register(SLOT_CB_EFFECT_DRAW)
{
    matrix g_World;
    matrix g_ViewProj;
};

cbuffer ViewBuffer : register(SLOT_CB_VIEW_MATRIX)
{
    matrix g_View;
}

// 水の流体パーティクル用の定数バッファ
cbuffer WaterFluidCB : register(SLOT_CB_EFFECT_PARTICLE)
{
    float g_SupportRadius;
    float g_RestDensity;
    float g_ParticleMass;
    float g_Stiffness;
    float g_Friction;
    float3 padding;
};

// 粒子構造体
struct WaterFluidParticle
{
    float3 position;            // 現在位置
    float density;              // 密度（周囲の粒子から算出）

    float3 velocity;            // 速度
    float lambda;               // 拘束条件解（位置補正のための係数）

    float3 predictedPosition;   // 予測位置（次の位置、拘束前）
    float lifeRemaining;        // 寿命（将来拡張用、今は1で固定でも可）

    float4 color;               // デバッグ用カラー（密度に応じた可視化など）
    
    float size;                 // サイズ
    float3 padding;
};

cbuffer EmitCB : register(SLOT_CB_EFFECT_EMIT)
{
    uint g_EmitCount;
    float3 g_EmitCenter;
    float g_EmitRadius;
    float3 _padding;
};

struct VS_INPUT
{
    uint instanceID : SV_InstanceID; // DrawInstancedIndirect で渡される
};

// VS → 単にParticleIDを渡す
struct VS_OUTPUT
{
    uint ParticleID : PARTICLE_ID;
};

struct GS_INPUT
{
    uint ParticleID : PARTICLE_ID;
};

// GS → Billboard展開
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

float Rand(float seed)
{
    return frac(sin(seed * 12.9898) * 43758.5453);
}

//*****************************************************************************
// グローバル変数
//*****************************************************************************
Texture2D g_DiffuseTex : register(SLOT_TEX_DIFFUSE); // ノイズテクスチャ
SamplerState g_Sampler : register(SLOT_SAMPLER_DEFAULT); // サンプラーステート

// バッファ
AppendStructuredBuffer<uint> g_AliveListUAV : register(SLOT_UAV_ALIVE_LIST); // 書き込み用生存パーティクルリスト
AppendStructuredBuffer<uint> g_FreeListUAV : register(SLOT_UAV_FREE_LIST); // 書き込み用空きパーティクルリスト
RWStructuredBuffer<WaterFluidParticle> g_ParticlesUAV : register(SLOT_UAV_PARTICLE); // 書き込み用パーティクルデータバッファ
StructuredBuffer<WaterFluidParticle> g_ParticlesSRV : register(SLOT_SRV_PARTICLE); // 読み取り用パーティクルデータバッファ
StructuredBuffer<uint> g_AliveListSRV : register(SLOT_SRV_ALIVE_LIST); // 読み取り用生存パーティクルリスト
ConsumeStructuredBuffer<uint> g_FreeListConsumeUAV : register(SLOT_UAV_FREE_LIST_CONSUME); // 読み取り用空きパーティクルリスト

//=============================================================================
// コンピュートシェーダ
//=============================================================================
[numthreads(64, 1, 1)]
void UpdateCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_MaxParticleCount)
        return;

    WaterFluidParticle p = g_ParticlesUAV[id];
    
    // ==== 生存している場合：更新処理 ====
    if (p.lifeRemaining > 0.0f)
    {
        float3 acc = g_Acceleration;

        p.velocity *= (1.0 - g_Friction);
        p.velocity += acc * g_DeltaTime;
        p.position += p.velocity * g_DeltaTime;

        p.predictedPosition = p.position; // 今回は仮に = position（PBD未実装）

        // データ書き戻し
        g_ParticlesUAV[id] = p;
    
        // 活発な粒子を AliveList に登録
        g_AliveListUAV.Append(id);
    }
    else
    {
        // 死亡した場合は FreeList に登録
        g_FreeListUAV.Append(id);
    }


}

float3 RandomInsideUnitSphere(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;

    state = state * 747796405u + 2891336453u;
    float x = frac(state / 4294967296.0);

    state = state * 747796405u + 2891336453u;
    float y = frac(state / 4294967296.0);

    state = state * 747796405u + 2891336453u;
    float z = frac(state / 4294967296.0);

    x = x * 2.0f - 1.0f;
    y = y * 2.0f - 1.0f;
    z = z * 2.0f - 1.0f;

    float3 v = float3(x, y, z);
    float len = length(v);

    // fallback にほんの少しノイズを足す（絶対 0 にしない）
    if (len > 1.0f || len == 0.0f)
        return float3(0.001f * seed, 0.0f, 0.001f * seed);

    return normalize(v);
}


[numthreads(64, 1, 1)]
void EmitCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_EmitCount)
        return;

    uint slotID = g_FreeListConsumeUAV.Consume();
    WaterFluidParticle p = (WaterFluidParticle) 0; // 明示的に全フィールドを初期化

    uint seed = asuint(g_TotalTime * 1000.0f) + dispatchThreadID.x;
    float3 offset = RandomInsideUnitSphere(seed);
    p.position = g_EmitCenter + offset * g_EmitRadius;
    p.velocity = float3(0, 0, 0);
    p.predictedPosition = p.position;

    p.density = g_RestDensity;
    p.lambda = 0.0f;
    p.lifeRemaining = g_LifeMax; // 寿命は最大値で初期化

    p.color = g_startColor; // 初期カラー
    p.size = g_Scale;
    

    g_ParticlesUAV[slotID] = p;
    g_AliveListUAV.Append(slotID);
}

//=============================================================================
// 頂点シェーダ：パーティクルIDをジオメトリシェーダーに渡すだけ
//=============================================================================
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    // インスタンスIDを使って、実際の粒子インデックスを取得
    output.ParticleID = g_AliveListSRV[input.instanceID];

    return output;
}

//=============================================================================
// ジオメトリシェーダ：粒子をビルボード化して四角形に展開
//=============================================================================
[maxvertexcount(6)]
void GS(point GS_INPUT input[1], inout TriangleStream<GS_OUTPUT> stream)
{
    WaterFluidParticle p = g_ParticlesSRV[input[0].ParticleID];

    float3 pos = p.position;
    float size = p.size;

    // ビルボード展開（Viewの1列目と2列目から）
    float3 right = normalize(float3(g_View._11, g_View._21, g_View._31));
    float3 up = normalize(float3(g_View._12, g_View._22, g_View._32));

    // 左上 → 右上 → 右下 → 左下 順に展開
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

    int triangleIndices[6] =
    {
        0, 1, 2, // 第1三角形
        2, 3, 0 // 第2三角形
    };
    
    for (int i = 0; i < 6; ++i)
    {
        int idx = triangleIndices[i];
        GS_OUTPUT o;
        float4 worldPos = mul(float4(corners[idx], 1.0f), g_World);
        o.PosH = mul(worldPos, g_ViewProj);
        o.TexCoord = texcoords[idx];
        o.Color = p.color;
        stream.Append(o);

        // 3個ごとにRestartStrip()
        if ((i + 1) % 3 == 0)
        {
            stream.RestartStrip();
        }
    }
}

//=============================================================================
// ピクセルシェーダ：ノイズで破れた煙を表現
//=============================================================================
float4 PS(PS_INPUT input) : SV_Target
{
    return input.Color;
}