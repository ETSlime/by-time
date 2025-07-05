//=============================================================================
//
// [FluidHeightFieldBuilder.cpp]
// Author : 
//
//=============================================================================
#include "FluidHeightFieldBuilder.h"

FluidHeightFieldBuilder::FluidHeightFieldBuilder() 
{
    // 初期グリッドサイズ（X方向）を仮に32とする
    const int initialGridX = 32;
    const int initialGridZ = 32;

	m_grid.reserve(initialGridX); // 外側配列の容量を確保
	m_grid.setSize(initialGridX); // 外側配列のサイズを設定

    // 内側配列（Z方向）も各列に対して事前に確保
    for (int gx = 0; gx < initialGridX; ++gx)
    {
        m_grid[gx].reserve(initialGridZ);  // 各列に対してZ方向の容量を確保
        m_grid[gx].setSize(initialGridZ);  // 各列のサイズを設定
    }
}

FluidHeightFieldBuilder::~FluidHeightFieldBuilder()
{
    // 全グリッド列を解放（Clear()はm_activeGridXしか処理しないので使用不可）
    for (UINT gx = 0; gx < m_grid.getSize(); ++gx)
    {
        m_grid[gx].clear();      // 各列のセルを削除
        m_grid[gx].setSize(0);   // サイズを0に設定（メモリは保持される）
    }

    m_grid.clear();              // 外側配列を削除
    m_grid.setSize(0);           // サイズをリセット
}

void FluidHeightFieldBuilder::Initialize(float minX, float maxX, float minZ, float maxZ, int gridX, int gridZ)
{
	m_gridX = gridX;
	m_gridZ = gridZ;
	m_minX = minX;
	m_maxX = maxX;
	m_minZ = minZ;
	m_maxZ = maxZ;

    m_cellSizeX = (maxX - minX) / static_cast<float>(gridX);
    m_cellSizeZ = (maxZ - minZ) / static_cast<float>(gridZ);

    // 現在のフレームで使用するグリッドサイズを記録
    m_activeGridX = gridX;
    m_activeGridZ = gridZ;

    // 外側配列（X方向）を拡張または初期化
    if (static_cast<UINT>(gridX) > m_grid.getSize())
    {
        // 容量不足ならリザーブ（2倍に拡張）
        if (static_cast<UINT>(gridX) > m_grid.getCapacity())
            m_grid.reserve(gridX * 2);

        // サイズを拡張（小さくはしない）
        m_grid.setSize(gridX);

        // 新しく追加された列を初期化
        for (int gx = m_activeGridX; gx < gridX; ++gx)
        {
            m_grid[gx].reserve(gridZ * 2); // 内側配列を事前にリザーブ
            m_grid[gx].setSize(gridZ);     // 必要な行数に設定
        }
    }

    // 既存の列に対して、内側配列（Z方向）を拡張
    for (int gx = 0; gx < m_activeGridX; ++gx)
    {
        if (static_cast<UINT>(gridZ) > m_grid[gx].getSize())
        {
            if (static_cast<UINT>(gridZ) > m_grid[gx].getCapacity())
                m_grid[gx].reserve(gridZ * 2); // 容量不足なら拡張

            m_grid[gx].setSize(gridZ); // サイズ更新（縮小しない）
        }
    }
}

void FluidHeightFieldBuilder::Initialize(const BOUNDING_BOX& bounds, float cellSize)
{
    const int minGridX = 8;
    const int minGridZ = 8;

    float width = bounds.GetWidth();
    float depth = bounds.GetDepth();

    int gridX = static_cast<int>(ceil(width / cellSize));
    int gridZ = static_cast<int>(ceil(depth / cellSize));

    // 最小グリッドサイズ保証（小さすぎると崩れる）
    gridX = max(gridX, minGridX);
    gridZ = max(gridZ, minGridZ);

    float adjustedWidth = gridX * cellSize;
    float adjustedDepth = gridZ * cellSize;

    float minX = bounds.minPoint.x;
    float maxX = minX + adjustedWidth;
    float minZ = bounds.minPoint.z;
    float maxZ = minZ + adjustedDepth;

    // 通常の初期化呼び出し
    Initialize(minX, maxX, minZ, maxZ, gridX, gridZ);
}

void FluidHeightFieldBuilder::AccumulateParticles(const SimpleArray<WaterFluidParticle>& particles)
{
	int count = particles.getSize();
    for (int i = 0; i < count; ++i)
    {
        const WaterFluidParticle& p = particles[i];

        float x = p.position.x;
        float y = p.position.y;
        float z = p.position.z;

        int gx = static_cast<int>((x - m_minX) / m_cellSizeX);
        int gz = static_cast<int>((z - m_minZ) / m_cellSizeZ);

        if (gx < 0 || gx >= m_gridX || gz < 0 || gz >= m_gridZ)
            continue;

        m_grid[gx][gz].heightSum += y;
        m_grid[gx][gz].count += 1;
    }
}

void FluidHeightFieldBuilder::Finalize(void)
{
    for (int gx = 0; gx < m_gridX; ++gx)
    {
        for (int gz = 0; gz < m_gridZ; ++gz)
        {
            Cell& cell = m_grid[gx][gz];

            if (cell.count > 0)
                cell.finalHeight = cell.heightSum / static_cast<float>(cell.count);
            else
            {
                // 周囲の平均高さを使って埋める（3x3）
                float sum = 0.0f;
                int count = 0;

                for (int dx = -1; dx <= 1; ++dx)
                {
                    for (int dz = -1; dz <= 1; ++dz)
                    {
                        int nx = gx + dx;
                        int nz = gz + dz;

                        if (nx >= 0 && nx < m_gridX && nz >= 0 && nz < m_gridZ)
                        {
                            const Cell& neighbor = m_grid[nx][nz];
                            if (neighbor.count > 0)
                            {
                                sum += neighbor.heightSum / static_cast<float>(neighbor.count);
                                count++;
                            }
                        }
                    }
                }

                if (count > 0)
                {
                    cell.finalHeight = sum / static_cast<float>(count);
                }
                else
                {
                    // 粒子のないセル → NaN を設定
                    cell.finalHeight = NAN;
                }
            }
        }
    }
}

float FluidHeightFieldBuilder::GetHeight(int gx, int gz) const
{
    if (gx < 0 || gx >= m_gridX || gz < 0 || gz >= m_gridZ)
        return NAN;

    return m_grid[gx][gz].finalHeight;
}

void FluidHeightFieldBuilder::Clear(void)
{
    for (int gx = 0; gx < m_activeGridX; ++gx)
    {
        for (int gz = 0; gz < m_activeGridZ; ++gz)
        {
            m_grid[gx][gz] = Cell{}; // 各セルを初期化
        }
    }
}
