#pragma once
//=============================================================================
//
// AABB による三角形空間分割構造（八分木） [OctreeNode.h]
// Author : 
// 三角形集合を AABB 単位で再帰的に空間分割し、
// 高速な領域クエリと交差判定を可能にするデータ構造を提供する
// 
//=============================================================================
#include "main.h"
#include "Core/Async/LightweightMutex.h"
#include "Utility/SimpleArray.h"
#include "Collision/AABBUtils.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_TRIANGLES           10 // 1ノードあたりの最大三角形数
#define CHILD_COUNT             8

// 八分木ノードの定義。各ノードは自作の AABB 境界を持ち、Triangle* を格納する。
class OctreeNode 
{
public:
    OctreeNode(const BOUNDING_BOX& boundary, bool threadSafe = true, int depth = 0, int maxDepth = 8)
        : m_boundary(boundary), m_threadSafe(threadSafe), m_depth(depth), m_maxDepth(maxDepth) {}

    ~OctreeNode();

    // 三角形をこの八分木に挿入する関数
    bool Insert(const Triangle* tri);

    // 指定した範囲 (AABB) と交差する三角形をクエリする関数
    void QueryRange(const BOUNDING_BOX& range, SimpleArray<const Triangle*>& result);

    const BOUNDING_BOX& GetBoundary(void) const { return m_boundary; }

private:
    // 現在のノードを8つの子ノードに分割する関数
    void Subdivide(void);

    BOUNDING_BOX m_boundary; // このノードがカバーする領域（ワールド座標系）
    SimpleArray<const Triangle*> m_triangles;   // ノードに格納される三角形ポインタのリスト
    OctreeNode* m_children[CHILD_COUNT] = { nullptr }; // 8つの子ノードへのポインタ
    int m_depth;
    int m_maxDepth;
    bool m_threadSafe = true; // デフォルトでスレッドセーフ有効

    LightweightMutex m_lock; // 各ノードの互互掛かりを防ぐ
};