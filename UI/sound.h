//=============================================================================
//
// サウンド処理 [sound.h]
//
//=============================================================================
#pragma once

#include <windows.h>
#include "xaudio2.h"						// サウンド処理で必要

// BGMとSE用の音量変数
extern float g_BGMVolume;
extern float g_SEVolume;

extern int g_CurrentBGM;
//*****************************************************************************
// サウンドファイル
//*****************************************************************************
enum 
{
	SOUND_LABEL_BGM_spring,			// 春BGM
	SOUND_LABEL_BGM_summer,			// 夏BGM
	SOUND_LABEL_BGM_autumn,			// 秋BGM
	SOUND_LABEL_BGM_winter,			// 冬BGM
	SOUND_LABEL_SE_Battle_boom,		// 銃SE
	SOUND_LABEL_SE_Battle_sword,	// 剣SE

	SOUND_LABEL_MAX,
};

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
BOOL InitSound(HWND hWnd);
void UninitSound(void);
void PlaySound(int label);
void StopSound(int label);
void StopSound(void);
// BGM/SE 音量設定用の関数
void SetBGMVolume(float volume);
void SetSEVolume(float volume);
void IncreaseBGMVolume();
void DecreaseBGMVolume();
void IncreaseSEVolume();
void DecreaseSEVolume();

