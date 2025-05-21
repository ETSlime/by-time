#include "Core/TextureMgr.h"

//*****************************************************************************
// ƒOƒ[ƒoƒ‹•Ï”
//*****************************************************************************
HashMap<char*, SRV_POOL*, CharPtrHash, CharPtrEquals> TextureMgr::mTextureSRV(
	MAX_SRV,
	CharPtrHash(),
	CharPtrEquals());

TextureMgr::TextureMgr()
{
	Init(renderer.GetDevice());
}

TextureMgr::~TextureMgr()
{

}



void TextureMgr::Init(ID3D11Device* device)
{
	md3dDevice = device;
}

void TextureMgr::Uninit()
{
	for (auto& srv : mTextureSRV)
	{
		if (srv.value && srv.value->srv)
		{
			srv.value->srv->Release();
			delete srv.value;
			srv.value = nullptr;
		}
			
	}

	mTextureSRV.clear();
}

ID3D11ShaderResourceView* TextureMgr::CreateTexture(char* filename)
{
	if (filename == nullptr)
		return nullptr;

	SRV_POOL** ppSrvPool = mTextureSRV.search(filename);
	
	if (ppSrvPool == nullptr)
	{
		SRV_POOL* pSrvPool = new SRV_POOL();
		ppSrvPool = &pSrvPool;
		D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
			filename,
			NULL,
			NULL,
			&pSrvPool->srv,
			NULL);
		mTextureSRV.insert(filename, *ppSrvPool);
	}

	return (*ppSrvPool)->srv;
}


