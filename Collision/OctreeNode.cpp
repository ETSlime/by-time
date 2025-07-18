//=============================================================================
//
// AABB による三角形空間分割構造（八分木） [OctreeNode.cpp]
// Author : 
// 三角形集合を AABB 単位で再帰的に空間分割し、
// 高速な領域クエリと交差判定を可能にするデータ構造を提供する
// 
//=============================================================================
#include "OctreeNode.h"

OctreeNode::~OctreeNode()
{
    for (int i = 0; i < 8; i++)
    {
        SAFE_DELETE(m_children[i]);
    }
}

bool OctreeNode::Insert(const Triangle* tri)
{
    // ノードの基本情報に対してロックをかける
    OptionalLockGuard guard(&m_lock, m_threadSafe);

    // 三角形の AABB がこのノードの境界と交差しなければ挿入しない
    if (!m_boundary.intersects(tri->aabb))
        return false;

    // 現在のノードが葉で、容量があるかまたは最大深さに達している場合はここに格納する
    if (m_triangles.getSize() < MAX_TRIANGLES || m_depth == m_maxDepth)
    {
        m_triangles.push_back(tri);
        return true;
    }

    // subdivide() が必要か一時判定（ダブルチェック用）
    bool needSubdivide = (m_children[0] == nullptr);

    // 最初のロックを解除して subdivide() によるデッドロックを防止
    guard.~OptionalLockGuard();

    // subdivide() を改めてロックのもと実行（ダブルチェックあり）
    if (needSubdivide)
    {
        OptionalLockGuard subdivideGuard(&m_lock, m_threadSafe);
        if (m_children[0] == nullptr)
            Subdivide(); // 子ノードに分割されていなければ、分割する
    }


    // 子ノードに挿入を試みる
    bool inserted = false;
    for (int i = 0; i < CHILD_COUNT; i++)
    {
        if (m_children[i]->Insert(tri))
            inserted = true;
    }

    // もし子ノードのどこにも完全に収まらない場合は、現在のノードに保持する
    if (!inserted)
    {
        OptionalLockGuard fallback(&m_lock, m_threadSafe);
        m_triangles.push_back(tri);
    }
    return true;
}

void OctreeNode::QueryRange(const BOUNDING_BOX& range, SimpleArray<const Triangle*>& result)
{
    OptionalLockGuard guard(const_cast<LightweightMutex*>(&m_lock), m_threadSafe);

    // 現在のノードと範囲が交差しなければ、何もせずリターン
    if (!m_boundary.intersects(range))
        return;

    // 現在のノードに格納された各三角形について、AABB の交差を確認する
    int size = m_triangles.getSize();
    for (int i = 0; i < size; i++)
    {
        if (m_triangles[i]->aabb.intersects(range))
            result.push_back(m_triangles[i]);
    }

    // ロック解除後も完全な同期性を保証
    OctreeNode* childrenCopy[CHILD_COUNT];
    for (int i = 0; i < CHILD_COUNT; ++i)
        childrenCopy[i] = m_children[i];

    // 子ノードが存在する場合、再帰的にクエリを実行する
    if (m_children[0] != nullptr)
    {
        for (int i = 0; i < CHILD_COUNT; i++)
        {
            m_children[i]->QueryRange(range, result);
        }
    }
}

void OctreeNode::Subdivide(void)
{
    XMFLOAT3 pMin = m_boundary.minPoint;
    XMFLOAT3 pMax = m_boundary.maxPoint;

    // 境界の中心点を計算する
    XMFLOAT3 center;
    center.x = (pMin.x + pMax.x) * 0.5f;
    center.y = (pMin.y + pMax.y) * 0.5f;
    center.z = (pMin.z + pMax.z) * 0.5f;

    // 8つの子ノードの境界をそれぞれ計算する
    // 子ノード 0: (min, center)
    m_children[0] = new OctreeNode(BOUNDING_BOX(pMin, center), m_depth + 1, m_maxDepth);
    // 子ノード 1: ( (center.x, min.y, min.z), (max.x, center.y, center.z) )
    m_children[1] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, pMin.y, pMin.z), XMFLOAT3(pMax.x, center.y, center.z)), m_depth + 1, m_maxDepth);
    // 子ノード 2: ( (min.x, center.y, min.z), (center.x, max.y, center.z) )
    m_children[2] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, center.y, pMin.z), XMFLOAT3(center.x, pMax.y, center.z)), m_depth + 1, m_maxDepth);
    // 子ノード 3: ( (center.x, center.y, min.z), (max.x, max.y, center.z) )
    m_children[3] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, center.y, pMin.z), XMFLOAT3(pMax.x, pMax.y, center.z)), m_depth + 1, m_maxDepth);
    // 子ノード 4: ( (min.x, min.y, center.z), (center.x, center.y, max.z) )
    m_children[4] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, pMin.y, center.z), XMFLOAT3(center.x, center.y, pMax.z)), m_depth + 1, m_maxDepth);
    // 子ノード 5: ( (center.x, min.y, center.z), (max.x, center.y, max.z) )
    m_children[5] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, pMin.y, center.z), XMFLOAT3(pMax.x, center.y, pMax.z)), m_depth + 1, m_maxDepth);
    // 子ノード 6: ( (min.x, center.y, center.z), (center.x, max.y, max.z) )
    m_children[6] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, center.y, center.z), XMFLOAT3(center.x, pMax.y, pMax.z)), m_depth + 1, m_maxDepth);
    // 子ノード 7: ( (center.x, center.y, center.z), (max.x, max.y, max.z) )
    m_children[7] = new OctreeNode(BOUNDING_BOX(center, pMax), m_depth + 1, m_maxDepth);
}