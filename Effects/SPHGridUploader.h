#pragma once
//=============================================================================
//
//  [SPHGridUploader.h]
// Author : 
//
//=============================================================================
#include "main.h"
#include "Collision/VoxelBufferUploader.h"
#include "Effects/ParticleStructs.h"
#include "Effects/ParticleEffectRendererBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_SPH_GRID_PAGE_SIZE      1024 // 1ページあたりの最大セル数

//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct SPHIndexTriplet
{
    UINT morton_low;    // Morton の下位32bit
    UINT morton_high;   // Morton の上位32bit
	UINT startOffset;   // g_GridSortedIndices[] の開始位置
	UINT count;         // そのセルに含まれる粒子数
    UINT padding;       // 16バイト境界に合わせるためのパディング
};

// 一時トリプレットをハッシュ表から収集
struct TempTriplet
{
	XMUINT2 morton; // モートンコード（2ビットずつ分割された20ビット）
    UINT count;
    const SimpleArray<UINT>* indices; // 粒子IDリストへのポインタ
};

// 64ビット版のIndexTripletPageHeader（モートンコードを64ビットで扱う）
struct IndexTripletPageHeader64
{
    UINT64 pageStartMorton_Low;  // uint2.x
    UINT64 pageStartMorton_High; // uint2.y
    UINT64 pageEndMorton_Low;    // uint2.x
    UINT64 pageEndMorton_High;   // uint2.y
    UINT pageOffset;
    UINT pageCount;
    UINT padding[2]; // パディングで16バイト境界に
};



class SPHGridUploader
{
public:
    void Initialize(float cellSize, UINT maxParticles);
	void Shutdown(void);
    void Clear(void);
    void BuildGrid(const SimpleArray<WaterFluidParticle>& particles);
    void UploadAndBindGridBuffers(void);
    void UploadSortedIndicesTo(ID3D11Buffer* dstBuffer) const;

    UINT GetTotalGridPageHeaderSize(void) const { return m_indexTripletPageHeaders.getSize(); }
    const SimpleArray<UINT>& GetGridSortedIndices(void) const { return m_gridSortedIndices; }
    const SimpleArray<IndexTripletPageHeader64>& GetPageHeaders(void) const { return m_indexTripletPageHeaders; }

private:
    // IndexTripletをページ単位で構築
	void BuildPagedIndexTripletBuffer(void);
    // MergeSort による IndexTriplet ソート（mortonIndex 昇順）
    void MergeSortTriplets(SimpleArray<TempTriplet>& triplets, int left, int right);
	// モートンコードの比較関数（64ビット版）
    inline bool CompareMorton64(const XMUINT2& a, const XMUINT2& b)
    {
        return (a.y < b.y) || (a.y == b.y && a.x < b.x); // 上位優先
    }

    float m_cellSize = 1.0f;
    XMFLOAT3 m_gridMin = { 0, 0, 0 };

	SimpleArray<UINT> m_gridSortedIndices; // ソートされた粒子インデックスの配列
	SimpleArray<SPHIndexTriplet> m_sphIndexTriplets; // モートンキーとパーティクルインデックスの対応付け
    SimpleArray<IndexTripletPageHeader64> m_indexTripletPageHeaders; // IndexTripletのページヘッダ

	UINT m_indexTripletCapacity = 0; // IndexTripletの最大容量
	ParticleBufferSet m_gridPageBufferSet; // グリッドページヘッダ用バッファセット
	ParticleBufferSet m_indexTripletBufferSet; // IndexTriplet用バッファセット

    ID3D11Device* m_device = Renderer::get_instance().GetDevice();
    ID3D11DeviceContext* m_context = Renderer::get_instance().GetDeviceContext();
    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();

    // morton キー → particleIDs
    HashMap<uint64_t, SimpleArray<uint32_t>, HashUInt64, EqualUInt64> m_mortonMap =
        HashMap<uint64_t, SimpleArray<uint32_t>, HashUInt64, EqualUInt64>(
            MAX_PARTICLES, // 初期バケット数
            HashUInt64(), // ハッシュ関数
            EqualUInt64() // 等価比較関数
        );
};
