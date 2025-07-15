#pragma once
//=============================================================================
//
//  [MarchingVoxelGridBuilder.h]
// Author : 
//
//=============================================================================
#include "Effects/SPHGridUploader.h"

class SparseVoxelGridUploader
{
public:
    void Initialize(float cellSize, const XMFLOAT3& gridMin);
    void Shutdown(void);
    void Clear(void);

    void BuildGrid(const SimpleArray<XMINT3>& activeVoxelCoords); // アクティブ体素群からグリッド構築
    void UploadAndBindGridBuffers(void); // GPUへアップロード

    const SimpleArray<SPHIndexTriplet>& GetVoxelTriplets(void) const { return m_voxelTriplets; }
    const SimpleArray<IndexTripletPageHeaderVec2>& GetPageHeaders(void) const { return m_pageHeaders; }
    const SimpleArray<UINT>& GetSortedIndices(void) const { return m_sortedVoxelIndices; }

private:
    void BuildPagedVoxelTripletBuffer(void);
    void MergeSortTriplets(SimpleArray<TempTriplet>& triplets, int left, int right);
    inline bool CompareMorton64(const XMUINT2& a, const XMUINT2& b)
    {
        return (a.y < b.y) || (a.y == b.y && a.x < b.x);
    }

private:
    float m_cellSize = 1.0f;
    XMFLOAT3 m_gridOrigin = { 0,0,0 };

    SimpleArray<UINT> m_sortedVoxelIndices; // アクティブ体素のソート済みインデックス
    SimpleArray<SPHIndexTriplet> m_voxelTriplets; // モートンインデックス→体素群
    SimpleArray<IndexTripletPageHeaderVec2> m_pageHeaders; // ページ情報

    ParticleBufferSet m_voxelTripletBufferSet;
    ParticleBufferSet m_pageHeaderBufferSet;

    HashMap<uint64_t, SimpleArray<UINT>, HashUInt64, EqualUInt64> m_mortonVoxelMap =
        HashMap<uint64_t, SimpleArray<UINT>, HashUInt64, EqualUInt64>(
            4096, HashUInt64(), EqualUInt64()
        );

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
};