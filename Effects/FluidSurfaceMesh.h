#pragma once
//=============================================================================
//
// [FluidSurfaceMesh.h]
// Author : 
//
//=============================================================================
#include "Core/Graphics/VertexStructs.h"
#include "Effects/FluidHeightFieldBuilder.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define BASE_COLOR_DEFAULT          XMFLOAT4(0.0f, 0.5f, 1.0f, 1.0f) // 水面のベース色のデフォルト値
#define FRESHENEL_POWER_DEFAULT     5.0f        // フレネル効果のデフォルト強度
#define ALPHA_SCALE_DEFAULT         0.5f        // アルファスケールのデフォルト値
#define INITIAL_VERTEX_CAPACITY		4096								// 初期頂点バッファの容量
#define INITIAL_INDEX_CAPACITY		INITIAL_VERTEX_CAPACITY * 6			// 初期インデックスバッファの容量

//*********************************************************
// 構造体
//*********************************************************
struct CBFluidSurface
{
    XMFLOAT4 baseColor;   // 水面のベース色（+ α）
    float    fresnelPower;// フレネル効果の強さ
    float    alphaScale;  // アルファスケール
    XMFLOAT2 padding;     // パディング（16byteアライン）
};

class FluidSurfaceMesh
{
public:
    FluidSurfaceMesh();
	~FluidSurfaceMesh() { ReleaseBuffers(); SafeRelease(&m_cbFluidSurface); }

	void Initialize(void);

    // 高さフィールドからメッシュを生成
	void GenerateSurfaceMesh(const FluidHeightFieldBuilder& heightField);
    void Draw(void);

	// 頂点バッファとインデックスバッファを取得
    ID3D11Buffer* GetVertexBuffer(void) const { return m_vertexBuffer; }
    ID3D11Buffer* GetIndexBuffer(void) const { return m_indexBuffer; }
    UINT GetIndexCount(void) const { return static_cast<UINT>(m_indices.getSize()); }

	void SetBaseColor(const XMFLOAT4& color) { m_baseColor = color; }
	void SetFresnelPower(float power) { m_fresnelPower = power; }
	void SetAlphaScale(float scale) { m_alphaScale = scale; }

private:
	bool LoadShaders(void); // シェーダーの読み込み
	void CreateBuffers(void); // 頂点バッファとインデックスバッファの作成
	XMFLOAT3 ComputeNormal(const FluidHeightFieldBuilder& heightField, int gx, int gz); // 法線計算
    void UpdateBuffers(void); // GPUにアップロード
	void EnsureBufferCapacity(UINT requiredVertexCount, UINT requiredIndexCount); // バッファの容量を確保
	void UpdateCBFluidSurface(void); // 定数バッファの更新
	void ReleaseBuffers(void); // バッファを解放

	XMFLOAT4 m_baseColor = BASE_COLOR_DEFAULT; // 水面のベース色
	float m_fresnelPower = FRESHENEL_POWER_DEFAULT; // フレネル効果の強さ
	float m_alphaScale = ALPHA_SCALE_DEFAULT; // アルファスケール（透明度調整）

	ShaderSet m_shaderSet; // シェーダーセット

    SimpleArray<FluidSurfaceVertex> m_vertices;
    SimpleArray<UINT> m_indices;
    UINT m_vertexCapacity = INITIAL_VERTEX_CAPACITY;
    UINT m_indexCapacity = INITIAL_INDEX_CAPACITY;

    ID3D11Buffer* m_vertexBuffer = nullptr;
    ID3D11Buffer* m_indexBuffer = nullptr;
	ID3D11Buffer* m_cbFluidSurface = nullptr;
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_context = nullptr;

    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();
};
