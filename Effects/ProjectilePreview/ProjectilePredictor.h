#pragma once
//=============================================================================
//
// [ProjectilePredictor.h]
// Author : 
// 
//=============================================================================
#include "Collision/CollisionManager.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************

// 軌跡点データ
struct TrajectoryPoint 
{
    XMFLOAT3 pos;  // 位置
    bool hit;      // 命中点かどうか
};

class ProjectilePredictor 
{
public:
    ProjectilePredictor(float gravity = 9.8f);

    // 軌跡を計算して点リストを返す
    void ComputeTrajectory(
        const XMFLOAT3& start,
        const XMFLOAT3& dir,
        float speed,
        float maxTime,
        float stepTime,
        SimpleArray<TrajectoryPoint>& outPoints,
        XMFLOAT3* outHitPos);

    void SetCollisionVoxelGrid(const VoxelGrid* grid) { m_voxelGrid = grid; }

private:

    float m_gravity;
    const VoxelGrid* m_voxelGrid;

};