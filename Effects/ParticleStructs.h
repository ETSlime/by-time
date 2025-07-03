#pragma once
//=============================================================================
//
// パーティクル内部構造体定義 [ParticleStructs.h]
// Author : 
// Compute Shaderで扱うパーティクル個別データを定義するヘッダにゃん
// Billboard系パーティクル用（通常 / Flipbook）に対応っ！ぷにぷに
//
//=============================================================================
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

// 水体シミュレーション用の粒子構造体
// Position Based Fluids (PBF) に基づいて、物理的な水挙動を表現する
struct WaterFluidParticle
{
	UINT     particleID;           // 粒子ID（0から始まるインデックス）
    XMFLOAT3 position;             // 現在位置
	XMFLOAT3 prevPosition;         // 前フレーム位置
    float    density;              // 密度（周囲の粒子から算出）

    XMFLOAT3 velocity;             // 速度
    float    lambda;               // 拘束条件解（位置補正のための係数）
	float    pressure;             // 圧力（密度から算出される）

    XMFLOAT3 predictedPosition;    // 予測位置（次の位置、拘束前）
    float    lifeRemaining;        // 寿命（将来拡張用、今は1で固定でも可）

    XMFLOAT4 color;                // デバッグ用カラー（密度に応じた可視化など）

    float size;                    // サイズ


    UINT debug_mortonIndex = 0;     
    XMUINT2 debug_mortonIndex64 = XMUINT2(0, 0);
    UINT debug_voxelX = 0;              
    UINT debug_voxelY = 0;
    UINT debug_voxelZ = 0;
    UINT debug_neighborX = 0;
    UINT debug_neighborY = 0;
    UINT debug_neighborZ = 0;
	UINT debug_neighborID = 0;          // 近傍粒子のID（デバッグ用）
    int debug_foundTripletIndex = 0;    
    UINT debug_triCheckCount = 0;       
    UINT debug_hit = 0;                 
    UINT debug_pageHeaderIndex = 0;
    XMFLOAT3 debug_position = { 0.0f, 0.0f, 0.0f };
    float debug_dirLength = 0;
    XMFLOAT3 debug_triV0 = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_triV1 = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_triV2 = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_rayStart = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_rayDir = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_hitPoint = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 debug_pressureForce = { 0.0f, 0.0f, 0.0f }; 
	XMFLOAT3 debug_viscosityForce = { 0.0f, 0.0f, 0.0f };
    XMFLOAT3 debug_gridCoord = { 0.0f, 0.0f, 0.0f };
	UINT     debug_left = 123456;              // デバッグ用：左側の粒子インデックス
	UINT     debug_right = 123456;             // デバッグ用：右側の粒子インデックス
	float    debug_w = 0.0f;                // デバッグ用：重み（W）値
	float    debug_dist = 0.0f;                // デバッグ用：距離（D）値
    UINT     debug_cellCount = 0;              // デバッグ用：セル内の粒子数
	UINT     debug_neighborCount = 0;          // デバッグ用：近傍粒子数
	UINT     debug_allParticleCount = 0;       // デバッグ用：全粒子数
	UINT     debug_neighborIDcount = 0;          // デバッグ用：近傍粒子IDの数
	UINT     debug_distCheckCount = 0;          // デバッグ用：距離チェック回数
    XMFLOAT3 debug_accel = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 debug_rawVelocity = { 0.0f, 0.0f, 0.0f }; // デバッグ用：速度
	XMFLOAT3 padding;              // パディング（16バイト境界調整用）
};