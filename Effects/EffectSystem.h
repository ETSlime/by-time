//=============================================================================
//
// パーティクルエフェクトの生成・管理・描画を統括する演出管理クラス [EffectSystem.h]
// Author : 
// 各種エフェクト（火炎・煙・ソフトボディ等）をパラメータ指定で生成し、
// ランタイムでの更新／描画制御・一括管理・リソース解放を行う
//
//=============================================================================
#pragma once
#include "ParticleEffectRendererBase.h"
#include "Utility/SingletonBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_EFFECT_NUM 512 // 最大エフェークト数
#define MAX_FIRE_EFFECT_NUM 256 // 最大ファイヤーフェークト数
#define MAX_SMOKE_EFFECT_NUM 128 // 最大スモークフェークト数
#define MAX_SOFTBODY_EFFECT_NUM 64 // 最大ソフトボディフェークト数

//*********************************************************
// 構造体
//*********************************************************
struct ParticleEffectParams
{
    EffectType type = EffectType::None;
    XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f);
    float scale = 1.0f;
    float lifeMin = 1.0f;
    float lifeMax = 2.0f;
    float spawnRateMin = 5.0f;
    float spawnRateMax = 15.0f;
    XMFLOAT3 acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
    UINT numParticles = MAX_PARTICLES;
    XMFLOAT4 startColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT4 endColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

    virtual ~ParticleEffectParams() = default; // RTTI、dynamic_cast のため
};

struct FireBallEffectParams : public ParticleEffectParams
{
    UINT tilesX = 7;
    UINT tilesY = 7;
    float coneAngleDegree = 25.0f;
    float coneRadius = 0.6f;
    float coneLength = 5.0f;
    float frameLerpCurve = 1.0f;
    float rotationSpeed = 0.5f;

	float startSpeedMin = 0.0f;
	float startSpeedMax = 0.0f;
};

class EffectSystem : public SingletonBase<EffectSystem>
{
public:
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown(void);
    void Update(void);
    void Draw(const XMMATRIX& viewProj);

    inline bool IsParticleEffectType(EffectType type)
    {
        return (type > EffectType::Particle_Start && type < EffectType::Particle_End);
    }

    // エフェクト生成
    ParticleEffectRendererBase* SpawnParticleEffect(ParticleEffectParams& params);

    // 汎用特效削除
    void RemoveEffect(IEffectRenderer* effect);
    // リソースの解除
    void ClearAllEffectBindings(void);

private:

    SimpleArray<IEffectRenderer*> m_allEffects;       // 全特效リスト
    SimpleArray<IEffectRenderer*> m_billboardSimpleEffects;
    SimpleArray<IEffectRenderer*> m_billboardFlipbookEffects;
    SimpleArray<IEffectRenderer*> m_fireEffects;      // ファイヤー
    SimpleArray<IEffectRenderer*> m_smokeEffects;     // スモーク
    SimpleArray<IEffectRenderer*> m_softBodyEffects;  // ソフトボディ

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    ShaderResourceBinder& m_ShaderResourceBinder = ShaderResourceBinder::get_instance();
};