//=============================================================================
//
// スカイボックス定義 [Skybox.h]
// Author : 
// - 昼夜の空を6面テクスチャで描くよ
// - blendFactor でスムーズな空の切り替えも実現
// - 専用のCBとSRVでGPUパイプラインにも優しい構成だよ
//
//=============================================================================
#pragma once
#include "Core/Graphics/Renderer.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	TEXTURE_SKYBOX_PATH		"data/TEXTURE/BrightSky.dds"

//*****************************************************************************
// 構造体定義
//*****************************************************************************
struct SkyBoxVertex 
{
    XMFLOAT3 position;
    XMFLOAT2 uv;
    int      faceIndex;
};

struct SkyBoxBuffer
{
    XMMATRIX view;
    XMMATRIX projection;
    float blendFactor;
};

class Skybox 
{
public:
    Skybox();
    Skybox::~Skybox();

    void Update(void);
    void Draw(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);

    static float GetCurrentDaytime(void) { return s_blendFactor; }

private:

    bool Initialize(void);
    void CreateCube();
    void LoadShaders();
    float AdjustBlendFactor(float time);

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;


    ShaderSet m_shaderSet;

    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_skyboxBuffer;
    ID3D11DepthStencilState* m_depthStencilState;

    ID3D11ShaderResourceView* m_skyboxDaySRVs[6];
    ID3D11ShaderResourceView* m_skyboxNightSRVs[6];

    ShaderResourceBinder& m_ShaderResourceBinder = ShaderResourceBinder::get_instance();

    float m_timeOfDay;
    bool m_dayToNight;
    static float s_blendFactor;
};