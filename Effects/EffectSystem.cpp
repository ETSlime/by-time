//=============================================================================
//
// EffectSystem処理 [EffectSystem.cpp]
// Author : 
//
//=============================================================================
#include "Effects/EffectSystem.h"
#include "SmokeEffectRenderer.h"
#include "FireBallEffectRenderer.h"

void EffectSystem::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device;
	m_context = context;

	// あらかじめリザーブしてメモリ確保（推定数値）
	m_allEffects.reserve(MAX_EFFECT_NUM);
	m_fireEffects.reserve(MAX_FIRE_EFFECT_NUM);
	m_smokeEffects.reserve(MAX_SMOKE_EFFECT_NUM);
	m_softBodyEffects.reserve(MAX_SOFTBODY_EFFECT_NUM);
}

void EffectSystem::Shutdown(void)
{
	for (auto& effect : m_allEffects)
	{
		effect->Shutdown();
		delete effect;
	}
}

void EffectSystem::Update(void)
{
	for (auto& effect : m_allEffects)
	{
		effect->Update();
	}
}

void EffectSystem::Draw(const XMMATRIX& viewProj)
{
	// 毎フレーム頭で分類クリア
	m_billboardSimpleEffects.clear();
	m_billboardFlipbookEffects.clear();
	m_softBodyEffects.clear();

	// 分類処理
	for (auto effect : m_allEffects)
	{
		EffectType type = effect->GetEffectType();
		if (IsParticleEffectType(type))
		{
			auto* particle = dynamic_cast<ParticleEffectRendererBase*>(effect);
			if (particle)
			{
				switch (particle->GetShaderGroupForEffect(type))
				{
				case ParticleShaderGroup::BillboardSimple:
					m_billboardSimpleEffects.push_back(particle);
					break;
				case ParticleShaderGroup::BillboardFlipbook:
					m_billboardFlipbookEffects.push_back(particle);
					break;
				default:
					break;
				}
			}
		}
		else
		{
			switch (effect->GetEffectType())
			{
			case EffectType::SoftBody:
				m_softBodyEffects.push_back(effect);
				break;
			default:
				break;
			}
		}

	}

	// BillboardSimple
	if (!m_billboardSimpleEffects.empty())
	{
		ParticleEffectRendererBase::ResetPipelineState(ParticleShaderGroup::BillboardSimple);
		for (auto* effect : m_billboardSimpleEffects)
		{
			effect->SetupPipeline();
			effect->Draw(viewProj);
		}
	}

	// BillboardFlipbook
	if (!m_billboardFlipbookEffects.empty())
	{
		ParticleEffectRendererBase::ResetPipelineState(ParticleShaderGroup::BillboardFlipbook);
		for (auto* effect : m_billboardFlipbookEffects)
		{
			effect->SetupPipeline();
			effect->Draw(viewProj);
		}
	}

	EffectType lastType = EffectType::None; // 最初のダミー値

	// ソフトボディフェークト描画
	if (!m_softBodyEffects.empty())
	{
		if (lastType != EffectType::SoftBody)
		{
			m_context->GSSetShader(nullptr, nullptr, 0);
			//SoftBodyRenderer::ResetPipelineState(); // クラス単位でリセット
		}

		for (auto softBody : m_softBodyEffects)
		{
			softBody->SetupPipeline();
			softBody->Draw(viewProj);
		}

		lastType = EffectType::SoftBody;
	}	

	// 最後にパイプライン状態をリセット
	ClearAllEffectBindings();
}

ParticleEffectRendererBase* EffectSystem::SpawnParticleEffect(ParticleEffectParams& params)
{
	if (m_allEffects.getSize() >= MAX_EFFECT_NUM)
		return nullptr; // 最大数に達している場合は生成しない

	// パーティクル数の制限
	if (params.numParticles > MAX_PARTICLES)
		params.numParticles = MAX_PARTICLES; // 最大数に制限
	else if (params.numParticles < 1)
		params.numParticles = 1; // 最小数に制限

	ParticleEffectRendererBase* effect = nullptr;

	// タイプに応じてインスタンス生成
	switch (params.type)
	{
	case EffectType::Fire:
		//effect = new FireEffectRenderer();
		break;
	case EffectType::Smoke:
		effect = new SmokeEffectRenderer();
		break;
	case EffectType::FireBall:
		effect = new FireBallEffectRenderer();
		break;
	default:
		return nullptr; // 未対応タイプ
	}

	if (!effect)
		return nullptr;

	effect->ConfigureEffect(params); // 初期化
	if (effect->Initialize(m_device, m_context))
	{
		m_allEffects.push_back(effect);
		return effect;
	}
	else
	{
		delete effect;
		return nullptr;
	}
}

void EffectSystem::RemoveEffect(IEffectRenderer* effect)
{
	int index = m_allEffects.find_index(effect);
	if (index >= 0)
	{
		m_allEffects[index]->Shutdown();
		delete m_allEffects[index];
		m_allEffects.erase(index);
	}
}

void EffectSystem::ClearAllEffectBindings(void)
{
	ID3D11ShaderResourceView* nullSRV = nullptr;

	m_ShaderResourceBinder.BindShaderResource(ShaderStage::VS, SLOT_SRV_PARTICLE, nullSRV);
	m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, nullSRV);
	m_context->GSSetShader(nullptr, nullptr, 0);
}
