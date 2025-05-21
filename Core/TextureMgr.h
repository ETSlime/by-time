#pragma once
//=============================================================================
//
// 【重複読込を回避し参照付きでテクスチャSRVを管理するクラス】 [TextureMgr.h]
// Author : 
// テクスチャの読み込み・キャッシュ・SRV生成を統括し、
// 使用回数に基づいたリリース制御により無駄なリソース消費を防止する。
//
//=============================================================================
#include "main.h"
#include "Core/Graphics/Renderer.h"
#include "Utility/HashMap.h"
#include "Utility/SingletonBase.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_SRV			222

//*********************************************************
// 構造体
//*********************************************************
struct SRV_POOL
{
	ID3D11ShaderResourceView* srv;
	unsigned int count;

	SRV_POOL() : srv(nullptr), count(0) {}
	~SRV_POOL()
	{
		Release();
	}

	SRV_POOL(const SRV_POOL& other) = delete;
	SRV_POOL& operator=(const SRV_POOL& other) = delete;

	SRV_POOL(SRV_POOL&& other) noexcept
		: srv(other.srv), count(other.count)
	{
		other.srv = nullptr;
		other.count = 0;
	}

	// move
	SRV_POOL& operator=(SRV_POOL&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			srv = other.srv;
			count = other.count;
			other.srv = nullptr;
			other.count = 0;
		}
		return *this;
	}

	void AddRef() { count++; }

	void Release()
	{
		if (srv && --count == 0)
		{
			srv->Release();
			srv = nullptr;
		}
	}
};

class TextureMgr : public SingletonBase<TextureMgr>
{
public:
	TextureMgr();
	~TextureMgr();

	void Init(ID3D11Device* device);
	void Uninit(void);

	ID3D11ShaderResourceView* CreateTexture(char* filename);

private:
	TextureMgr(const TextureMgr& rhs);
	TextureMgr& operator=(const TextureMgr& rhs) = default;

private:
	ID3D11Device* md3dDevice;
	static HashMap<char*, SRV_POOL*, CharPtrHash, CharPtrEquals> mTextureSRV;
	Renderer& renderer = Renderer::get_instance();
};