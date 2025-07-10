#pragma once
//=============================================================================
//
// [MarchingCubesSurfaceMeshGPU.h]
// Author : 
//
//=============================================================================
#include "Effects/MarchingCubesSurfaceMesh.h"
#include "Effects/ParticleEffectRendererBase.h"

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct DrawIndexedInstancedArgs
{
    UINT IndexCountPerInstance;
    UINT InstanceCount;
    UINT StartIndexLocation;
    INT  BaseVertexLocation;
    UINT StartInstanceLocation;
}; // DrawIndexedInstancedIndirect 用の引数構造体

class MarchingCubesSurfaceMeshGPU : public MarchingCubesSurfaceMesh
{
public:
    void Initialize(void);
    void UpdateMesh(ID3D11ShaderResourceView* scalarFieldSRV, const VolumeReconstructionParams& params);
    void Draw(void);
    void ComputeMeshRequirementsByFieldDim(const XMUINT3& fieldDim, UINT* outVertexCount, UINT* outIndexCount);
private:
    void EnsureBufferCapacity(const XMUINT3& fieldDim);
    void CreateMeshBuffers(void);
	void ReleaseMeshBuffer(void);

    // GPU buffer
    ParticleBufferSet m_vertexBufferSet;     // RWStructuredBuffer<MCVertex>
    ParticleBufferSet m_indexBufferSet;      // RWStructuredBuffer<uint>
    ParticleBufferSet m_drawArgsBufferSet;   // DrawIndexedInstancedIndirect 用

    // Marching Cubes コンピュートシェーダー
	ComputeShaderSet m_marchingCubesCS;
};