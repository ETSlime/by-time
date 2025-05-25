#pragma once
//=============================================================================
//
//  [OBJLoader.h]
// Author : 
//
//
//=============================================================================
#include "main.h"
#include "Utility/SingletonBase.h"
#include "Model/Model.h"

class OBJLoader : public SingletonBase<OBJLoader>
{
public:
	// モデル読込
	bool LoadObjModel(const char* FileName, StaticModelData* Model);
private:
	//マテリアル読み込み
	void LoadMaterial(char* FileName, MODEL_MATERIAL** MaterialArray, unsigned int* MaterialNum);
	// テクスチャ名読み込み
	void LoadTextureName(char* FileName, FILE* fp, MODEL_MATERIAL* Material, int mc, TextureType type);

	bool m_loadTangent = false;
};