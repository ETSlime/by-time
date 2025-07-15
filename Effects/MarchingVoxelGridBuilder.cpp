//=============================================================================
//
//  [MarchingVoxelGridBuilder.cpp]
// Author : 
//
//=============================================================================
#include "Effects/MarchingVoxelGridBuilder.h"

void SparseVoxelGridUploader::Initialize(float cellSize, const XMFLOAT3& gridMin)
{
    m_gridOrigin = gridMin;
    m_cellSize = cellSize;

    m_voxelTriplets.clear();
    m_pageHeaders.clear();
}

void SparseVoxelGridUploader::BuildGrid(const SimpleArray<XMINT3>& activeVoxelCoords)
{
    // 各体素に対して Morton インデックスを計算
    std::vector<std::pair<uint64_t, uint3>> mortonAndCoords;
    for (const auto& coord : activeVoxels)
    {
        uint64_t morton = EncodeMorton3D(coord.x, coord.y, coord.z);
        mortonAndCoords.emplace_back(morton, coord);
    }

    // Morton インデックス順にソート
    std::sort(mortonAndCoords.begin(), mortonAndCoords.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    //===== 3. SPHIndexTriplet 配列を構築 =====
    for (size_t i = 0; i < mortonAndCoords.size(); ++i)
    {
        SPHIndexTriplet triplet{};
        triplet.gridCoord = mortonAndCoords[i].second;
        triplet.mortonIndex = mortonAndCoords[i].first;
        triplet.count = 1;
        triplet.startOffset = static_cast<uint>(i); // Flat index

        m_indexTriplets.push_back(triplet);
    }

    //===== 4. Page Header 配列を構築（固定ページサイズ） =====
    const uint pageSize = 64;
    for (uint i = 0; i < m_indexTriplets.size(); i += pageSize)
    {
        IndexTripletPageHeaderVec2 header{};
        header.pageOffset = i;
        header.pageCount = std::min(pageSize, (uint)(m_indexTriplets.size() - i));
        header.pageStartMorton = m_indexTriplets[i].mortonIndex;
        header.pageEndMorton = m_indexTriplets[i + header.pageCount - 1].mortonIndex;
        m_pageHeaders.push_back(header);
    }
}
