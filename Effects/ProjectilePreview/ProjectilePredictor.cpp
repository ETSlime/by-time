//=============================================================================
//
// [ProjectilePredictor.cpp]
// Author : 
// 
//=============================================================================
#include "Effects/ProjectilePreview/ProjectilePredictor.h"

ProjectilePredictor::ProjectilePredictor(float gravity) 
	: m_gravity(gravity), m_voxelGrid(nullptr) {}

void ProjectilePredictor::ComputeTrajectory(const XMFLOAT3& start, const XMFLOAT3& dir, float speed, float maxTime, 
	float stepTime, SimpleArray<TrajectoryPoint>& outPoints, XMFLOAT3* outHitPos)
{
    outPoints.clear();
    if (outHitPos) *outHitPos = XMFLOAT3(0, 0, 0);

    XMFLOAT3 prevPos = start;
    bool hitFound = false;

    for (float t = 0.0f; t <= maxTime; t += stepTime) 
    {
        XMFLOAT3 pos;
        pos.x = start.x + dir.x * speed * t;
        pos.y = start.y + dir.y * speed * t - 0.5f * m_gravity * t * t;
        pos.z = start.z + dir.z * speed * t;

        // DDA‚Å‚ÌÕ“Ë”»’è
        XMFLOAT3 hitPos;
        if (m_voxelGrid && m_voxelGrid->Raycast(prevPos, pos, &hitPos)) 
        {
            outPoints.push_back({ hitPos, true });
            if (outHitPos) *outHitPos = hitPos;
            hitFound = true;
            break;
        }
        else 
        {
            outPoints.push_back({ pos, false });
        }

        prevPos = pos;
    }
}