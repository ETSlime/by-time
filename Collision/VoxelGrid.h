#pragma once
//=============================================================================
//
// 空間加速構造体ちゃん [VoxelGrid.h]
// Author : 
// ボクセルベースの空間分割によって、三角形メッシュを3Dグリッドに整理し、
// 高速なクエリ（衝突判定・視界探索・地形サンプリングなど）を可能にする
// 
// 主な機能：
//   - モートンコード（Morton Encoding）による高速ハッシュ
//   - スレッドセーフ対応（SpinLockでガード）
//   - 三角形の一括登録／動的追加
//   - 指定半径内の高速検索クエリ(Query)
//
//=============================================================================
#include "Utility/HashMap.h"
#include "Collision/AABBUtils.h"
#include "Core/Async/SpinLock.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_VOXEL_NUM       20000000 // ボクセルグリッドの最大サイズ

struct VoxelKey
{
    int x, y, z;
}; // ボクセルのキーを表す構造体

// ボクセルキーのハッシュ関数
inline uint64_t Part1By2(uint32_t x)
{
    x &= 0x3ff; // 限定10位
    x = (x | (x << 16)) & 0x30000ff;
    x = (x | (x << 8)) & 0x300f00f;
    x = (x | (x << 4)) & 0x30c30c3;
    x = (x | (x << 2)) & 0x9249249;
    return x;
}

// 3次元座標をモートンコードにエンコードする関数
inline uint64_t EncodeMorton3(uint32_t x, uint32_t y, uint32_t z)
{
    return (Part1By2(z) << 2) | (Part1By2(y) << 1) | Part1By2(x);
}

// VoxelKey のハッシュ関数と等価比較関数を定義
struct HashVoxelKey
{
    size_t operator()(const VoxelKey& key) const
    {
        return static_cast<size_t>(EncodeMorton3(key.x, key.y, key.z));
    }
};

// VoxelKey の等価比較関数
struct EqualVoxelKey
{
    bool operator()(const VoxelKey& a, const VoxelKey& b) const
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
};

class VoxelGrid
{
public:
    VoxelGrid(bool threadSafe = true) :m_threadSafe(threadSafe) {}

    ~VoxelGrid(void);

    // ボクセルグリッドの初期化
    void Initialize(float voxelSize = 100.0f);
    // 静的地形データから一括で構築（高速）
    void Build(const SimpleArray<Triangle*>& triangles);
    // 三角形を動的に追加（物理オブジェクトや生成地形など）
    void InsertTriangle(const Triangle* tri);

	// 指定された中心と半径に基づいて、ボクセルグリッドから三角形をクエリする関数
    void Query(const XMFLOAT3& center, float radius, SimpleArray<const Triangle*>& outTriangles) const;

    // デバッグ用の統計情報を出力
    void print_debug_stats(void) const;

private:
    // 各Voxelに登録された三角形＋スピンロック
    struct VoxelEntry
    {
        SimpleArray<const Triangle*> triangles;
        mutable SpinLock lock;
    };

    // 位置ベクトルから所属するVoxelキーを算出
    VoxelKey GetVoxelKey(const XMFLOAT3& pos) const;
    // 三角形の軸方向AABBを計算する（各Voxelとの交差判定用）
    BOUNDING_BOX ComputeTriangleAABB(const Triangle* tri) const;

    float m_voxelSize = 1.0f;
    bool m_threadSafe = true; // スレッドセーフかどうか

	// ボクセルキーと三角形のマッピングを保持するハッシュマップ
	HashMap<VoxelKey, VoxelEntry, HashVoxelKey, EqualVoxelKey> m_voxelMap =
        HashMap<VoxelKey, VoxelEntry, HashVoxelKey, EqualVoxelKey>(
        MAX_VOXEL_NUM, // 初期バケット数
		HashVoxelKey(), // ハッシュ関数
		EqualVoxelKey() // 等価比較関数
        );
};