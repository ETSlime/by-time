//=============================================================================
//
// [MarchingCubesSurfaceMeshGPU.h]
// Author : 
//
//=============================================================================
#include "Effects/MarchingCubesSurfaceMeshGPU.h"

void MarchingCubesSurfaceMeshGPU::Initialize(void)
{
	m_buildOnGPU = true; // GPU上でのメッシュ生成を有効化

    // 基底クラスの初期化
	MarchingCubesSurfaceMesh::Initialize();

    // メッシュバッファの作成
	CreateMeshBuffers();

    // DrawArgsバッファ（RWStructuredBuffer<DrawIndexedInstancedArgs>）
    ParticleEffectRendererBase::CreateStructuredBuffer(m_device,
        sizeof(DrawIndexedInstancedArgs),   // 構造体サイズ
        1,                                  // 要素数は1つ（DrawIndexedInstancedArgs用）
        nullptr,                            // 初期データはなし
        &m_drawArgsBufferSet.buffer,        // RWStructuredBuffer<DrawIndexedInstancedArgs>
        nullptr,                            // SRV は不要
        &m_drawArgsBufferSet.uav            // DrawIndexedInstancedArgs 用の UAV
    );

    // シェーダーの取得
    m_marchingCubesCS = ShaderManager::get_instance().GetComputeShader(ParticleComputeGroup::WaterFluid, ComputePassType::MarchingCubes);
}

void MarchingCubesSurfaceMeshGPU::UpdateMesh(ID3D11ShaderResourceView* scalarFieldSRV, const VolumeReconstructionParams& params)
{
	if (!m_marchingCubesCS.cs || !scalarFieldSRV)
	{
		OutputDebugStringA("[MarchingCubesSurfaceMeshGPU] シェーダーまたはスカラー場SRVが無効です。\n");
		return;
	}

    const XMUINT3& fieldDim = XMUINT3(params.gridX, params.gridY, params.gridZ);

    // 必要なバッファ容量を確保
	EnsureBufferCapacity(fieldDim);

    // UAVの初期化（必要ならClear）
    UINT initCounts[4] = { 0, 0, 0, 0 };
    m_context->ClearUnorderedAccessViewUint(m_vertexBufferSet.uav, initCounts);
    m_context->ClearUnorderedAccessViewUint(m_indexBufferSet.uav, initCounts);

    // CSにバインド
    m_context->CSSetShader(m_marchingCubesCS.cs, nullptr, 0);
    m_shaderResourceBinder.BindShaderResource(ShaderStage::CS, SLOT_SRV_VOLUME_SCALAR_FIELD, scalarFieldSRV);
    ID3D11UnorderedAccessView* uavs[] = { m_vertexBufferSet.uav, m_indexBufferSet.uav, m_drawArgsBufferSet.uav };
    m_shaderResourceBinder.BindUnorderedAccessViews(SLOT_UAV_MARCHING_CUBES_VERTEX, 3, uavs);

    // Dispatch
    UINT groupX = (fieldDim.x - 1);
    UINT groupY = (fieldDim.y - 1);
    UINT groupZ = (fieldDim.z - 1);
    m_context->Dispatch(groupX, groupY, groupZ);

    // Unbind
    ID3D11UnorderedAccessView* nullUAV[3] = {};
    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_shaderResourceBinder.BindUnorderedAccessViews(SLOT_UAV_MARCHING_CUBES_VERTEX, 3, nullUAV);
    m_shaderResourceBinder.BindShaderResource(ShaderStage::CS, SLOT_SRV_VOLUME_SCALAR_FIELD, nullSRV);

    UpdateCBMarchingCubes();
}

void MarchingCubesSurfaceMeshGPU::Draw(void)
{
    // パイプラインのセットアップ
    SetupPipeline();

    m_context->DrawIndexedInstancedIndirect(m_drawArgsBufferSet.buffer, 0);
}

void MarchingCubesSurfaceMeshGPU::ComputeMeshRequirementsByFieldDim(const XMUINT3& fieldDim, UINT* outVertexCount, UINT* outIndexCount)
{
    const UINT cellCount = (fieldDim.x - 1) * (fieldDim.y - 1) * (fieldDim.z - 1);
    const UINT maxTrianglesPerCell = 5;

    *outVertexCount = cellCount * maxTrianglesPerCell * 3;
    *outIndexCount = *outVertexCount;
}

void MarchingCubesSurfaceMeshGPU::EnsureBufferCapacity(const XMUINT3& fieldDim)
{
    const float RESERVE_SCALE = 1.5f; // 冗長確保用スケーリング係数

	// 必要な頂点数とインデックス数を計算
    UINT requiredVertices, requiredIndices;
    ComputeMeshRequirementsByFieldDim(fieldDim, &requiredVertices, &requiredIndices);

    if (requiredVertices > m_vertexCapacity ||
        requiredIndices > m_indexCapacity)
    {
        // 必要な頂点数またはインデックス数が現在の容量を超える場合、バッファを再作成
        m_vertexCapacity = requiredVertices * RESERVE_SCALE;
        m_indexCapacity = requiredIndices * RESERVE_SCALE;
        CreateMeshBuffers();
    }
}

void MarchingCubesSurfaceMeshGPU::CreateMeshBuffers(void)
{
	ReleaseMeshBuffer();

    // 頂点バッファ（RWStructuredBuffer<MarchingCubesVertex>）
    // - Compute Shader が三角形頂点を出力するために書き込むバッファ
    // - Vertex Shader が SRV 経由で参照し、頂点位置・法線などを利用
    // - DrawIndexedInstancedIndirect では IA バインドせず、SRV 経由で頂点取得
    ParticleEffectRendererBase::CreateStructuredBuffer(m_device, 
		sizeof(MarchingCubesVertex),        // 頂点のサイズ
        m_vertexCapacity,                   // バッファ内に確保する最大頂点数（Dispatch 範囲に応じて拡張）
		nullptr,                            // 初期データはなし
		&m_vertexBufferSet.buffer,          // 実バッファ本体
		&m_vertexBufferSet.srv,             // ShaderResourceView：VS で頂点を読み取る用
		&m_vertexBufferSet.uav              // UnorderedAccessView：CS で頂点を書き込む用
    );

    // インデックスバッファ（RWStructuredBuffer<uint>）
    // - Compute Shader が三角形のインデックス情報（頂点番号）を書き込むバッファ
    // - VS 内で SV_VertexID を使ってこのインデックスを参照し、該当頂点にアクセス
    // - SRV 経由で読み込めるように、インデックスバッファにも SRV を生成しておく
    ParticleEffectRendererBase::CreateStructuredBuffer(m_device, 
		sizeof(UINT),                       // インデックスのサイズ
        m_indexCapacity,                    // 最大インデックス数（三角形数×3 を想定して確保）
		nullptr,                            // 初期データはなし
		&m_indexBufferSet.buffer,           // 実バッファ本体
		&m_indexBufferSet.srv,              // ShaderResourceView：VSでインデックス取得用
		&m_indexBufferSet.uav               // UnorderedAccessView：CSでインデックス書き込み用
    );
}

void MarchingCubesSurfaceMeshGPU::ReleaseMeshBuffer(void)
{
	SafeRelease(&m_vertexBufferSet.buffer);
	SafeRelease(&m_vertexBufferSet.srv);
	SafeRelease(&m_vertexBufferSet.uav);
	SafeRelease(&m_indexBufferSet.buffer);
	SafeRelease(&m_indexBufferSet.srv);
	SafeRelease(&m_indexBufferSet.uav);
}
