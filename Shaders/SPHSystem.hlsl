// ============================================================================
//  BuildGridCS.hlsl & SPHSimulateCS.hlsl (統合ソース)
//  Author :
// ----------------------------------------------------------------------------
//  本ファイルは 2 つの Compute Shader を同一ファイルに収めています。
//  1) BuildGridCS   : パーティクルをユニフォームグリッドへ分類し、
//                     SortedIndices と GridStartIndex を構築します。
//  2) SPHSimulateCS : SPH の『密度→圧力→力計算→位置更新→地形衝突』
//                     さらに Alive / Free List の管理を 1 Dispatch で行います
// ============================================================================
//  * u4  : RWStructuredBuffer<uint>      SortedIndices            (BuildGrid)
//  * u5  : RWStructuredBuffer<uint>      GridStartIndex           (BuildGrid)
//  * u0  : RWStructuredBuffer<Particle>  Particles                (Simulate)
//  * u1  : AppendStructuredBuffer<uint>  AliveList                (Simulate)
//  * u2  : AppendStructuredBuffer<uint>  FreeList                 (Simulate)
//  * t6  : StructuredBuffer<Particle>    Particles (Read Only)    (Both)
//  * t12 : StructuredBuffer<uint>        SortedIndices            (Simulate)
//  * t13 : StructuredBuffer<uint>        GridStartIndex           (Simulate)
// ============================================================================
#include "VoxelCollisionUtil.hlsl"
#include "SharedParticleUtil.hlsl"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define SLOT_CB_EFFECT_SPH                  b8
#define SLOT_UAV_SORTED_INDICES             u4
#define SLOT_UAV_GRID_START_INDEX           u5
#define SLOT_UAV_GRID_COUNTER               u6
#define SLOT_SRV_SORTED_INDICES             t12
#define SLOT_SRV_GRID_START_INDEX           t13
#define SLOT_SRV_GRID_COUNTER               t14
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
    float pressure; // 圧力（密度から算出される）
    
    float3 predictedPosition; // 予測位置（次の位置、拘束前）
    float lifeRemaining; // 寿命（将来拡張用、今は1で固定でも可）

    float4 color; // デバッグ用カラー（密度に応じた可視化など）
    
    float size; // サイズ
    
    uint debug_mortonIndex;
    uint debug_voxelX;
    uint debug_voxelY;
    uint debug_voxelZ;
    int debug_foundTripletIndex;
    uint debug_triCheckCount;
    uint debug_hit;
    uint debug_pageHeaderIndex;
    float3 debug_position;
    float debug_dirLength;
    float3 debug_triV0;
    float3 debug_triV1;
    float3 debug_triV2;
    float3 debug_rayStart;
    float3 debug_rayDir;
    float3 debug_hitPoint;
    float3 debug_pressureForce;
    float3 debug_viscosityForce;
    float3 debug_gridCoord;
    
    float3 padding;
};

// SPH 計算用のパラメータ構造体
struct SPHSimParams
{
    float smoothingRadius; // h (SPH 平滑半径)
    float particleMass; // 質量 m
    float restDensity; // 目標密度 ρ0
    float pressureMultiplier; // 圧力係数 k (圧力 = k * (密度 - ρ0))
    
    float viscosity; // 粘性係数
    float3 acceleration; // 外力 (重力など)
    
    uint3 gridDim; // グリッド分割数 (x,y,z)
    uint gridCellStart; // グリッドセルの開始インデックス（SortedIndices のオフセット）
    
    float3 gridMin; // ワールド空間でのグリッド最小座標
    uint maxParticleCount; // 総粒子数
    
    float cellSize; // 1 セルの大きさ (= h 推奨)
    uint maxParticlesPerCell; // 1 セルあたりの最大パーティクル数
    
    float deltaTime; // タイムステップ (Δt)
    float padding; // パディング（アライメント用）
};

//*****************************************************************************
// 定数バッファ
//*****************************************************************************

cbuffer CBSPHParams : register(SLOT_CB_EFFECT_SPH)
{
    SPHSimParams SPHParams;
};


// バッファ
RWStructuredBuffer<WaterFluidParticle> g_ParticlesUAV   : register(SLOT_UAV_PARTICLE); // 書き込み用パーティクルデータバッファ
RWStructuredBuffer<uint> g_SortedIndicesUAV             : register(SLOT_UAV_SORTED_INDICES); // 書き込み用ソート済みインデックス
StructuredBuffer<uint> g_SortedIndicesSRV               : register(SLOT_SRV_SORTED_INDICES); // 読み取り用ソート済みインデックス
RWStructuredBuffer<uint> g_GridStartIndexUAV            : register(SLOT_UAV_GRID_START_INDEX); // 書き込み用グリッド開始位置
StructuredBuffer<uint> g_GridStartIndexSRV              : register(SLOT_SRV_GRID_START_INDEX); // 読み取り用グリッド開始位置
RWStructuredBuffer<uint> g_GridCounterUAV               : register(SLOT_UAV_GRID_COUNTER); // 各グリッドセル内のパーティクル数カウンタ（原子加算用）
StructuredBuffer<uint> g_GridCounterSRV                 : register(SLOT_SRV_GRID_COUNTER); // 読み取り用グリッドカウンタ

//================== 補助関数 ==================
// Poly6 / Spiky / Viscosity カーネル
float W_poly6(float r, float h)
{
    float x = h * h - r * r;
    return (315.0 / (64.0 * 3.141592 * pow(abs(h), 9))) * pow(x, 3);
}

float W_spiky_grad(float r, float h)
{
    return (15.0 / (3.141592 * pow(h, 6))) * pow(h - r, 2);
}

float W_visc_laplacian(float r, float h)
{
    return (45.0 / (3.141592 * pow(h, 6))) * (h - r);
}

// 座標がグリッド内かどうかを判定
bool IsCoordInBounds(uint3 coord, uint3 gridDim)
{
    return (coord.x < gridDim.x) &&
           (coord.y < gridDim.y) &&
           (coord.z < gridDim.z);
}

// グリッド座標を 1次元インデックスに変換
uint Flatten3DTo1D(uint3 coord, uint3 gridDim)
{
    return coord.x + coord.y * gridDim.x + coord.z * gridDim.x * gridDim.y;
}

// ------------------------------
// BuildGridCS
// ------------------------------
// グリッドセル内カウンタを確保するため、GridStartIndex は毎フレーム Ping-Pong
// GridStartIndex 初期値は 0xFFFFFFFF (未設定)。
// -----------------------------------------------------------------------------
[numthreads(THREAD_COUNT, 1, 1)]
void BuildGirdCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_MaxParticleCount)
        return;

    // 読み込み
    WaterFluidParticle p = g_ParticlesUAV[id];

    // ワールド座標をグリッド座標へ変換
    float3 rel = p.position - SPHParams.gridMin;
    uint3 gridCoord = (uint3) floor(rel / SPHParams.cellSize);
    uint flat = Flatten3DTo1D(gridCoord, SPHParams.gridDim);

    // 各セルにおける書き込みインデックスを取得（原子加算）
    uint localOffset;
    InterlockedAdd(g_GridCounterUAV[flat], 1, localOffset);
    
    // Global list の中でこの粒子が書き込む場所を計算（スタート + オフセット）
    uint globalIndex = SPHParams.gridCellStart + flat * SPHParams.maxParticlesPerCell + localOffset;
    g_SortedIndicesUAV[globalIndex] = id;

    // GridStartIndex を一度だけ設定（CompareExchange）
    uint expected = 0xFFFFFFFF;
    InterlockedCompareExchange(g_GridStartIndexUAV[flat], globalIndex, expected, expected); // start = 最初の粒子位置

    // インデックスを書き込み
    g_SortedIndicesUAV[globalIndex] = id;
}

// ------------------------------
// SPHSimulateCS
// ------------------------------
// SPH計算と位置更新をまとめて行う
// -------------------------------
[numthreads(THREAD_COUNT, 1, 1)]
void SimulateCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint id = dispatchThreadID.x;
    if (id >= g_MaxParticleCount)
        return;

    WaterFluidParticle p = g_ParticlesUAV[id];
    
    // ==== 生存している場合：更新処理 ====
    if (p.lifeRemaining > 0.0f)
    {
        // ------------------
        // Step 1: 密度 & 圧力計算
        // ------------------
        float density = 0.0f;

        // 所属グリッド座標
        float3 rel = p.position - SPHParams.gridMin;
        uint3 gridCoord = (uint3) floor(rel / SPHParams.cellSize);
        
        p.debug_gridCoord = gridCoord;

        // 3x3x3 の近傍セルを走査
        [allow_uav_condition]
        for (int z = -1; z <= 1; ++z)
        {
            [allow_uav_condition]
            for (int y = -1; y <= 1; ++y)
            {
                [allow_uav_condition]
                for (int x = -1; x <= 1; ++x)
                {
                    uint3 neighborCoord = gridCoord + uint3(x, y, z);
                    if (!IsCoordInBounds(neighborCoord, SPHParams.gridDim))
                        continue;
                    
                    uint flat = Flatten3DTo1D(neighborCoord, SPHParams.gridDim);
                    uint startIndex = g_GridStartIndexSRV[flat];
                    if (startIndex == 0xFFFFFFFF)
                        continue;
                    
                    uint count = g_GridCounterSRV[flat];
                    [allow_uav_condition]
                    for (uint i = 0; i < count; ++i)
                    {
                        if (startIndex + i >= SPHParams.maxParticleCount)
                            break;
                        
                        uint neighborID = g_SortedIndicesSRV[startIndex + i];
                        WaterFluidParticle q = g_ParticlesUAV[neighborID];
                        
                        float3 r = p.position - q.position;
                        float dist = length(r);
                        if (dist < SPHParams.smoothingRadius && dist > 0.0001f)
                        {
                            float W = W_poly6(dist, SPHParams.smoothingRadius);
                            density += SPHParams.particleMass * W;
                        }
                    }
                }
            }
        }
    
        p.density = density;
        p.pressure = SPHParams.pressureMultiplier * max(density - SPHParams.restDensity, 0.0);

        // ------------------
        // Step 2: 力計算 (圧力 + 粘性)
        // ------------------
        float3 pressureForce = float3(0, 0, 0);
        float3 viscosityForce = float3(0, 0, 0);

        // 近傍セルを再度走査して圧力と粘性の力を計算
        [allow_uav_condition]
        for (int zz = -1; zz <= 1; ++zz)
        {
            [allow_uav_condition]
            for (int yy = -1; yy <= 1; ++yy)
            {
                [allow_uav_condition]
                for (int xx = -1; xx <= 1; ++xx)
                {
                    uint3 neighborCoord = gridCoord + uint3(xx, yy, zz);
                    if (!IsCoordInBounds(neighborCoord, SPHParams.gridDim))
                        continue;
                    uint flat = Flatten3DTo1D(neighborCoord, SPHParams.gridDim);
                    uint startIndex = g_GridStartIndexSRV[flat];
                    if (startIndex == 0xFFFFFFFF)
                        continue;
                    uint count = g_GridCounterSRV[flat];
                    
                    [allow_uav_condition]
                    for (uint i = 0; i < count; ++i)
                    {
                        if (startIndex + i >= SPHParams.maxParticleCount)
                            break;
                        uint neighborID = g_SortedIndicesSRV[startIndex + i];
                        WaterFluidParticle q = g_ParticlesUAV[neighborID];
                        
                        float3 r = p.position - q.position;
                        float dist = length(r);
                        if (dist < SPHParams.smoothingRadius && dist > 0.0001f)
                        {
                            float3 dir = normalize(r);
                            
                            // 圧力項
                            float W_spiky = W_spiky_grad(dist, SPHParams.smoothingRadius);
                            float avgPressure = (p.pressure + SPHParams.pressureMultiplier * (q.density - SPHParams.restDensity)) / 2.0f;
                            pressureForce -= dir * avgPressure * W_spiky * SPHParams.particleMass / q.density;

                            // 粘性項
                            float3 velDiff = q.velocity - p.velocity;
                            float viscosity = SPHParams.viscosity;
                            float W_visc = W_visc_laplacian(dist, SPHParams.smoothingRadius);
                            viscosityForce += viscosity * velDiff * W_visc * SPHParams.particleMass / q.density;
                        }
                    }
                }
            }
        }
        
        p.debug_pressureForce = pressureForce;
        p.debug_viscosityForce = viscosityForce;

        // ------------------
        // Step 3: 位置 & 速度更新
        // ------------------
        float3 gravity = g_Acceleration;
        //float3 accel = (pressureForce + viscosityForce) / p.density + gravity;
        float3 accel = gravity;
        p.velocity += accel * g_DeltaTime;
        float3 deltaPos = p.velocity * g_DeltaTime;
        p.predictedPosition = p.position + deltaPos;

        // ------------------
        // Step 4: 地形衝突（Voxel Grid）
        // ------------------
        float4 worldPredicted = mul(float4(p.predictedPosition, 1.0), g_World);
        uint3 voxelCoord = GetVoxelCoord(worldPredicted.xyz);
        uint mortonIndex = EncodeMorton3(voxelCoord.x, voxelCoord.y, voxelCoord.z);
        
        int found = FindIndexTriplet(mortonIndex);
        if (found >= 0)
        {
            IndexTriplet trip = g_IndexTriplets[found];
            float3 bestHit = float3(0, 0, 0);
            float3 bestNormal = float3(0, 0, 0);
            float closestRayDistance = 1e20;
            float4 rayStartWS = mul(float4(p.position, 1.0), g_World);
            float4 rayDirWS = mul(float4(deltaPos, 0.0), g_World);
            float3 rayDir = normalize(rayDirWS.xyz);
            float rayRange = length(rayDirWS.xyz);
            float3 rayStart = rayStartWS.xyz + rayDir * 0.001f;

            [allow_uav_condition]
            for (uint i = 0; i < trip.count; ++i)
            {
                TriangleStruct tri = g_Triangles[g_VoxelTriangleIndices[trip.startOffset + i]];
                float3 hitPoint = float3(0, 0, 0);
                float rayDist = 0;
                if (RayIntersectsTriangle(rayStart, rayDir, tri, hitPoint, rayDist))
                {
                    if (rayDist <= rayRange && rayDist < closestRayDistance)
                    {
                        closestRayDistance = rayDist;
                        bestHit = hitPoint;
                        bestNormal = tri.normal; // 命中した三角形の法線を保存
                    }
                }
            }
            if (closestRayDistance < 1e19)
            {
                // 位置修正 + 速度投影 (滑り)
                float3 terrainNormal = normalize(bestNormal);
                //p.velocity = 0;
                p.color = float4(1, 0, 0, 1); // 衝突時は赤くする
                float4 localHit = mul(float4(bestHit, 1.0), g_WorldInv);
                p.predictedPosition = localHit.xyz + terrainNormal * 0.01f; // 地面より少しだけ押し戻す
                p.velocity = ApplyCollisionSlideVelocity(p.velocity, terrainNormal, 0.99f, 0.01f);
                p.debug_hit = 1;
                p.debug_hitPoint = bestHit;
                
            }
        }
        
        p.position = p.predictedPosition;

        // ------------------
        // Step 5: 書き戻し & Alive/Free List 更新
        // ------------------
        g_ParticlesUAV[id] = p;
        g_AliveListUAV.Append(id);
    
    }
    else
    {
        // 死亡した場合は FreeList に登録
        g_FreeListUAV.Append(id);
    }
}