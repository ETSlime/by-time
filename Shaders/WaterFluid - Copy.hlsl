//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_EFFECT_UPDATE               b5
#define SLOT_CB_EFFECT_DRAW                 b6
#define SLOT_CB_EFFECT_PARTICLE             b7
#define SLOT_CB_EFFECT_EMIT                 b10
#define SLOT_CB_VOXEL_META                  b13
#define SLOT_UAV_PARTICLE                   u0
#define SLOT_UAV_ALIVE_LIST                 u1
#define SLOT_UAV_FREE_LIST                  u2
#define SLOT_UAV_FREE_LIST_CONSUME          u3
#define SLOT_SRV_PARTICLE                   t6
#define SLOT_SRV_ALIVE_LIST                 t7
#define SLOT_SRV_VOXEL_TRIANGLE_INDEX       t8
#define SLOT_SRV_VOXEL_TRIANGLE_STRUCT      t9
#define SLOT_SRV_VOXEL_IDX_TRIPLET          t10
#define SLOT_SRV_VOXEL_IDX_TRIPLET_PAGE     t11
#define SLOT_TEX_DIFFUSE                    t0
#define SLOT_SAMPLER_DEFAULT                s0
#define SLOT_CB_VIEW_MATRIX                 b1

#define MAX_TRI_PER_VOXEL                   64  
#define MAX_INDEX_TRIPLET_PAGES             1024

//*****************************************************************************
// 構造体定義
//*****************************************************************************

// 粒子構造体
struct WaterFluidParticle
{
    float3 position; // 現在位置
    float density; // 密度（周囲の粒子から算出）

    float3 velocity; // 速度
    float lambda; // 拘束条件解（位置補正のための係数）

    float3 predictedPosition; // 予測位置（次の位置、拘束前）
    float lifeRemaining; // 寿命（将来拡張用、今は1で固定でも可）

    float4 color; // デバッグ用カラー（密度に応じた可視化など）
    
    float size; // サイズ
    float3 padding;
};

// 三角形構造体：頂点3つと法線
struct TriangleStruct
{
    float3 v0;
    float pad0;
    float3 v1;
    float pad1;
    float3 v2;
    float pad2;
    float3 normal;
    float pad3;
};

// IndexTriplet：voxel index → 三角形の開始オフセットと数
struct IndexTriplet
{
    uint voxelIndex;
    uint startOffset;
    uint count;
    uint pad; // パディング（cbuffer整列のため）
};

// IndexTripletPageHeader：IndexTripletのページヘッダー
struct IndexTripletPageHeader
{
    uint pageStartMorton; // ページ最小モートン番号
    uint pageEndMorton; // ページ最大モートン番号
    uint pageOffset; // g_IndexTripletsでのオフセット
    uint pageCount; // このページのトリプレット数
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

//*****************************************************************************
// 定数バッファ
//*****************************************************************************

// パーティクル更新用の定数バッファ
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

// パーティクル描画用の定数バッファ
cbuffer CBParticleDraw : register(SLOT_CB_EFFECT_DRAW)
{
    matrix g_World;
    matrix g_ViewProj;
};

// ビュー行列用の定数バッファ
cbuffer ViewBuffer : register(SLOT_CB_VIEW_MATRIX)
{
    matrix g_View;
}

// 水の流体パーティクル用の定数バッファ
cbuffer CBWaterFluid : register(SLOT_CB_EFFECT_PARTICLE)
{
    float g_SupportRadius;
    float g_RestDensity;
    float g_ParticleMass;
    float g_Stiffness;
    float g_Friction;
    float3 padding;
};

// ボクセルメタデータ用の定数バッファ
cbuffer CBVoxelMeta : register(SLOT_CB_VOXEL_META)
{
    float3 g_VoxelOrigin;           // ワールド空間での原点
    float g_VoxelSize;              // 各ボクセルの1辺の長さ

    uint g_TotalIndexTripletSize;   // g_IndexTripletsの総数
    uint g_TotalPageHeaderSize;     // g_PageHeadersの総数
    uint2 padVoxelMeta;             // パディングで16バイト揃え
};

// === ページヘッダーだけの定数バッファ ===
//cbuffer CB_IndexTripletPages : register(SLOT_CB_INDEX_TRIPLET_PAGE_HEADERS)
//{
//    IndexTripletPageHeader g_PageHeaders[MAX_INDEX_TRIPLET_PAGES];
//};

// パーティクルエミット用の定数バッファ
cbuffer CBEmit : register(SLOT_CB_EFFECT_EMIT)
{
    uint g_EmitCount;
    float3 g_EmitCenter;
    float g_EmitRadius;
    float3 _padding;
};

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
StructuredBuffer<uint> g_VoxelTriangleIndices : register(SLOT_SRV_VOXEL_TRIANGLE_INDEX); // 三角形インデックス
StructuredBuffer<TriangleStruct> g_Triangles : register(SLOT_SRV_VOXEL_TRIANGLE_STRUCT); // 三角形構造体
StructuredBuffer<IndexTriplet> g_IndexTriplets : register(SLOT_SRV_VOXEL_IDX_TRIPLET); // インデックス三つ組（モートンコード用）
StructuredBuffer<IndexTripletPageHeader> g_PageHeaders : register(SLOT_SRV_VOXEL_IDX_TRIPLET_PAGE); // ページヘッダー

//================== 補助関数 ==================
float Rand(float seed)
{
    return frac(sin(seed * 12.9898) * 43758.5453);
}

uint3 GetVoxelCoord(float3 worldPos)
{
    float3 local = (worldPos - g_VoxelOrigin) / g_VoxelSize;
    return uint3(local);
}

uint Part1By2(uint x)
{
    x &= 0x3ff;
    x = (x | (x << 16)) & 0x30000ff;
    x = (x | (x << 8))  & 0x300f00f;
    x = (x | (x << 4))  & 0x30c30c3;
    x = (x | (x << 2))  & 0x9249249;
    return x;
}

uint EncodeMorton3(uint x, uint y, uint z)
{
    return (Part1By2(z) << 2) | (Part1By2(y) << 1) | Part1By2(x);
}

// 二分探索で voxelIndex に一致する IndexTriplet を探す
int FindIndexTriplet(uint voxelIndex)
{
    uint left = 0;
    uint right = g_TotalPageHeaderSize - 1;

    // ページヘッダーの二分探索
    [allow_uav_condition]
    while (left <= right)
    {
        uint mid = (left + right) / 2;
        IndexTripletPageHeader pageHeader = g_PageHeaders[mid];
        
        if (voxelIndex < pageHeader.pageStartMorton)
        {
            right = mid - 1;
        }
        else if (voxelIndex > pageHeader.pageEndMorton)
        {
            left = mid + 1;
        }
        else
        {
            // ページの範囲に含まれているから、内部を探索する
            [allow_uav_condition]
            for (uint j = 0; j < pageHeader.pageCount; ++j)
            {
                IndexTriplet triplet = g_IndexTriplets[pageHeader.pageOffset + j];
                if (triplet.voxelIndex == voxelIndex)
                    return pageHeader.pageOffset + j;
            }

            // でも実際には見つからなかったら…？
            // 範囲は合っていたけど、ページに含まれていないパターン！
            // **探索は続けるべきじゃなくて、これはありえない状態かも！**
            return -2; // 一応保険
        }
    }

    return -1; // 完全に見つからない
}

//================== コリジョンチェック ==================
bool RayIntersectsTriangle(float3 origin, float3 dir, TriangleStruct tri, out float3 hitPoint)
{
    float3 edge1 = tri.v1 - tri.v0;
    float3 edge2 = tri.v2 - tri.v0;
    float3 h = cross(dir, edge2);
    float a = dot(edge1, h);

    if (abs(a) < 1e-6f)
        return false;

    float f = 1.0f / a;
    float3 s = origin - tri.v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    float3 q = cross(s, edge1);
    float v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    float t = f * dot(edge2, q);
    if (t > 1e-6f)
    {
        hitPoint = origin + t * dir;
        return true;
    }
    return false;
}

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
        // --- 予測位置更新（単純な速度積分）
        float3 acc = g_Acceleration;
        p.velocity += acc * g_DeltaTime; // 重力加速度 × Δt
        float3 predicted = p.position + p.velocity * g_DeltaTime;
        
        // Voxel 座標 → Morton Index
        uint3 voxelCoord = GetVoxelCoord(predicted);
        uint mortonIndex = EncodeMorton3(voxelCoord.x, voxelCoord.y, voxelCoord.z);
        
        int found = FindIndexTriplet(mortonIndex);
        
        if (found >= 0)
        {
            IndexTriplet t = g_IndexTriplets[found];
            
            [allow_uav_condition] // ループ展開のためのアトミック制御
            for (uint i = 0; i < t.count; ++i)
            {   
                uint triIndex = g_VoxelTriangleIndices[t.startOffset + i];
                TriangleStruct tri = g_Triangles[triIndex];

                float3 hitPoint;
                float3 dir = predicted - p.position;

                if (RayIntersectsTriangle(p.position, dir, tri, hitPoint))
                {
                    // --- 簡易的な反射処理っ！（滑る感じで跳ね返す）
                    float3 n = normalize(tri.normal);
                    float3 reflected = reflect(p.velocity, n);
                    p.velocity = reflected * 0.4f;
                    predicted = hitPoint + n * 0.01f;
                    break;
                }
            }
        }

        p.position += predicted;
        p.predictedPosition = predicted;

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