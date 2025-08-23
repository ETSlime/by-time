#pragma once
//=============================================================================
//
// [ProjectilePredictor.h]
// Author : 
// 
//=============================================================================
#include "Collision/CollisionManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define     MAX_TRAJECTORY_POINTS 5000 // 軌跡点の最大数

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
    ProjectilePredictor(float gravity = 1.0f);

    // 軌跡を計算して点リストを返す
    void ComputeTrajectory(
        const XMFLOAT3& start,
        const XMFLOAT3& dir,
        float speed,
        float maxTime,
        float stepTime,
        XMFLOAT3* outHitPos,
        XMFLOAT3* outHitNormal);

    void SetCollisionVoxelGrid(const VoxelGrid* grid) { m_voxelGrid = grid; }

    // 軌跡データを取得
    const SimpleArray<TrajectoryPoint>& GetTrajectoryPoints(void) const { return m_trajectoryPoints; }

private:
    SimpleArray<TrajectoryPoint> m_trajectoryPoints;
    float m_gravity;
    const VoxelGrid* m_voxelGrid;

};