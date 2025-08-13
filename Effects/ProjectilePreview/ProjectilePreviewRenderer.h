#pragma once
//=============================================================================
//
// [ProjectilePreviewRenderer.h]
// Author : 
//
//=============================================================================
#include "Effects/ProjectilePreview/ProjectilePredictor.h"

class ProjectilePreviewRenderer 
{
public:
    ProjectilePreviewRenderer();
    ~ProjectilePreviewRenderer();

    void Initialize(void);
    void ShutDown(void);

    void Update(const SimpleArray<TrajectoryPoint>& trajectoryPoints, const XMFLOAT3& hitPos, bool hasHit);

    void Draw(void);

private:
    void CreateSphereMesh(UINT sliceCount, UINT stackCount);
    bool LoadShaders(void);

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;

    // 軌跡線用の頂点バッファ
    ID3D11Buffer* m_trajectoryVB;
    UINT m_maxTrajectoryPoints;
    UINT m_currentTrajectoryPoints;

    // 半球メッシュ用バッファ
    ID3D11Buffer* m_sphereVB;
    ID3D11Buffer* m_sphereIB;
    UINT m_sphereIndexCount;

    ShaderSet m_shaderSet;

    // 描画状態
    XMFLOAT3 m_hitPos;
    bool m_hasHit;
    float m_breathAnimTime; // 半球呼吸アニメーション用
};
