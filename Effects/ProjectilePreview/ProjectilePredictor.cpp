//=============================================================================
//
// [ProjectilePredictor.cpp]
// Author : 
// 
//=============================================================================
#include "Effects/ProjectilePreview/ProjectilePredictor.h"



ProjectilePredictor::ProjectilePredictor(float gravity) 
	: m_gravity(gravity), m_voxelGrid(nullptr) 
{
    m_trajectoryPoints.reserve(MAX_TRAJECTORY_POINTS);
}

void ProjectilePredictor::ComputeTrajectory(const XMFLOAT3& start, const XMFLOAT3& dir, float speed, 
    float maxTime, float stepTime, XMFLOAT3* outHitPos, XMFLOAT3* outHitNormal)
{
    if (!m_voxelGrid)
		SetCollisionVoxelGrid(CollisionManager::get_instance().GetVoxelGrid());

    m_trajectoryPoints.clear();
    if (outHitPos) *outHitPos = XMFLOAT3(0, 0, 0);
	if (outHitNormal) *outHitNormal = XMFLOAT3(0, 0, 0);

    XMFLOAT3 prevPos = start;
    bool hitFound = false;

    for (float t = 0.0f; t <= maxTime; t += stepTime) 
    {
        XMFLOAT3 pos;
        pos.x = start.x + dir.x * speed * t;
        pos.y = start.y + dir.y * speed * t - 0.5f * m_gravity * t * t;
        pos.z = start.z + dir.z * speed * t;

        // DDA‚Å‚ÌÕ“Ë”»’è
        XMFLOAT3 hitPos, hitNormal;
        if (m_voxelGrid && m_voxelGrid->Raycast(prevPos, pos, &hitPos, &hitNormal))
        {
            m_trajectoryPoints.push_back({ hitPos, true });
            if (outHitPos) *outHitPos = hitPos;
			if (outHitNormal) *outHitNormal = hitNormal;
            hitFound = true;
            break;
        }
        else 
        {
            m_trajectoryPoints.push_back({ pos, false });
        }

        prevPos = pos;
    }
}