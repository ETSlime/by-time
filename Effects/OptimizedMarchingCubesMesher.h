#pragma once
//=============================================================================
//
// [OptimizedMarchingCubesMesher.h]
// Author : 
//
//=============================================================================
#include "Core/Graphics/VertexStructs.h"
#include "Effects/SPHVolumeBuilder.h"
#include "Utility/SimpleArray.h"

class OptimizedMarchingCubesMesher
{
public:
    OptimizedMarchingCubesMesher() = default;

    void BuildMesh(const SimpleArray<SimpleArray<SimpleArray<float>>>& scalarField,
        const VolumeReconstructionParams& params);

    // メッシュの頂点数とインデックス数を計算
    void ComputeMeshRequirements(
        const SimpleArray<SimpleArray<SimpleArray<float>>>& scalarField,
        const VolumeReconstructionParams& params,
        UINT* outVertexCount,
        UINT* outIndexCount);

    // メッシュの頂点とインデックスを設定
    void SetTargetBuffer(
        SimpleArray<MarchingCubesVertex>& vertices,
        SimpleArray<UINT>& indices);

    const SimpleArray<MarchingCubesVertex>& GetVertices(void) const { return *m_targetVertices; }
    const SimpleArray<UINT>& GetIndices(void) const { return *m_targetIndices; }

private:
    SimpleArray<MarchingCubesVertex>* m_targetVertices = nullptr;
    SimpleArray<UINT>* m_targetIndices = nullptr;
    SimpleArray<int> m_edgeVertexCache; // flat エッジキャッシュ

    int GetEdgeCacheIndex(int x, int y, int z, int edge, const VolumeReconstructionParams& params) const;

    XMFLOAT3 InterpolateVertex(const XMFLOAT3& p1, const XMFLOAT3& p2, float valp1, float valp2);
    XMFLOAT3 ComputeNormal(const SimpleArray<SimpleArray<SimpleArray<float>>>& field,
        int x, int y, int z);
};
