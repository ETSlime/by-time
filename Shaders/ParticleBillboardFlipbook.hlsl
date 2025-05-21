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

cbuffer CBFireBall : register(SLOT_CB_EFFECT_PARTICLE)
{
    uint g_TilesX;
    uint g_TilesY;
    float g_InitialFrameOffset;
    float g_FrameLerpCurve;

    float g_RotationSpeed; 
    float g_ConeAngleDegree;  // コーンの角度
    float g_ConeRadius; // コーンの半径
    float g_ConeLength; // コーンの高さ
    
    float g_StartSpeedMin;
    float g_StartSpeedMax;
    float2 padding;
};


// 粒子構造体
struct BillboardFlipbookParticle
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
    float frameIndex;
    float frameSpeed;
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
    float2 TexCoord : TEXCOORD0;
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
RWStructuredBuffer<BillboardFlipbookParticle> g_ParticlesUAV : register(SLOT_UAV_PARTICLE); // 書き込み用パーティクルデータバッファ
StructuredBuffer<BillboardFlipbookParticle> g_ParticlesSRV : register(SLOT_SRV_PARTICLE); // 読み取り用パーティクルデータバッファ
StructuredBuffer<uint> g_AliveListSRV : register(SLOT_SRV_ALIVE_LIST); // 読み取り用生存パーティクルリスト
ConsumeStructuredBuffer<uint> g_FreeListConsumeUAV : register(SLOT_UAV_FREE_LIST_CONSUME); // 読み取り用空きパーティクルリスト

float Hash11(float x)
{
    return frac(sin(x * 12.9898f) * 43758.5453f);
}

//=============================================================================
// コンピュートシェーダ
//=============================================================================
[numthreads(64, 1, 1)]
void UpdateCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_MaxParticleCount)
        return; // 範囲外は無視

    BillboardFlipbookParticle p = g_ParticlesUAV[id];
    
    // ==== 生存している場合：更新処理 ====
    if (p.lifeRemaining > 0.0f)
    {
        // 残り寿命を減少
        p.lifeRemaining -= g_DeltaTime;
        float lifeRatio = saturate(p.lifeRemaining / max(p.life, 0.0001f)); // 生存率
        
        // 移動処理
        p.position += p.velocity * g_DeltaTime; // 位置更新
        
        // 回転
        p.rotation += g_RotationSpeed * g_DeltaTime;

        // アニメーション更新（非線形曲線あり）
        float animProgress = pow(1 - lifeRatio, g_FrameLerpCurve);
        p.frameIndex = animProgress * (float) (g_TilesX * g_TilesY - 1);
                
        // RGBA 全体のグラデーション
        float4 baseColor = lerp(g_endColor, g_startColor, 1.0 - lifeRatio);

        // Alpha を明度スケールとしても活用（Additive対策）
        // ここで明るさを調整する共通倍率
        float brightnessScale = baseColor.a * 0.6f;

        // 明るさ抑制後のRGB
        float3 finalColor = baseColor.rgb * brightnessScale;

        // 最終カラー（RGBは抑制済み、AはBlendに任せる）
        p.color = float4(finalColor, baseColor.a);
        
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
    float randA = Hash11((float) id + g_TotalTime * 1.0f);
    float randB = Hash11((float) id + g_TotalTime * 3.14f);
    float randC = Hash11((float) id + g_TotalTime * 2.71f);
    float randD = Hash11((float) id + g_TotalTime * 0.577f);
    
    /// コーン角度をラジアン変換して方向を生成
    float angleRad = radians(g_ConeAngleDegree);
    
    // コーン内方向の cosθ の範囲 [cos(角度), 1.0] でランダムに分布
    float cosTheta = lerp(cos(angleRad), 1.0f, randB);
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
    float phi = randA * 6.2831f;
    // 発射方向（Y軸方向のコーン）
    float3 coneDir = float3(
    sinTheta * cos(phi),
    cosTheta,
    sinTheta * sin(phi));
    
    // 発射位置（Cone内部に沿った方向で長さに応じた距離）
    float3 position = coneDir * (randC * g_ConeLength); // 0~ConeLengthの間でランダム

    // パーティクルを再生成する処理（位置・速度・サイズなど初期化）
    BillboardFlipbookParticle p = (BillboardFlipbookParticle) 0; // 明示的に全フィールドを初期化
    p.position = position;
    p.velocity = coneDir * lerp(g_StartSpeedMin, g_StartSpeedMax, randC) * g_Scale * 0.5f; // 発射速度
    
    // ライフ設定
    float life = lerp(g_LifeMin, g_LifeMax, randD);
    p.life = life;
    p.lifeRemaining = life;
    
    // 回転初期値（0～2π）
    p.rotation = randB * 6.2831f;
    
    // 初期サイズ
    p.size = lerp(2.0f, 3.0f, randC) * g_Scale;
    
    // 色
    p.startColor = g_startColor;
    p.endColor = g_endColor;

    // アニメーション初期フレーム
    p.frameIndex = 0.0f; // アニメーション開始フレーム
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
    BillboardFlipbookParticle p = g_ParticlesSRV[input[0].ParticleID];
    
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
    float totalFrames = g_TilesX * g_TilesY;
    float curIndex = p.frameIndex;
    int frameA = (int) floor(curIndex);
    int frameB = min(frameA + 1, (int) totalFrames - 1);
    float blend = saturate(curIndex - frameA);

    int2 tileA = int2(frameA % g_TilesX, frameA / g_TilesX);
    int2 tileB = int2(frameB % g_TilesX, frameB / g_TilesX);

    // UVスケール
    float2 tileSize = 1.0f / float2(g_TilesX, g_TilesY);
    
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
        float3 worldPos = mul(float4(corners[idx], 1.0f), g_World);
        float4 posWVP = mul(float4(worldPos, 1.0f), g_ViewProj);

        
        GS_OUTPUT o;
        o.PosH = posWVP;
        
        // 現在フレームと次フレームのUV（ピクセルシェーダー側で補間する）
        o.TexCoordA = tileA * tileSize + uvBase[idx] * tileSize;
        o.TexCoordB = tileB * tileSize + uvBase[idx] * tileSize;
        //o.TexCoordA = texcoords[idx];
        o.Blend = blend;
        o.Color = p.color;
        
        stream.Append(o);

        // 3個ごとにRestartStrip()
        if ((i + 1) % 3 == 0)
        {
            stream.RestartStrip();
        }
    }
    
    
    //BillboardFlipbookParticle p = g_ParticlesSRV[input[0].ParticleID];
    
    //if (p.lifeRemaining <= 0.001f || p.life <= 0.001f)
    //    return;

    //float3 pos = p.position;
    //float size = p.size;

    //// ビルボード展開（Viewの1列目と2列目から）
    //float3 right = normalize(float3(g_View._11, g_View._21, g_View._31));
    //float3 up = normalize(float3(g_View._12, g_View._22, g_View._32));
    
    //// 回転角取得（パーティクル毎）
    //float angle = p.rotation;
    //float cosA = cos(angle);
    //float sinA = sin(angle);
    
    //// 回転後の軸ベクトル
    //float3 rightRot = cosA * right + sinA * up;
    //float3 upRot = -sinA * right + cosA * up;

    //// 左上 → 右上 → 右下 → 左下 順に展開
    //float3 corners[4] =
    //{
    //    pos + (-rightRot + upRot) * size,
    //    pos + (rightRot + upRot) * size,
    //    pos + (rightRot - upRot) * size,
    //    pos + (-rightRot - upRot) * size
    //};

    //float2 texcoords[4] =
    //{
    //    float2(0.0f, 0.0f),
    //    float2(1.0f, 0.0f),
    //    float2(1.0f, 1.0f),
    //    float2(0.0f, 1.0f)
    //};

    //int triangleIndices[6] =
    //{
    //    0, 1, 2, // 第1三角形
    //    2, 3, 0 // 第2三角形
    //};
    
    //for (int i = 0; i < 6; ++i)
    //{
    //    int idx = triangleIndices[i];
    //    GS_OUTPUT o = (GS_OUTPUT) 0;
    //    float4 worldPos = mul(float4(corners[idx], 1.0f), g_World);
    //    o.PosH = mul(worldPos, g_ViewProj);
    //    o.TexCoordA = texcoords[idx];
    //    o.Color = p.color;
    //    stream.Append(o);

    //    // 3個ごとにRestartStrip()
    //    if ((i + 1) % 3 == 0)
    //    {
    //        stream.RestartStrip();
    //    }
    //}
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
    
        // ノイズテクスチャをサンプリング
    //float noise = g_DiffuseTex.Sample(g_Sampler, input.TexCoordA).r;

    //// ノイズを強調して破れた感じを出す
    //noise = saturate((noise - 0.4f) * 3.0f);

    //float4 finalColor = input.Color;
    //finalColor.a *= noise; // αにノイズ適用
    
    //return finalColor;
}