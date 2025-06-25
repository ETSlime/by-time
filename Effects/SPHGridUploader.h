#pragma once
//=============================================================================
//
//  [SPHGridUploader.h]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Effects/ParticleStructs.h"
#include "Effects/ParticleEffectRendererBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_SPH_GRID_NUM            2500000
#define MAX_SPH_GRID_PAGE_SIZE      1024 // 1ページあたりの最大セル数

//*****************************************************************************
// 構造体定義
//*****************************************************************************

// Morton 置換用のグリッドページヘッダ構造体
struct SPHGridPageHeader
{
    UINT morton;        // Morton 置換されたGrid座標
    UINT count;         // そのセルに含まれる粒子数
    UINT startIndex;    // g_GridParticleIndices[] 中の開始インデックス
	UINT padding;       // 16バイト境界に合わせるためのパディング
};

class SPHGridUploader
{
public:
    void Initialize(float cellSize, UINT maxParticles);
	void Shutdown(void);
    void Clear(void);
    void BuildGrid(const SimpleArray<WaterFluidParticle>& particles);
    void UploadAndBindPageBuffers(void);
    void UploadSortedIndicesTo(ID3D11Buffer* dstBuffer) const;

    UINT GetTotalGridPageHeaderSize(void) const { return m_gridPageHeaders.getSize(); }
    const SimpleArray<UINT>& GetGridSortedIndices(void) const { return m_gridSortedIndices; }
    const SimpleArray<SPHGridPageHeader>& GetPageHeaders(void) const { return m_gridPageHeaders; }

private:
    float m_cellSize = 1.0f;
    XMFLOAT3 m_gridMin = { 0, 0, 0 };

    SimpleArray<SPHGridPageHeader> m_gridPageHeaders;
    SimpleArray<UINT> m_gridSortedIndices;

	ParticleBufferSet m_gridPageBufferSet;

    ID3D11Device* m_device = Renderer::get_instance().GetDevice();
    ID3D11DeviceContext* m_context = Renderer::get_instance().GetDeviceContext();
    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();

    // morton キー → particleIDs
    HashMap<uint32_t, SimpleArray<uint32_t>, HashUInt64, EqualUInt64> m_mortonMap =
        HashMap<uint32_t, SimpleArray<uint32_t>, HashUInt64, EqualUInt64>(
            MAX_SPH_GRID_NUM, // 初期バケット数
            HashUInt64(), // ハッシュ関数
            EqualUInt64() // 等価比較関数
        );
};
