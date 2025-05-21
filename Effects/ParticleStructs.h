//=============================================================================
//
// パーティクル内部構造体定義 [ParticleStructs.h]
// Author : 
// Compute Shaderで扱うパーティクル個別データを定義するヘッダにゃん
// Billboard系パーティクル用（通常 / Flipbook）に対応っ！ぷにぷに
//
//=============================================================================
#pragma once
#include "Effects/IEffectRenderer.h"

//*********************************************************
// 構造体
//*********************************************************

// --- BillboardSimple ---
struct BillboardSimpleParticle
{
    XMFLOAT3 position;
    XMFLOAT3 velocity;
    float size;
    float life;
    float lifeRemaining;
    float rotation;
    XMFLOAT4 color;
    XMFLOAT4 startColor;
    XMFLOAT4 endColor;
};

// --- BillboardFlipbook ---
struct BillboardFlipbookParticle
{
    XMFLOAT3 position;              // 現在位置
    XMFLOAT3 velocity;              // 速度
    float size;                     // サイズ
    float life;
    float lifeRemaining;            // 残り寿命（秒）
    float rotation;                 // 回転角（ラジアン）
    XMFLOAT4 color;
    XMFLOAT4 startColor;
    XMFLOAT4 endColor;
    float frameIndex;               // アニメーションの現在フレーム（float、小数対応）
    float frameSpeed;               // 1秒あたり再生速度（補間対応用
};