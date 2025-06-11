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

}

void VoxelGrid::Initialize(float voxelSize)
{
	// ボクセルサイズの設定
	m_voxelSize = voxelSize;
    // ボクセルマップの初期化
	m_voxelMap.clear();
}

void VoxelGrid::Build(const SimpleArray<Triangle*>& triangles)
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
    BOUNDING_BOX triBox = ComputeTriangleAABB(tri);

    // 三角形の境界ボックスからボクセルキーを計算
    VoxelKey minKey = GetVoxelKey(triBox.minPoint);
    VoxelKey maxKey = GetVoxelKey(triBox.maxPoint);

    // ボクセルキーの範囲を計算
    for (int x = minKey.x; x <= maxKey.x; ++x)
    {
        for (int y = minKey.y; y <= maxKey.y; ++y)
        {
            for (int z = minKey.z; z <= maxKey.z; ++z)
            {
                VoxelKey key{ x, y, z };
                VoxelEntry& voxel = m_voxelMap[key]; // 新規または既存

                if (m_threadSafe)
                {
                    ScopedSpinLock guard(voxel.lock); // スピンロックで排他制御
					voxel.triangles.push_back(tri); // 三角形を追加
                }
                else
                {
					voxel.triangles.push_back(tri); // 三角形を追加
                }
            }
        }
    }
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
                    if (m_threadSafe)
                    {
                        ScopedSpinLock guard(voxel.lock); // スピンロックで読み取り排他
                        for (const Triangle* tri : voxel.triangles)
                            outTriangles.push_back(tri);
                    }
                    else
                    {
                        for (const Triangle* tri : voxel.triangles)
                            outTriangles.push_back(tri);
                    }
				}
			}
		}
	}
}

VoxelKey VoxelGrid::GetVoxelKey(const XMFLOAT3& pos) const
{
	// 指定された位置からボクセルキーを計算
    return {
        static_cast<int>(floor(pos.x / m_voxelSize)),
        static_cast<int>(floor(pos.y / m_voxelSize)),
        static_cast<int>(floor(pos.z / m_voxelSize))
    };
}

BOUNDING_BOX VoxelGrid::ComputeTriangleAABB(const Triangle* tri) const
{
    BOUNDING_BOX box;

	// 三角形の頂点から AABB を計算
    box.minPoint.x = min(min(tri->v0.x, tri->v1.x), tri->v2.x);
    box.minPoint.y = min(min(tri->v0.y, tri->v1.y), tri->v2.y);
    box.minPoint.z = min(min(tri->v0.z, tri->v1.z), tri->v2.z);
    box.maxPoint.x = max(max(tri->v0.x, tri->v1.x), tri->v2.x);
    box.maxPoint.y = max(max(tri->v0.y, tri->v1.y), tri->v2.y);
    box.maxPoint.z = max(max(tri->v0.z, tri->v1.z), tri->v2.z);

    return box;
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
