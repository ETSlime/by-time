//=============================================================================
//
// 空間加速構造体ちゃん [VoxelGrid.cpp]
// Author : 
// ボクセルベースの空間分割によって、三角形メッシュを3Dグリッドに整理し、
// 高速なクエリ（衝突判定・視界探索・地形サンプリングなど）を可能にする
//
//=============================================================================
#include "VoxelGrid.h"

VoxelGrid::~VoxelGrid()
{
	m_voxelMap.clear(); // ボクセルマップをクリア
	m_triangles.clear(); // 登録された三角形をクリア
}

void VoxelGrid::Initialize(float voxelSize)
{
	// ボクセルサイズの設定
	m_voxelSize = voxelSize;
    // ボクセルマップの初期化
	m_voxelMap.clear();
    // 登録された三角形のリストの初期化
	m_triangles.clear();
	m_triangles.reserve(MAX_VOXEL_TRIANGLE_NUM); // 初期容量を設定（必要に応じて調整）
	// グリッドの原点を初期化
    m_voxelBounds.minPoint = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_voxelBounds.maxPoint = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

void VoxelGrid::BuildTriangles(const SimpleArray<Triangle*>& triangles)
{
    UINT count = triangles.getSize();
    for (UINT i = 0; i < count; ++i)
    {
        // 実質Insertの高速ループ
		InsertTriangle(triangles[i]);
    }
}

void VoxelGrid::InsertTriangle(const Triangle* tri)
{
    if (m_threadSafe)
    {
        ScopedSpinLock guard(m_lock); // スピンロックで排他制御
        m_triangles.push_back(tri); // 三角形を追加

		// ボクセルの境界を更新
        ComputeVoxelBounds(tri);
    }
    else
    {
        m_triangles.push_back(tri); // 三角形を追加

		// ボクセルの境界を更新
        ComputeVoxelBounds(tri);
    }
}

void VoxelGrid::BuildVoxelMapByTriangles(void)
{
	if (m_isReady) return; // すでにビルド済みなら何もしない

    // グリッドの原点を決定（負数を避けて切り下げ）
    m_origin.x = floor(m_voxelBounds.minPoint.x / m_voxelSize) * m_voxelSize;
    m_origin.y = floor(m_voxelBounds.minPoint.y / m_voxelSize) * m_voxelSize;
    m_origin.z = floor(m_voxelBounds.minPoint.z / m_voxelSize) * m_voxelSize;

    UINT count = m_triangles.getSize();
    for (UINT i = 0; i < count; ++i)
    {
		const Triangle* tri = m_triangles[i]; // 現在の三角形を取得
        const BOUNDING_BOX& triBox = tri->aabb; // 三角形のAABBを取得

        // 三角形の境界ボックスからボクセルキーを計算
        VoxelKey minKey = GetVoxelKey(triBox.minPoint);
        VoxelKey maxKey = GetVoxelKey(triBox.maxPoint);

#ifdef _DEBUG
        // EncodeMorton3 は負のキー許容しない
        assert(minKey.x >= 0 && minKey.y >= 0 && minKey.z >= 0);
#endif // DEBUG

        // ボクセルキーの範囲を計算
        for (int x = minKey.x; x <= maxKey.x; ++x)
        {
            for (int y = minKey.y; y <= maxKey.y; ++y)
            {
                for (int z = minKey.z; z <= maxKey.z; ++z)
                {
                    VoxelKey key{ x, y, z };
                    VoxelEntry& voxel = m_voxelMap[key]; // 新規または既存

					voxel.triangles.push_back(tri); // 三角形を追加
                }
            }
        }
    }  

	m_isReady = true; // ビルド完了フラグを設定
}


void VoxelGrid::Query(const XMFLOAT3& center, float radius, SimpleArray<const Triangle*>& outTriangles) const
{
	VoxelKey centerKey = GetVoxelKey(center);
	int radiusVoxels = static_cast<int>(ceil(radius / m_voxelSize));

	// 中心ボクセルから半径分の範囲を計算
    for (int dx = -radiusVoxels; dx <= radiusVoxels; ++dx)
    {
        for (int dy = -radiusVoxels; dy <= radiusVoxels; ++dy)
        {
            for (int dz = -radiusVoxels; dz <= radiusVoxels; ++dz)
            {
				VoxelKey key{ centerKey.x + dx, centerKey.y + dy, centerKey.z + dz };

				// ボクセルキーが存在する場合、対応する三角形を出力
                auto it = m_voxelMap.find(key);
                if (it != m_voxelMap.end())
				{
                    const VoxelEntry& voxel = it->value;

                    // 三角形のリストを出力配列に追加
                    for (const Triangle* tri : voxel.triangles)
                        outTriangles.push_back(tri);
				}
			}
		}
	}
}

VoxelKey VoxelGrid::GetVoxelKey(const XMFLOAT3& pos) const
{
    // 負数の座標を避けるため、origin をオフセットとする
    XMFLOAT3 local = XMFLOAT3(
        (pos.x - m_origin.x) / m_voxelSize,
        (pos.y - m_origin.y) / m_voxelSize,
        (pos.z - m_origin.z) / m_voxelSize
    );

	// 指定された位置からボクセルキーを計算
    return VoxelKey{
        static_cast<int>(floorf(local.x)),
        static_cast<int>(floorf(local.y)),
        static_cast<int>(floorf(local.z))
    };
}

void VoxelGrid::ComputeVoxelBounds(const Triangle* tri)
{
    const BOUNDING_BOX& triBox = tri->aabb; // 三角形のAABBを取得

    m_voxelBounds.minPoint.x = min(m_voxelBounds.minPoint.x, triBox.minPoint.x);
    m_voxelBounds.minPoint.y = min(m_voxelBounds.minPoint.y, triBox.minPoint.y);
    m_voxelBounds.minPoint.z = min(m_voxelBounds.minPoint.z, triBox.minPoint.z);

    m_voxelBounds.maxPoint.x = max(m_voxelBounds.maxPoint.x, triBox.maxPoint.x);
    m_voxelBounds.maxPoint.y = max(m_voxelBounds.maxPoint.y, triBox.maxPoint.y);
    m_voxelBounds.maxPoint.z = max(m_voxelBounds.maxPoint.z, triBox.maxPoint.z);
}

void VoxelGrid::print_debug_stats(void) const
{
    char buffer[512];

    sprintf_s(buffer, "[ハッシュマップ統計情報]\n");
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ 要素数           : %zu\n", m_voxelMap.getSize());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ バケット数       : %d\n", m_voxelMap.bucketCount());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ 負荷率 (load)    : %.3f\n", m_voxelMap.load_factor());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ 使用中のバケット : %d\n", m_voxelMap.used_bucket_count());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ ハッシュ衝突数   : %d\n", m_voxelMap.collision_count());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ 最大チェーン長   : %d\n", m_voxelMap.max_chain_length());
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "◆ 平均チェーン長   : %.3f\n", m_voxelMap.average_chain_length());
    OutputDebugStringA(buffer);
}
