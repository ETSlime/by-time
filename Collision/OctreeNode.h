//=============================================================================
//
// AABB による三角形空間分割構造（八分木） [OctreeNode.h]
// Author : 
// 三角形集合を AABB 単位で再帰的に空間分割し、
// 高速な領域クエリと交差判定を可能にするデータ構造を提供する
// 
//=============================================================================
#pragma once
#include "main.h"
#include "Utility/SimpleArray.h"
#include "Collision/AABBUtils.h"
//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct Triangle
{
    XMFLOAT3 v0, v1, v2;
    BOUNDING_BOX aabb;
    XMFLOAT3 normal;

    // AABBを計算
    Triangle(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, bool alwaysFaceUp = false) : v0(a), v1(b), v2(c)
    {
        aabb.minPoint.x = min(min(v0.x, v1.x), v2.x);
        aabb.minPoint.y = min(min(v0.y, v1.y), v2.y);
        aabb.minPoint.z = min(min(v0.z, v1.z), v2.z);
        aabb.maxPoint.x = max(max(v0.x, v1.x), v2.x);
        aabb.maxPoint.y = max(max(v0.y, v1.y), v2.y);
        aabb.maxPoint.z = max(max(v0.z, v1.z), v2.z);

        // 法線を計算する（頂点座標から交叉積を利用して正規化する）
        XMVECTOR v0Vec = XMLoadFloat3(&v0);
        XMVECTOR v1Vec = XMLoadFloat3(&v1);
        XMVECTOR v2Vec = XMLoadFloat3(&v2);
        XMVECTOR edge1 = XMVectorSubtract(v1Vec, v0Vec);
        XMVECTOR edge2 = XMVectorSubtract(v2Vec, v0Vec);
        XMVECTOR normVec = XMVector3Cross(edge1, edge2);
        normVec = XMVector3Normalize(normVec);

        // 法線のy成分を確認し、負の場合は反転
        if (XMVectorGetY(normVec) < 0.0f && alwaysFaceUp)
        {
            normVec = XMVectorNegate(normVec);  // 法線反転
        }
        XMStoreFloat3(&normal, normVec);
    }

    // コンストラクタ：三角形の法線を直接指定できるようにする
    Triangle(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, const XMFLOAT3& norm)
        : v0(a), v1(b), v2(c), normal(norm)
    {
        aabb.minPoint.x = min(min(v0.x, v1.x), v2.x);
        aabb.minPoint.y = min(min(v0.y, v1.y), v2.y);
        aabb.minPoint.z = min(min(v0.z, v1.z), v2.z);
        aabb.maxPoint.x = max(max(v0.x, v1.x), v2.x);
        aabb.maxPoint.y = max(max(v0.y, v1.y), v2.y);
        aabb.maxPoint.z = max(max(v0.z, v1.z), v2.z);
    }
};

// 八分木ノードの定義。各ノードは自作の AABB 境界を持ち、Triangle* を格納する。
class OctreeNode 
{
public:
    static const int MAX_TRIANGLES = 10; // 1ノードあたりの最大三角形数
    BOUNDING_BOX boundary; // このノードがカバーする領域（ワールド座標系）
    SimpleArray<Triangle*> triangles;   // ノードに格納される三角形ポインタのリスト
    OctreeNode* children[8] = { nullptr }; // 8つの子ノードへのポインタ
    int depth;
    int maxDepth;

    OctreeNode(const BOUNDING_BOX& boundary, int depth = 0, int maxDepth = 8)
        : boundary(boundary), depth(depth), maxDepth(maxDepth) {}

    ~OctreeNode() 
    {
        for (int i = 0; i < 8; i++) 
        {
            delete children[i];
        }
    }

    // 現在のノードを8つの子ノードに分割する関数
    void subdivide() 
    {
        XMFLOAT3 pMin = boundary.minPoint;
        XMFLOAT3 pMax = boundary.maxPoint;

        // 境界の中心点を計算する
        XMFLOAT3 center;
        center.x = (pMin.x + pMax.x) * 0.5f;
        center.y = (pMin.y + pMax.y) * 0.5f;
        center.z = (pMin.z + pMax.z) * 0.5f;

        // 8つの子ノードの境界をそれぞれ計算する
        // 子ノード 0: (min, center)
        children[0] = new OctreeNode(BOUNDING_BOX(pMin, center), depth + 1, maxDepth);
        // 子ノード 1: ( (center.x, min.y, min.z), (max.x, center.y, center.z) )
        children[1] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, pMin.y, pMin.z), XMFLOAT3(pMax.x, center.y, center.z)), depth + 1, maxDepth);
        // 子ノード 2: ( (min.x, center.y, min.z), (center.x, max.y, center.z) )
        children[2] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, center.y, pMin.z), XMFLOAT3(center.x, pMax.y, center.z)), depth + 1, maxDepth);
        // 子ノード 3: ( (center.x, center.y, min.z), (max.x, max.y, center.z) )
        children[3] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, center.y, pMin.z), XMFLOAT3(pMax.x, pMax.y, center.z)), depth + 1, maxDepth);
        // 子ノード 4: ( (min.x, min.y, center.z), (center.x, center.y, max.z) )
        children[4] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, pMin.y, center.z), XMFLOAT3(center.x, center.y, pMax.z)), depth + 1, maxDepth);
        // 子ノード 5: ( (center.x, min.y, center.z), (max.x, center.y, max.z) )
        children[5] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(center.x, pMin.y, center.z), XMFLOAT3(pMax.x, center.y, pMax.z)), depth + 1, maxDepth);
        // 子ノード 6: ( (min.x, center.y, center.z), (center.x, max.y, max.z) )
        children[6] = new OctreeNode(BOUNDING_BOX(XMFLOAT3(pMin.x, center.y, center.z), XMFLOAT3(center.x, pMax.y, pMax.z)), depth + 1, maxDepth);
        // 子ノード 7: ( (center.x, center.y, center.z), (max.x, max.y, max.z) )
        children[7] = new OctreeNode(BOUNDING_BOX(center, pMax), depth + 1, maxDepth);
    }

    // 三角形をこの八分木に挿入する関数
    bool insert(Triangle* tri) 
    {
        // 三角形の AABB がこのノードの境界と交差しなければ挿入しない
        if (!boundary.intersects(tri->aabb))
            return false;

        // 現在のノードが葉で、容量があるかまたは最大深さに達している場合はここに格納する
        if (triangles.getSize() < MAX_TRIANGLES || depth == maxDepth) 
        {
            triangles.push_back(tri);
            return true;
        }

        // 子ノードに分割されていなければ、分割する
        if (children[0] == nullptr) 
        {
            subdivide();
        }

        // 子ノードに挿入を試みる
        bool inserted = false;
        for (int i = 0; i < 8; i++) 
        {
            if (children[i]->insert(tri))
                inserted = true;
        }

        // もし子ノードのどこにも完全に収まらない場合は、現在のノードに保持する
        if (!inserted)
            triangles.push_back(tri);
        return true;
    }

    // 指定した範囲 (AABB) と交差する三角形をクエリする関数
    void queryRange(const BOUNDING_BOX& range, SimpleArray<Triangle*>& result) const
    {
        // 現在のノードと範囲が交差しなければ、何もせずリターン
        if (!boundary.intersects(range))
            return;

        // 現在のノードに格納された各三角形について、AABB の交差を確認する
        int size = triangles.getSize();
        for (int i = 0; i < size; i++)
        {
            if (triangles[i]->aabb.intersects(range))
                result.push_back(triangles[i]);
        }

        // 子ノードが存在する場合、再帰的にクエリを実行する
        if (children[0] != nullptr) 
        {
            for (int i = 0; i < 8; i++) 
            {
                children[i]->queryRange(range, result);
            }
        }
    }
};