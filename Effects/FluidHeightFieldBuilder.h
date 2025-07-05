#pragma once
//=============================================================================
//
// [FluidHeightFieldBuilder.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleStructs.h"
#include "Utility/SimpleArray.h"
#include "Collision/AABBUtils.h"

//*********************************************************
// ç\ë¢ëÃ
//*********************************************************
struct Cell
{
	float heightSum = 0.0f;
	int count = 0;
	float finalHeight = 0.0f;
};

class FluidHeightFieldBuilder
{
public:
	FluidHeightFieldBuilder();
	~FluidHeightFieldBuilder();

	void Initialize(float minX, float maxX, float minZ, float maxZ, int gridX, int gridZ);
	void Initialize(const BOUNDING_BOX& bounds, float cellSize);
	
	void Clear(void);

	void AccumulateParticles(const SimpleArray<WaterFluidParticle>& particles);
	void Finalize(void);
	float GetHeight(int gx, int gz) const;

	inline float GetHeightClamped(int gx, int gz) const
	{
		int clampedX = Clamp(gx, 0, m_gridX - 1);
		int clampedZ = Clamp(gz, 0, m_gridZ - 1);
		float h = GetHeight(clampedX, clampedZ);
		return isnan(h) ? 0.0f : h;
	}

	int GetGridX(void) const { return m_gridX; }
	int GetGridZ(void) const { return m_gridZ; }
	float GetMinX(void) const { return m_minX; }
	float GetMinZ(void) const { return m_minZ; }
	float GetCellSizeX(void) const { return m_cellSizeX; }
	float GetCellSizeZ(void) const { return m_cellSizeZ; }

private:

	SimpleArray<SimpleArray<Cell>> m_grid;
	int m_gridX = 0, m_gridZ = 0;
	float m_minX = FLT_MAX, m_maxX = FLT_MIN;
	float m_minZ = FLT_MAX, m_maxZ = FLT_MIN;
	float m_cellSizeX = 0.0f, m_cellSizeZ = 0.0f;
	int m_activeGridX = 0;
	int m_activeGridZ = 0;
};