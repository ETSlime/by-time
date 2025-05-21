//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_EFFECT_UPDATE       b5
#define SLOT_CB_EFFECT_DRAW         b6
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

// 粒子構造体
struct BillboardSimpleParticle
{
    float3 position;
    float3 velocity;
    float size;
    float life;
    float lifeRemaining;
    float rotation;
    float4 color;
    float4 startColor;
    float4 endColor;
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
RWStructuredBuffer<BillboardSimpleParticle> g_ParticlesUAV : register(SLOT_UAV_PARTICLE); // 書き込み用パーティクルデータバッファ
StructuredBuffer<BillboardSimpleParticle> g_ParticlesSRV : register(SLOT_SRV_PARTICLE); // 読み取り用パーティクルデータバッファ
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
        return; // 範囲外は無視

    BillboardSimpleParticle p = g_ParticlesUAV[id];
    
    // ==== 生存している場合：更新処理 ====
    if (p.lifeRemaining > 0.0f)
    {
        // 残り寿命を減少
        p.lifeRemaining -= g_DeltaTime;

        // 速度更新
        p.velocity += g_Acceleration * g_DeltaTime;

        // 位置更新
        p.position += p.velocity * g_DeltaTime;

        // サイズ成長
        p.size += g_DeltaTime * g_Scale * 0.1f;
        
        // 死にかけで透明になる
        float lifeRatio = saturate(p.lifeRemaining / p.life);
        p.color = p.startColor * lifeRatio;
        
        // 書き戻し
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

[numthreads(64, 1, 1)]
void EmitCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_ParticlesToEmitThisFrame)
        return;

    // 空きスロットを取得
    uint slotID = g_FreeListConsumeUAV.Consume();

    // 乱数生成（TotalTimeで時間依存性）
    float seed = frac(sin(id * 17.3f + g_TotalTime) * 43758.5453f);
    float randA = frac(sin(id * 31.7f + g_TotalTime) * 12345.67f);
    float randB = frac(sin(id * 61.3f + g_TotalTime) * 67891.23f);
    float randAngle = randB * 6.2831f;
    float radius = randA * 0.2f * g_Scale;

    // パーティクルを再生成する処理（位置・速度・サイズなど初期化）
    BillboardSimpleParticle p = (BillboardSimpleParticle) 0; // 明示的に全フィールドを初期化
    
    // 初期位置（中心周辺に少し散らす）
    p.position = float3(cos(randAngle), 0, sin(randAngle)) * radius;

    // 初期速度（上方向 + 乱れ）    
    float3 horiz = float3(cos(randAngle), 0.0f, sin(randAngle)) * 0.5f;
    p.velocity = float3(0, 1.5f, 0) + horiz;
    
    // 初期サイズ
    p.size = lerp(0.3f, 0.5f, randB) * g_Scale;
    
    // 色（静的、必要ならfadeをPSで処理）
    p.startColor = g_startColor;
    
    // ライフ設定
    p.life = lerp(g_LifeMin, g_LifeMax, randA);
    p.lifeRemaining = p.life;
    
    // [0 ~ 2π] の回転角
    p.rotation = randAngle;

    // 書き戻し
    g_ParticlesUAV[slotID] = p;
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
    BillboardSimpleParticle p = g_ParticlesSRV[input[0].ParticleID];
    
    if (p.lifeRemaining <= 0.001f || p.life <= 0.001f)
        return;

    float3 pos = p.position;
    float size = p.size;

    // ビルボード展開（Viewの1列目と2列目から）
    float3 right = normalize(float3(g_View._11, g_View._21, g_View._31));
    float3 up = normalize(float3(g_View._12, g_View._22, g_View._32));
    
    // 回転角取得（パーティクル毎）
    float angle = p.rotation;
    float cosA = cos(angle);
    float sinA = sin(angle);
    
    // 回転後の軸ベクトル
    float3 rightRot = cosA * right + sinA * up;
    float3 upRot = -sinA * right + cosA * up;

    // 左上 → 右上 → 右下 → 左下 順に展開
    float3 corners[4] =
    {
        pos + (-rightRot + upRot) * size,
        pos + (rightRot + upRot) * size,
        pos + (rightRot - upRot) * size,
        pos + (-rightRot - upRot) * size
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
        2, 3, 0  // 第2三角形
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
    // ノイズテクスチャをサンプリング
    float noise = g_DiffuseTex.Sample(g_Sampler, input.TexCoord).r;

    // ノイズを強調して破れた感じを出す
    noise = saturate((noise - 0.4f) * 3.0f);

    float4 finalColor = input.Color;
    finalColor.a *= noise; // αにノイズ適用

    return finalColor;
}