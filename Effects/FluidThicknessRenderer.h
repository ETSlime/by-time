#pragma once
//=============================================================================
//
//  [FluidThicknessRenderer.h]
// Author : 
//
//=============================================================================
#include "Effects/ParticleStructs.h"
#include "Core/Graphics/RenderTexture.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define THICKNESS_PER_PARTICLE_DEFAULT      1.0f // 1粒子あたりの厚みのデフォルト値

//*********************************************************
// 構造体
//*********************************************************
struct CBFluidThickness
{
	float thicknessPerParticle; // 流体の厚み（1粒子あたりの厚み）
	XMFLOAT2 screenSize; // スクリーンサイズ（幅, 高さ）
    float padding;
}; // 流体の厚み計算用定数バッファ

class FluidThicknessRenderer
{
public:
    FluidThicknessRenderer();
    ~FluidThicknessRenderer() { Release(); }

    void Initialize(int width, int height);
    void Release(void);
    void Resize(int newWidth, int newHeight);
    void UpdateCB(void);
	void Draw(UINT count);
    void ComputeParticleScreenRadius(float smoothingRadius);
    ID3D11ShaderResourceView* GetSRV(void) const { return m_thicknessRT.GetSRV(); }

private:
	RenderTexture m_thicknessRT; // 流体の厚みを格納するレンダーテクスチャ
    ComputeShaderSet m_shaderSet; // パーティクル厚み計算用のシェーダーセット

    int m_width = 0;
    int m_height = 0;
	float m_thicknessPerParticle = THICKNESS_PER_PARTICLE_DEFAULT; // 1粒子あたりの厚み（ピクセル単位）
    CBFluidThickness m_cbData{}; // 流体の厚み計算用定数バッファデータ
	ID3D11Buffer* m_cbFluidThickness = nullptr; // 流体の厚み計算用定数バッファ

    ID3D11Device* m_device = Renderer::get_instance().GetDevice();
    ID3D11DeviceContext* m_context = Renderer::get_instance().GetDeviceContext();
    ShaderResourceBinder& m_shaderResourceBinder = ShaderResourceBinder::get_instance();
};