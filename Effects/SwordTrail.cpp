//=============================================================================
//
// SwordTrail処理 [SwordTrail.cpp]
// Author : 
//
//=============================================================================

#include "Effects/SwordTrail.h"


SwordTrail::SwordTrail(const GameObject<SkinnedMeshModelInstance>* weapon) : weapon(weapon)
{
	vertexBuffer = nullptr;
	swordTrailTexture = nullptr;
	maxTrailFrames = 20;
	trailType = SwordTrailType::Default;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = sizeof(VFXVertex) * maxTrailFrames * 6;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Renderer::get_instance().GetDevice()->CreateBuffer(&bufferDesc, nullptr, &vertexBuffer);

}

SwordTrail::~SwordTrail()
{
	SafeRelease(&vertexBuffer);
	SafeRelease(&swordTrailTexture);
}


void SwordTrail::GetSwordTrailPoints(XMVECTOR& hiltWorld, XMVECTOR& tipWorld)
{
	if (!weapon) 
		return;

	BOUNDING_BOX swordAABB = weapon->GetSkinnedMeshModelConst()->GetBoundingBox();
	float hiltPosZ = swordAABB.minPoint.z;
	float tipPosZ = swordAABB.maxPoint.z;
	float swordPosY = (swordAABB.maxPoint.y + swordAABB.minPoint.y) * 0.5f;
	float sowrdPosX = (swordAABB.maxPoint.x + swordAABB.minPoint.x) * 0.5f;

	// ローカル座標系のヒルトと先端の座標を取得
	XMFLOAT3 hiltLocalPos = XMFLOAT3(sowrdPosX, swordPosY, hiltPosZ);
	XMFLOAT3 tipLocalPos = XMFLOAT3(sowrdPosX, swordPosY, tipPosZ);

	XMMATRIX swordWorldMatrix = weapon->GetWorldMatrix();

	hiltWorld = XMVector3Transform(XMLoadFloat3(&hiltLocalPos), swordWorldMatrix);
	tipWorld = XMVector3Transform(XMLoadFloat3(&tipLocalPos), swordWorldMatrix);
}

void SwordTrail::Update(void)
{
	XMVECTOR hiltWorld, tipWorld;
	GetSwordTrailPoints(hiltWorld, tipWorld);

	hiltTrail.push_back(hiltWorld);
	tipTrail.push_back(tipWorld);

	// トレイルの最初のフレームを削除
	if (static_cast<int>(hiltTrail.getSize()) > maxTrailFrames)
		hiltTrail.erase(0);
	if (static_cast<int>(tipTrail.getSize()) > maxTrailFrames)
		tipTrail.erase(0);

	GenerateSwordTrailMesh();
}

void SwordTrail::GenerateSwordTrailMesh(void)
{
	swordTrailVertices.clear();

	if (hiltTrail.getSize() < 2) return;

	// トレイルのメッシュを生成
	for (UINT i = 1; i < hiltTrail.getSize(); i++)
	{
		XMVECTOR hilt1 = hiltTrail[i - 1];
		XMVECTOR tip1 = tipTrail[i - 1];
		XMVECTOR hilt2 = hiltTrail[i];
		XMVECTOR tip2 = tipTrail[i];

		VFXVertex v1, v2, v3, v4;

		XMStoreFloat3(&v1.position, hilt1);
		XMStoreFloat3(&v2.position, tip1);
		XMStoreFloat3(&v3.position, hilt2);
		XMStoreFloat3(&v4.position, tip2);


		v1.color = XMFLOAT4(1, 1, 1, 1);
		v2.color = XMFLOAT4(1, 1, 1, 1);
		v3.color = XMFLOAT4(1, 1, 1, 1);
		v4.color = XMFLOAT4(1, 1, 1, 1);

		v1.uv = XMFLOAT2(0.0f, 0.0f);
		v2.uv = XMFLOAT2(1.0f, 0.0f);
		v3.uv = XMFLOAT2(0.0f, 1.0f);
		v4.uv = XMFLOAT2(1.0f, 1.0f);

		swordTrailVertices.push_back(v1);
		swordTrailVertices.push_back(v2);
		swordTrailVertices.push_back(v3);

		swordTrailVertices.push_back(v3);
		swordTrailVertices.push_back(v2);
		swordTrailVertices.push_back(v4);
	}
}

bool SwordTrail::LoadTrailTexture(char* path)
{
	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
		path,
		NULL,
		NULL,
		&swordTrailTexture,
		NULL);

	if (FAILED(hr))
		return false;

	return true;
}

bool SwordTrail::SetTrailType(SwordTrailType type)
{
	bool loadTexture = false;

	if (trailType == type) 
		return true;
	else
	{
		// テクスチャを解放
		SafeRelease(&swordTrailTexture);
		// トレイルタイプを設定
		trailType = type;
		// テクスチャを読み込む
		switch (trailType)
		{
		case SwordTrailType::Normal:
			loadTexture = LoadTrailTexture(SWORD_TRAIL_TEXTURE_PATH);
			break;
		case SwordTrailType::Flame:
			loadTexture = LoadTrailTexture(SWORD_TRAIL_TEXTURE_PATH);
			break;
		case SwordTrailType::Ice:
			loadTexture = LoadTrailTexture(SWORD_TRAIL_TEXTURE_PATH);
			break;
		case SwordTrailType::Lightning:
			loadTexture = LoadTrailTexture(SWORD_TRAIL_TEXTURE_PATH);
			break;
		}
	}

	return loadTexture;
}

void SwordTrail::SetTrailFrames(int Frames)
{
	maxTrailFrames = Frames;
}

void SwordTrail::Draw(void)
{
	if (swordTrailVertices.getSize() == 0)
		return;

	VFXVertex* trailVerticesArray = new VFXVertex[swordTrailVertices.getSize()];
	for (UINT i = 0; i < swordTrailVertices.getSize(); i++)
	{
		trailVerticesArray[i] = swordTrailVertices[i];
	}

	// バーテックスバッファを作成
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	Renderer::get_instance().GetDeviceContext()->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	memcpy(mappedResource.pData, trailVerticesArray, sizeof(VFXVertex) * swordTrailVertices.getSize());
	renderer.GetDeviceContext()->Unmap(vertexBuffer, 0);

	renderer.GetDeviceContext()->PSSetShaderResources(0, 1, &swordTrailTexture);

	// 描画
	UINT stride = sizeof(VFXVertex);
	UINT offset = 0;
	renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	renderer.GetDeviceContext()->Draw(swordTrailVertices.getSize(), 0);

	SAFE_DELETE_ARRAY(trailVerticesArray);
}