//=============================================================================
//
// AABB（軸対称境界ボックス）ユーティリティ [AABBUtils.h]
// Author : 
// 境界ボックスの生成・交差判定・点包含判定・座標変換など、
// バウンディングボックスに関する各種計算処理を提供する
// 
//=============================================================================
#pragma once
#include "main.h"
#include "Utility/SimpleArray.h"
//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct SKINNED_MESH_BOUNDING_BOX
{
    XMFLOAT3 minPoint;
    XMFLOAT3 maxPoint;
    XMFLOAT3 baseMinPoint;
    XMFLOAT3 baseMaxPoint;
    ID3D11Buffer* BBVertexBuffer;
    int	boneIdx;

    SKINNED_MESH_BOUNDING_BOX()
    {
        minPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
        maxPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
        baseMinPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
        baseMaxPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
        BBVertexBuffer = nullptr;
        boneIdx = 0;
    }

    SKINNED_MESH_BOUNDING_BOX(const XMFLOAT3& min, const XMFLOAT3& max)
    {
        minPoint = min;
        maxPoint = max;
        baseMinPoint = min;
        baseMaxPoint = max;
        BBVertexBuffer = nullptr;
        boneIdx = 0;
    }


    ~SKINNED_MESH_BOUNDING_BOX()
    {
        SafeRelease(&BBVertexBuffer);
    }

    // 指定した点がこの AABB 内に含まれるか判定する関数
    bool contains(const XMFLOAT3& point) const
    {
        return (point.x >= minPoint.x && point.x <= maxPoint.x &&
            point.y >= minPoint.y && point.y <= maxPoint.y &&
            point.z >= minPoint.z && point.z <= maxPoint.z);
    }

    // 他の AABB と交差するかどうか判定する関数
    bool intersects(const SKINNED_MESH_BOUNDING_BOX& other) const
    {
        return (minPoint.x <= other.maxPoint.x && maxPoint.x >= other.minPoint.x &&
            minPoint.y <= other.maxPoint.y && maxPoint.y >= other.minPoint.y &&
            minPoint.z <= other.maxPoint.z && maxPoint.z >= other.minPoint.z);
    }
};

struct BOUNDING_BOX
{
    XMFLOAT3 minPoint;
    XMFLOAT3 maxPoint;

    BOUNDING_BOX()
    {
        minPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
        maxPoint = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }

    BOUNDING_BOX(const XMFLOAT3& min, const XMFLOAT3& max)
    {
        minPoint = min;
        maxPoint = max;
    }

    // 指定した点がこの AABB 内に含まれるか判定する関数
    bool contains(const XMFLOAT3& point) const
    {
        return (point.x >= minPoint.x && point.x <= maxPoint.x &&
            point.y >= minPoint.y && point.y <= maxPoint.y &&
            point.z >= minPoint.z && point.z <= maxPoint.z);
    }

    // 他の AABB と交差するかどうか判定する関数
    bool intersects(const BOUNDING_BOX& other) const
    {
        return (minPoint.x <= other.maxPoint.x && maxPoint.x >= other.minPoint.x &&
            minPoint.y <= other.maxPoint.y && maxPoint.y >= other.minPoint.y &&
            minPoint.z <= other.maxPoint.z && maxPoint.z >= other.minPoint.z);
    }

    // min/max を変換したAABBを返す（ワールド空間のAABB）
    BOUNDING_BOX TransformAABB(const XMMATRIX& worldMatrix) const
    {
        XMFLOAT3 corners[8];
        GetCorners(corners);

        XMVECTOR minV = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR maxV = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

        for (int i = 0; i < 8; ++i)
        {
            XMVECTOR corner = XMLoadFloat3(&corners[i]);
            XMVECTOR transformed = XMVector3Transform(corner, worldMatrix);

            minV = XMVectorMin(minV, transformed);
            maxV = XMVectorMax(maxV, transformed);
        }

        BOUNDING_BOX result;
        XMStoreFloat3(&result.minPoint, minV);
        XMStoreFloat3(&result.maxPoint, maxV);
        return result;
    }


    void GetCorners(XMVECTOR outCorners[8]) const
    {
        const XMVECTOR minV = XMLoadFloat3(&minPoint);
        const XMVECTOR maxV = XMLoadFloat3(&maxPoint);

        outCorners[0] = XMVectorSelect(maxV, minV, XMVectorSelectControl(1, 1, 1, 0)); // (minX, minY, minZ)
        outCorners[1] = XMVectorSelect(minV, maxV, XMVectorSelectControl(0, 1, 1, 0)); // (maxX, minY, minZ)
        outCorners[2] = XMVectorSelect(minV, maxV, XMVectorSelectControl(1, 0, 1, 0)); // (minX, maxY, minZ)
        outCorners[3] = XMVectorSelect(minV, maxV, XMVectorSelectControl(0, 0, 1, 0)); // (maxX, maxY, minZ)
        outCorners[4] = XMVectorSelect(minV, maxV, XMVectorSelectControl(1, 1, 0, 0)); // (minX, minY, maxZ)
        outCorners[5] = XMVectorSelect(minV, maxV, XMVectorSelectControl(0, 1, 0, 0)); // (maxX, minY, maxZ)
        outCorners[6] = XMVectorSelect(minV, maxV, XMVectorSelectControl(1, 0, 0, 0)); // (minX, maxY, maxZ)
        outCorners[7] = XMVectorSelect(minV, maxV, XMVectorSelectControl(0, 0, 0, 0)); // (maxX, maxY, maxZ)
    }

    void GetCorners(XMFLOAT3 outCorners[8]) const
    {
        const float xMin = minPoint.x;
        const float yMin = minPoint.y;
        const float zMin = minPoint.z;

        const float xMax = maxPoint.x;
        const float yMax = maxPoint.y;
        const float zMax = maxPoint.z;

        outCorners[0] = XMFLOAT3(xMin, yMin, zMin);
        outCorners[1] = XMFLOAT3(xMax, yMin, zMin);
        outCorners[2] = XMFLOAT3(xMin, yMax, zMin);
        outCorners[3] = XMFLOAT3(xMax, yMax, zMin);
        outCorners[4] = XMFLOAT3(xMin, yMin, zMax);
        outCorners[5] = XMFLOAT3(xMax, yMin, zMax);
        outCorners[6] = XMFLOAT3(xMin, yMax, zMax);
        outCorners[7] = XMFLOAT3(xMax, yMax, zMax);
    }


};

class AABBUtils 
{
public:
    
    // 点の集合からバウンディングボックスを作成する関数
    static BOUNDING_BOX CreateFromPoints(const SimpleArray<XMFLOAT3>& points);
    // 点の集合からバウンディングボックスを作成する関数（XMVECTOR版）
    static BOUNDING_BOX CreateFromPoints(const SimpleArray<XMVECTOR>& points);
};