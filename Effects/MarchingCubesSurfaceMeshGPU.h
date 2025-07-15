#pragma once
//=============================================================================
//
// [MarchingCubesSurfaceMeshGPU.h]
// Author : 
//
//=============================================================================
#include "Effects/MarchingCubesSurfaceMesh.h"
#include "Effects/ParticleEffectRendererBase.h"
#include "Effects/MCVoxelClusterUploader.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define INITIAL_EDGE_CACHE_CAPACITY         40000

//*****************************************************************************
// 構造体定義
//*****************************************************************************
//struct DrawIndexedInstancedArgs
//{
//    UINT IndexCountPerInstance;
//    UINT InstanceCount;
//    UINT StartIndexLocation;
//    INT  BaseVertexLocation;
//    UINT StartInstanceLocation;
//}; // DrawIndexedInstancedIndirect 用の引数構造体

struct DrawInstancedArgs
{
    UINT VertexCountPerInstance;
    UINT InstanceCount;
    UINT StartVertexLocation;
    UINT StartInstanceLocation;
};

struct DebugVoxelInfo
{
    XMUINT3 gridCoord;
    float cornerScalar[8];
    int cubeIndex;
    BOOL hasMesh;
};

class MarchingCubesSurfaceMeshGPU : public MarchingCubesSurfaceMesh
{
public:
    void Initialize(UINT maxParticles);
    void Release(void);
    void UpdateMesh(ID3D11ShaderResourceView* scalarFieldSRV, 
        const SimpleArray<SPHIndexTriplet>& triplets, const VolumeReconstructionParams& params);
    void Draw(void);
    void ComputeMeshRequirementsByFieldDim(const XMUINT3& fieldDim, 
        UINT* outVertexCount, UINT* outIndexCount, UINT* outEdgeCacheCount);

	// クラスタアップローダーへのアクセス
	SimpleArray<UINT>& GetInitialLabelBuffer(void) { return m_clusterUploader.GetInitialLabelBuffer(); }

private:
    void EnsureGlobalMeshCapacity(UINT maxParticles);
    void EnsureBufferCapacity(const XMUINT3& fieldDim);
    void CreateMeshBuffers(void);
    void CreateLookupTables(void);
    void CreateDrawArgsBuffer(void);
	void CreateDummyIndexBuffer(void);
	void ReleaseMeshBuffer(void);

    // GPU buffer
    ParticleBufferSet m_vertexBufferSet;        // RWStructuredBuffer<MCVertex>
    ParticleBufferSet m_indexBufferSet;         // RWStructuredBuffer<uint>
    ParticleBufferSet m_edgeCacheBufferSet;     // flat エッジキャッシュ
	ParticleBufferSet m_drawArgsBufferSet;      // GPU 書き込み用 DrawIndexedInstancedIndirect の引数バッファ
	ParticleBufferSet m_vertexCounterBufferSet; // RWStructuredBuffer<UINT> for vertex count
	ID3D11Buffer* m_drawIndirectArgsBuffer = nullptr; // DrawIndexedInstancedIndirect 用の引数バッファ

    // Lookup Table 用バッファセット
    ParticleBufferSet m_edgeTableBufferSet; // g_EdgeTable 用
    ParticleBufferSet m_triTableBufferSet;  // g_TriTable 用

    // Debug
	ParticleBufferSet m_debugVoxelInfoBufferSet; // デバッグ用ボクセル情報バッファ（RWStructuredBuffer<DebugVoxelInfo>）

	ID3D11Buffer* m_dummyIB = nullptr; // ダミーインデックスバッファ（DrawIndexedInstancedIndirect 用）

	MCVoxelClusterUploader m_clusterUploader; // クラスタアップローダー

    UINT m_edgeCacheCapacity = INITIAL_EDGE_CACHE_CAPACITY;

    // Marching Cubes コンピュートシェーダー
	ComputeShaderSet m_marchingCubesCS;
};