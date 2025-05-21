//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_EFFECT_UPDATE       b5
#define SLOT_CB_EFFECT_DRAW         b6
#define SLOT_CB_EFFECT_PARTICLE     b7
#define SLOT_UAV_PARTICLE           u0
#define SLOT_UAV_ALIVE_LIST         u1
#define SLOT_UAV_FREE_LIST          u2
#define SLOT_UAV_FREE_LIST_CONSUME  u3
#define SLOT_SRV_PARTICLE           t6
#define SLOT_SRV_ALIVE_LIST         t7
#define SLOT_TEX_DIFFUSE            t0
#define SLOT_SAMPLER_DEFAULT        s0
#define SLOT_CB_VIEW_MATRIX         b1
static const uint TILE_X = 7;
static const uint TILE_Y = 7;
static const uint TOTAL_FRAME = TILE_X * TILE_Y;
//*****************************************************************************
// 定数バッファ
//*****************************************************************************
cbuffer CBParticleUpdate : register(SLOT_CB_EFFECT_UPDATE)
{
    float3 g_Acceleration;
    float g_Scale; // サイズスケール
    
    float g_DeltaTime; // 経過時間
    float g_TotalTime; // 全体経過時間
    float g_LifeMin; // 最小ライフ
    float g_LifeMax; // 最大ライフ
    float g_SpawnRateMin; // 毎秒発射数
    float g_SpawnRateMax; // 毎秒発射数
    
    uint g_MaxParticleCount; // 最大パーティクル数
    uint g_ParticlesToEmitThisFrame; // このフレームに発射するパーティクル数
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

cbuffer CBFireBall : register(SLOT_CB_EFFECT_PARTICLE)
{
    uint g_TilesX;
    uint g_TilesY;
    float g_InitialFrameOffset;
    float g_FrameLerpCurve;

    float g_RotationSpeed;
    float g_ConeAngleDegree;
    float g_ConeRadius;
    float g_ConeLength;
};


// 粒子構造体
struct FireBallParticle
{
    float3 position;
    float lifeRemaining;
    float life;
    float3 velocity;
    float rotation;
    float size;
    float frameIndex;
    float frameSpeed;
    float4 color;
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
    float2 TexCoordA : TEXCOORD0;
    float2 TexCoordB : TEXCOORD1;
    float Blend : TEXCOORD2;
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
RWStructuredBuffer<FireBallParticle> g_ParticlesUAV : register(SLOT_UAV_PARTICLE); // 書き込み用パーティクルデータバッファ
StructuredBuffer<FireBallParticle> g_ParticlesSRV : register(SLOT_SRV_PARTICLE); // 読み取り用パーティクルデータバッファ
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

    FireBallParticle p = g_ParticlesUAV[id];
    
    // ==== 生存している場合：更新処理 ====
    if (p.lifeRemaining > 0.0f)
    {
        // 残り寿命を減少
        p.lifeRemaining -= g_DeltaTime;

        // 速度更新
        p.velocity += g_Acceleration * g_DeltaTime;

        // 位置更新
        p.position += p.velocity * g_DeltaTime;
        
        // 回転
        p.rotation += g_RotationSpeed * g_DeltaTime;

        // アニメーション更新（非線形曲線あり）
        float lifeRatio = saturate(1.0f - p.lifeRemaining / p.life);
        float animProgress = pow(lifeRatio, g_FrameLerpCurve);
        p.frameIndex = animProgress * (float) (g_TilesX * g_TilesY - 1);
        
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
    uint seed = slotID * 1664525 + 1013904223 + asuint(g_TotalTime * 1000.0f);
    float randA = frac(sin(seed * 12.9898) * 43758.5453);
    float randB = frac(sin(seed * 78.233) * 24634.6345);
    float randC = frac(sin(seed * 11.31) * 19493.2242);
    float randD = frac(sin(seed * 91.123) * 51932.7753);
    
    // コーン角度をラジアン変換して方向を生成
    float angleRad = radians(g_ConeAngleDegree);
    float theta = randA * 6.2831f;
    float y = cos(randB * angleRad);
    float r = sqrt(1.0 - y * y);
    float3 dir = normalize(float3(r * cos(theta), y, r * sin(theta)));
    
    // 発射位置オフセット（コーンの長さに基づく）
    float3 position = dir * (randC * g_ConeLength);

    // パーティクルを再生成する処理（位置・速度・サイズなど初期化）
    FireBallParticle p = (FireBallParticle) 0; // 明示的に全フィールドを初期化
    p.position = position;
    p.velocity = dir * 2.0f;
    
    // ライフ設定
    float life = lerp(g_LifeMin, g_LifeMax, randD);
    p.life = life;
    p.lifeRemaining = life;
    
    // 回転初期値（0～2π）
    p.rotation = randB * 6.2831f;
    
    // 初期サイズ
    p.size = lerp(2.0f, 3.0f, randC) * g_Scale;

    // アニメーション初期フレーム
    p.frameIndex = 0.0f;
    p.frameSpeed = (float) (g_TilesX * g_TilesY) / life;

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
    FireBallParticle p = g_ParticlesSRV[input[0].ParticleID];
    
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
    
    // アニメーション用UVの基礎UV（左上~右下）
    float2 uvBase[4] =
    {
        float2(0.0f, 0.0f),
        float2(1.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(0.0f, 1.0f)
    };
    
    // フレーム番号と補間係数（flipbook blending用）
    float totalFrames = TILE_X * TILE_Y;
    float curIndex = p.frameIndex;
    int frameA = (int) floor(curIndex);
    int frameB = min(frameA + 1, (int) totalFrames - 1);
    float blend = saturate(curIndex - frameA);

    int2 tileA = int2(frameA % TILE_X, frameA / TILE_X);
    int2 tileB = int2(frameB % TILE_X, frameB / TILE_X);

    // UVスケール
    float2 tileSize = 1.0f / float2(TILE_X, TILE_Y);
    
     // 寿命比率 
    float lifeRatio = saturate(p.lifeRemaining / p.life);
    
    // 簡易グラデーション（黄 → 橙 → 消滅）
    float3 col = lerp(float3(1.0, 1.0, 0.3), float3(1.2, 0.5, 0.0), 1.0 - lifeRatio);
    float alpha = pow(lifeRatio, 1.2); // フェードアウト

    float4 finalColor = float4(col, alpha);

    int triangleIndices[6] =
    {
        0, 1, 2, // 第1三角形
        2, 3, 0  // 第2三角形
    };
    
    for (int i = 0; i < 6; ++i)
    {
        int idx = triangleIndices[i];
        float3 worldPos = corners[idx];
        float4 posWVP = mul(float4(worldPos, 1.0f), g_ViewProj);

        
        GS_OUTPUT o;
        o.PosH = posWVP;
        
        // 現在フレームと次フレームのUV（ピクセルシェーダー側で補間する）
        o.TexCoordA = tileA * tileSize + uvBase[idx] * tileSize;
        o.TexCoordB = tileB * tileSize + uvBase[idx] * tileSize;
        o.Blend = blend;
        o.Color = finalColor;
        
        stream.Append(o);

        // 3個ごとにRestartStrip()
        if ((i + 1) % 3 == 0)
        {
            stream.RestartStrip();
        }
    }
}

//=============================================================================
// ピクセルシェーダ
//=============================================================================
float4 PS(GS_OUTPUT input) : SV_Target
{
    float4 colA = g_DiffuseTex.Sample(g_Sampler, input.TexCoordA);
    float4 colB = g_DiffuseTex.Sample(g_Sampler, input.TexCoordB);
    float4 texCol = lerp(colA, colB, input.Blend);

    texCol *= input.Color;

    return texCol;
}