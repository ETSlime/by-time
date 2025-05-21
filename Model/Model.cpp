//=============================================================================
//
// モデルの処理 [Model.cpp]
// Author : 
//
//=============================================================================
#define _CRT_SECURE_NO_WARNINGS
#include "main.h"
#include "Model/Model.h"
#include "Core/Camera.h"
#include "Utility/InputManager.h"
#include "Utility/Debugproc.h"
#include "Model/ModelCacheLoader.h"
#include "Collision/CollisionManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define	VALUE_MOVE_MODEL	(0.50f)					// 移動速度
#define	RATE_MOVE_MODEL		(0.20f)					// 移動慣性係数
#define	VALUE_ROTATE_MODEL	(XM_PI * 0.05f)			// 回転速度
#define	RATE_ROTATE_MODEL	(0.20f)					// 回転慣性係数
#define	SCALE_MODEL			(10.0f)					// 回転慣性係数
#define MAX_MODEL_NUM		(30)

//*****************************************************************************
// グローバル変数
//*****************************************************************************
HashMap<char*, MODEL_POOL, CharPtrHash, CharPtrEquals> Model::modelHashMap(
	MAX_MODEL_NUM,
	CharPtrHash(),
	CharPtrEquals()
);

//=============================================================================
// 初期化処理
//=============================================================================
Model::Model(char *FileName)
{
	modelData = new MODEL_DATA();

	if (!ModelCacheLoader::LoadFromCache(FileName, modelData))
	{
		LoadObj(FileName, modelData);
		ModelCacheLoader::SaveToCache(FileName, modelData);
	}

	// 頂点バッファ生成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof( VERTEX_3D ) * modelData->VertexNum;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory( &sd, sizeof(sd) );
		sd.pSysMem = modelData->VertexArray;

		renderer.GetDevice()->CreateBuffer( &bd, &sd, &this->VertexBuffer );


		bd.ByteWidth = sizeof(VERTEX_3D) * 24;
		renderer.GetDevice()->CreateBuffer(&bd, NULL, &this->BBVertexBuffer);

		CreateBoundingBoxVertex();
	}

	// インデックスバッファ生成
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( unsigned int ) * modelData->IndexNum;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory( &sd, sizeof(sd) );
		sd.pSysMem = modelData->IndexArray;

		renderer.GetDevice()->CreateBuffer( &bd, &sd, &this->IndexBuffer );
	}

	// サブセット設定
	{
		for( unsigned int i = 0; i < modelData->SubsetNum; i++ )
		{
			modelData->SubsetArray[i].StartIndex = modelData->SubsetArray[i].StartIndex;
			modelData->SubsetArray[i].IndexNum = modelData->SubsetArray[i].IndexNum;


			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.DiffuseTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].diffuseTexture,
				NULL);

			if (modelData->SubsetArray[i].diffuseTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.noTexSampling = 1;

			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.NormalTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].normalTexture,
				NULL);

			if (modelData->SubsetArray[i].normalTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.normalMapSampling = 0;

			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.BumpTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].bumpTexture,
				NULL);

			if (modelData->SubsetArray[i].bumpTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.bumpMapSampling = 0;

			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.OpacityTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].opacityTexture,
				NULL);

			if (modelData->SubsetArray[i].opacityTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.opacityMapSampling = 0;

			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.ReflectTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].reflectTexture,
				NULL);

			if (modelData->SubsetArray[i].reflectTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.reflectMapSampling = 0;

			D3DX11CreateShaderResourceViewFromFile(renderer.GetDevice(),
				modelData->SubsetArray[i].Material.TranslucencyTextureName,
				NULL,
				NULL,
				&modelData->SubsetArray[i].translucencyTexture,
				NULL);

			if (modelData->SubsetArray[i].translucencyTexture == NULL)
				modelData->SubsetArray[i].Material.MaterialData.translucencyMapSampling = 0;
		}
	}
}


//=============================================================================
// 終了処理
//=============================================================================
Model::~Model()
{

	if( this->VertexBuffer )		this->VertexBuffer->Release();
	if( this->IndexBuffer )		this->IndexBuffer->Release();
	if (this->BBVertexBuffer)	this->BBVertexBuffer->Release();
	if (this->modelData)		delete modelData;

}


//=============================================================================
// 描画処理
//=============================================================================
void Model::DrawModel()
{
	// 頂点バッファ設定
	UINT stride = sizeof( VERTEX_3D );
	UINT offset = 0;
	renderer.GetDeviceContext()->IASetVertexBuffers( 0, 1, &this->VertexBuffer, &stride, &offset );

	// インデックスバッファ設定
	renderer.GetDeviceContext()->IASetIndexBuffer( this->IndexBuffer, DXGI_FORMAT_R32_UINT, 0 );

	// プリミティブトポロジ設定
	renderer.GetDeviceContext()->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	for( unsigned int i = 0; i < modelData->SubsetNum; i++ )
	{
		// マテリアル設定
		if (modelData->SubsetArray[i].Material.MaterialData.LoadMaterial)
			renderer.SetMaterial( modelData->SubsetArray[i].Material.MaterialData);

		// テクスチャ設定
		if (modelData->SubsetArray[i].Material.MaterialData.noTexSampling == 0)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, modelData->SubsetArray[i].diffuseTexture);
		}
		if (modelData->SubsetArray[i].Material.MaterialData.normalMapSampling == 1)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_NORMAL, modelData->SubsetArray[i].normalTexture);
		}
		if (modelData->SubsetArray[i].Material.MaterialData.bumpMapSampling == 1)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_BUMP, modelData->SubsetArray[i].bumpTexture);
		}
		if (modelData->SubsetArray[i].Material.MaterialData.opacityMapSampling == 1)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_OPACITY, modelData->SubsetArray[i].opacityTexture);
		}
		if (modelData->SubsetArray[i].Material.MaterialData.reflectMapSampling == 1)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_REFLECT, modelData->SubsetArray[i].reflectTexture);
		}
		if (modelData->SubsetArray[i].Material.MaterialData.translucencyMapSampling == 1)
		{
			m_ShaderResourceBinder.BindShaderResource(ShaderStage::PS, SLOT_TEX_TRANSLUCENCY, modelData->SubsetArray[i].translucencyTexture);
		}

		// ポリゴン描画
		renderer.GetDeviceContext()->DrawIndexed( modelData->SubsetArray[i].IndexNum, modelData->SubsetArray[i].StartIndex, 0 );
	}

#ifdef _DEBUG
	if (drawBoundingBox)
		DrawBoundingBox();
#endif
}

void Model::DrawBoundingBox()
{
	renderer.SetFillMode(D3D11_FILL_WIREFRAME);
	// 頂点バッファ設定
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, &this->BBVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	MATERIAL material;
	ZeroMemory(&material, sizeof(material));
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.noTexSampling = TRUE;
	renderer.SetMaterial(material);

	renderer.GetDeviceContext()->Draw(24, 0);
	renderer.SetFillMode(D3D11_FILL_SOLID);
}



//モデル読込
void Model::LoadObj( char *FileName, MODEL_DATA* Model )
{

	XMFLOAT3	*positionArray;
	XMFLOAT3	*normalArray;
	XMFLOAT2	*texcoordArray;

	unsigned int	positionNum = 0;
	unsigned int	normalNum = 0;
	unsigned int	texcoordNum = 0;
	unsigned int	vertexNum = 0;
	unsigned int	indexNum = 0;
	unsigned int	in = 0;
	unsigned int	subsetNum = 0;

	MODEL_MATERIAL	*materialArray = NULL;
	unsigned int	materialNum = 0;

	char str[256];
	char *s;
	char c;


	FILE *file;
	file = fopen( FileName, "rt" );
	if( file == NULL )
	{
		printf( "エラー:LoadModel %s \n", FileName );
		return;
	}



	//要素数カウント
	while( TRUE )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;

		if( strcmp( str, "v" ) == 0 )
		{
			positionNum++;
		}
		else if( strcmp( str, "vn" ) == 0 )
		{
			normalNum++;
		}
		else if( strcmp( str, "vt" ) == 0 )
		{
			texcoordNum++;
		}
		else if( strcmp( str, "usemtl" ) == 0 )
		{
			subsetNum++;
		}
		else if( strcmp( str, "f" ) == 0 )
		{
			in = 0;

			do
			{
				fscanf( file, "%s", str );
				vertexNum++;
				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c!= '\r' );

			//四角は三角に分割
			if( in == 4 )
				in = 6;

			indexNum += in;
		}
	}


	//メモリ確保
	positionArray = new XMFLOAT3[ positionNum ];
	normalArray   = new XMFLOAT3[ normalNum ];
	texcoordArray = new XMFLOAT2[ texcoordNum ];


	Model->VertexArray = new VERTEX_3D[ vertexNum ];
	Model->VertexNum = vertexNum;

	Model->IndexArray = new unsigned int[ indexNum ];
	Model->IndexNum = indexNum;

	Model->SubsetArray = new SUBSET[ subsetNum ];
	Model->SubsetNum = subsetNum;


	Model->boundingBox.minPoint = XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
	Model->boundingBox.maxPoint = XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);


	//要素読込
	XMFLOAT3 *position = positionArray;
	XMFLOAT3 *normal = normalArray;
	XMFLOAT2 *texcoord = texcoordArray;

	unsigned int vc = 0;
	unsigned int ic = 0;
	unsigned int sc = 0;


	fseek( file, 0, SEEK_SET );

	while( TRUE )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;

		if( strcmp( str, "mtllib" ) == 0 )
		{
			//マテリアルファイル
			fscanf( file, "%s", str );

			char path[256];
		//	strcpy( path, "data/model/" );

			//----------------------------------- フォルダー対応
			strcpy(path, FileName);
			char* adr = path;
			char* ans = adr;
			while (1)
			{
				adr = strstr(adr, "/");
				if (adr == NULL) break;
				else ans = adr;
				adr++;
			}
			if (path != ans) ans++;
			*ans = 0;
			//-----------------------------------

			strcat( path, str );

			LoadMaterial( path, &materialArray, &materialNum );
		}
		else if( strcmp( str, "o" ) == 0 )
		{
			//オブジェクト名
			fscanf( file, "%s", str );
		}
		else if( strcmp( str, "v" ) == 0 )
		{
			//頂点座標
			fscanf( file, "%f", &position->x );
			fscanf( file, "%f", &position->y );
			fscanf( file, "%f", &position->z );
			position->x *= SCALE_MODEL;
			position->y *= SCALE_MODEL;
			position->z *= SCALE_MODEL;

			// Update bounding box
			Model->boundingBox.minPoint.x = min(Model->boundingBox.minPoint.x, position->x);
			Model->boundingBox.minPoint.y = min(Model->boundingBox.minPoint.y, position->y);
			Model->boundingBox.minPoint.z = min(Model->boundingBox.minPoint.z, position->z);

			Model->boundingBox.maxPoint.x = max(Model->boundingBox.maxPoint.x, position->x);
			Model->boundingBox.maxPoint.y = max(Model->boundingBox.maxPoint.y, position->y);
			Model->boundingBox.maxPoint.z = max(Model->boundingBox.maxPoint.z, position->z);


			position++;
		}
		else if( strcmp( str, "vn" ) == 0 )
		{
			//法線
			fscanf( file, "%f", &normal->x );
			fscanf( file, "%f", &normal->y );
			fscanf( file, "%f", &normal->z );
			normal++;
		}
		else if( strcmp( str, "vt" ) == 0 )
		{
			//テクスチャ座標
			fscanf( file, "%f", &texcoord->x );
			fscanf( file, "%f", &texcoord->y );
			texcoord->y = 1.0f - texcoord->y;
			texcoord++;
		}
		else if( strcmp( str, "usemtl" ) == 0 )
		{
			//マテリアル
			fscanf( file, "%s", str );

			if( sc != 0 )
				Model->SubsetArray[ sc - 1 ].IndexNum = ic - Model->SubsetArray[ sc - 1 ].StartIndex;

			Model->SubsetArray[ sc ].StartIndex = ic;


			for( unsigned int i = 0; i < materialNum; i++ )
			{
				if( strcmp( str, materialArray[i].Name ) == 0 )
				{
					Model->SubsetArray[ sc ].Material.MaterialData = materialArray[i].MaterialData;
					Model->SubsetArray[sc].Material.MaterialData.LoadMaterial = TRUE;
					strcpy(Model->SubsetArray[ sc ].Material.DiffuseTextureName, materialArray[i].DiffuseTextureName);
					strcpy(Model->SubsetArray[sc].Material.NormalTextureName, materialArray[i].NormalTextureName);
					strcpy(Model->SubsetArray[sc].Material.BumpTextureName, materialArray[i].BumpTextureName);
					strcpy(Model->SubsetArray[sc].Material.OpacityTextureName, materialArray[i].OpacityTextureName);
					strcpy(Model->SubsetArray[sc].Material.ReflectTextureName, materialArray[i].ReflectTextureName);
					strcpy(Model->SubsetArray[sc].Material.TranslucencyTextureName, materialArray[i].TranslucencyTextureName);
					strcpy(Model->SubsetArray[ sc ].Material.Name, materialArray[i].Name );

					break;
				}
			}

			sc++;
			
		}
		else if( strcmp( str, "f" ) == 0 )
		{
			//面
			in = 0;

			do
			{
				fscanf( file, "%s", str );

				s = strtok( str, "/" );	
				Model->VertexArray[vc].Position = positionArray[ atoi( s ) - 1 ];
				if( s[ strlen( s ) + 1 ] != '/' )
				{
					//テクスチャ座標が存在しない場合もある
					s = strtok( NULL, "/" );
					Model->VertexArray[vc].TexCoord = texcoordArray[ atoi( s ) - 1 ];
				}
				s = strtok( NULL, "/" );	
				Model->VertexArray[vc].Normal = normalArray[ atoi( s ) - 1 ];

				Model->VertexArray[vc].Diffuse = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );

				Model->IndexArray[ic] = vc;
				ic++;
				vc++;

				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c != '\r' );

			//四角は三角に分割
			if( in == 4 )
			{
				Model->IndexArray[ic] = vc - 4;
				ic++;
				Model->IndexArray[ic] = vc - 2;
				ic++;
			}
		}
	}

	if (loadTangent == false)
	{
		for (unsigned int i = 0; i < indexNum; i += 3)
		{
			VERTEX_3D v0 = Model->VertexArray[Model->IndexArray[i]];
			VERTEX_3D v1 = Model->VertexArray[Model->IndexArray[i + 1]];
			VERTEX_3D v2 = Model->VertexArray[Model->IndexArray[i + 2]];

			XMVECTOR edge1 = DirectX::XMLoadFloat3(&v1.Position) - DirectX::XMLoadFloat3(&v0.Position);
			XMVECTOR edge2 = DirectX::XMLoadFloat3(&v2.Position) - DirectX::XMLoadFloat3(&v0.Position);

			float deltaU1 = v1.TexCoord.x - v0.TexCoord.x;
			float deltaV1 = v1.TexCoord.y - v0.TexCoord.y;
			float deltaU2 = v2.TexCoord.x - v0.TexCoord.x;
			float deltaV2 = v2.TexCoord.y - v0.TexCoord.y;

			float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);
			XMVECTOR tangent = f * (deltaV2 * edge1 - deltaV1 * edge2);

			XMStoreFloat3(&v0.Tangent, tangent);
			XMStoreFloat3(&v1.Tangent, tangent);
			XMStoreFloat3(&v2.Tangent, tangent);

			Model->VertexArray[Model->IndexArray[i]] = v0;
			Model->VertexArray[Model->IndexArray[i + 1]] = v1;
			Model->VertexArray[Model->IndexArray[i + 2]] = v2;
		}
	}

	if( sc != 0 )
		Model->SubsetArray[ sc - 1 ].IndexNum = ic - Model->SubsetArray[ sc - 1 ].StartIndex;




	delete[] positionArray;
	delete[] normalArray;
	delete[] texcoordArray;
	delete[] materialArray;

	fclose(file);
}




//マテリアル読み込み///////////////////////////////////////////////////////////////////
void Model::LoadMaterial( char *FileName, MODEL_MATERIAL **MaterialArray, unsigned int *MaterialNum )
{
	char str[256];

	FILE *file;
	file = fopen( FileName, "rt" );
	if( file == NULL )
	{
		printf( "エラー:LoadMaterial %s \n", FileName );
		return;
	}

	MODEL_MATERIAL *materialArray;
	unsigned int materialNum = 0;

	//要素数カウント
	while( TRUE )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;


		if( strcmp( str, "newmtl" ) == 0 )
		{
			materialNum++;
		}
	}


	//メモリ確保
	materialArray = new MODEL_MATERIAL[ materialNum ];
	ZeroMemory(materialArray, sizeof(MODEL_MATERIAL)*materialNum);


	//要素読込
	int mc = -1;

	fseek( file, 0, SEEK_SET );

	while( TRUE )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;


		if( strcmp( str, "newmtl" ) == 0 )
		{
			//マテリアル名
			mc++;
			fscanf( file, "%s", materialArray[ mc ].Name );
			strcpy( materialArray[ mc ].DiffuseTextureName, "" );
			strcpy( materialArray[ mc ].NormalTextureName, "" );
			strcpy( materialArray[ mc ].OpacityTextureName, "" );
			strcpy(materialArray[mc].ReflectTextureName, "");
			strcpy(materialArray[mc].TranslucencyTextureName, "");
			materialArray[mc].MaterialData.noTexSampling = 1;
		}
		else if( strcmp( str, "Ka" ) == 0 )
		{
			//アンビエント
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Ambient.x );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Ambient.y );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Ambient.z );
			materialArray[ mc ].MaterialData.Ambient.w = 1.0f;
		}
		else if( strcmp( str, "Kd" ) == 0 )
		{
			//ディフューズ
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Diffuse.x );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Diffuse.y );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Diffuse.z );

			// Mayaでテクスチャを貼ると0.0fになっちゃうみたいなので
			if ((materialArray[mc].MaterialData.Diffuse.x + materialArray[mc].MaterialData.Diffuse.y + materialArray[mc].MaterialData.Diffuse.z) == 0.0f)
			{
				materialArray[mc].MaterialData.Diffuse.x = materialArray[mc].MaterialData.Diffuse.y = materialArray[mc].MaterialData.Diffuse.z = 1.0f;
			}

			materialArray[ mc ].MaterialData.Diffuse.w = 1.0f;
		}
		else if( strcmp( str, "Ks" ) == 0 )
		{
			//スペキュラ
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Specular.x );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Specular.y );
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Specular.z );
			materialArray[ mc ].MaterialData.Specular.w = 1.0f;
		}
		else if( strcmp( str, "Ns" ) == 0 )
		{
			//スペキュラ強度
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Shininess );
		}
		else if( strcmp( str, "d" ) == 0 )
		{
			//アルファ
			fscanf( file, "%f", &materialArray[ mc ].MaterialData.Diffuse.w );
		}
		else if( strcmp( str, "map_Kd" ) == 0 )
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Diffuse);
		}
		else if( strcmp( str, "norm" ) == 0)
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Normal);
		}
		else if( strcmp( str, "map_d" ) == 0 )
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Opacity);
		}
		else if (strcmp(str, "map_refl") == 0 || strcmp(str, "map_Reflect") == 0)
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Reflect);
		}
		else if (strcmp(str, "map_Bump") == 0)
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Bump);
		}
		else if (strcmp(str, "map_Translucency") == 0)
		{
			LoadTextureName(FileName, file, materialArray, mc, TextureType::Translucency);
		}
	}


	*MaterialArray = materialArray;
	*MaterialNum = materialNum;

	fclose(file);
}

void Model::LoadTextureName(char* FileName, FILE* file, MODEL_MATERIAL* Material, int mc, TextureType type)
{
	char str[256];

	//テクスチャ
	fscanf(file, "%s", str);

	char path[256];
	//	strcpy( path, "data/model/" );

		//----------------------------------- フォルダー対応
	strcpy(path, FileName);
	char* adr = path;
	char* ans = adr;
	while (1)
	{
		adr = strstr(adr, "/");
		if (adr == NULL) break;
		else ans = adr;
		adr++;
	}
	if (path != ans) ans++;
	*ans = 0;
	//-----------------------------------

	strcat(path, str);

	switch (type)
	{
	case TextureType::Diffuse:
		strcpy(Material[mc].DiffuseTextureName, path);
		Material[mc].MaterialData.noTexSampling = 0;
		break;
	case TextureType::Normal:
		strcpy(Material[mc].NormalTextureName, path);
		Material[mc].MaterialData.normalMapSampling = 1;
		break;
	case TextureType::Bump:
		strcpy(Material[mc].BumpTextureName, path);
		Material[mc].MaterialData.bumpMapSampling = 1;
		break;
	case TextureType::Opacity:
		strcpy(Material[mc].OpacityTextureName, path);
		Material[mc].MaterialData.opacityMapSampling = 1;
		break;
	case TextureType::Reflect:
		strcpy(Material[mc].ReflectTextureName, path);
		Material[mc].MaterialData.reflectMapSampling = 1;
		break;
	case TextureType::Translucency:
		strcpy(Material[mc].TranslucencyTextureName, path);
		Material[mc].MaterialData.translucencyMapSampling = 1;
		break;
	}
}


// モデルの全マテリアルのディフューズを取得する。Max16個分にしてある
void Model::GetModelDiffuse(XMFLOAT4 *diffuse)
{
	unsigned int max = (modelData->SubsetNum < MODEL_MAX_MATERIAL) ? modelData->SubsetNum : MODEL_MAX_MATERIAL;

	for (unsigned int i = 0; i < max; i++)
	{
		// ディフューズ設定
		diffuse[i] = modelData->SubsetArray[i].Material.MaterialData.Diffuse;
	}
}


// モデルの指定マテリアルのディフューズをセットする。
void Model::SetModelDiffuse(int mno, XMFLOAT4 diffuse)
{
	// ディフューズ設定
	modelData->SubsetArray[mno].Material.MaterialData.Diffuse = diffuse;
}

void Model::CreateBoundingBoxVertex()
{
	D3D11_MAPPED_SUBRESOURCE msr;
	renderer.GetDeviceContext()->Map(this->BBVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	VERTEX_3D* vertex = (VERTEX_3D*)msr.pData;

	// 頂点座標の設定
	vertex[0].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[1].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[2].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);

	vertex[3].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[4].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);
	vertex[5].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);

	vertex[6].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[7].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[8].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);

	vertex[9].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[10].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[11].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);

	vertex[12].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);
	vertex[13].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);
	vertex[14].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);

	vertex[15].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.minPoint.z);
	vertex[16].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[17].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.maxPoint.y, modelData->boundingBox.maxPoint.z);

	vertex[18].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[19].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[20].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);

	vertex[21].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.minPoint.z);
	vertex[22].Position = XMFLOAT3(modelData->boundingBox.maxPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);
	vertex[23].Position = XMFLOAT3(modelData->boundingBox.minPoint.x, modelData->boundingBox.minPoint.y, modelData->boundingBox.maxPoint.z);


	// 法線の設定
	vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[4].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[5].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[6].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[7].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[8].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[9].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[10].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[11].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[12].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[13].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[14].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[15].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[16].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[17].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[18].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[19].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[20].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[21].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[22].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	vertex[23].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

	// 拡散光の設定
	vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[4].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[5].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[6].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[7].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[8].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[9].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[10].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[11].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[12].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[13].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[14].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[15].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[16].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[17].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[18].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[19].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[20].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[21].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[22].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	vertex[23].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// テクスチャ座標の設定
	vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);
	vertex[4].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[5].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[6].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[7].TexCoord = XMFLOAT2(1.0f, 1.0f);
	vertex[8].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[9].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[10].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[11].TexCoord = XMFLOAT2(1.0f, 1.0f);
	vertex[12].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[13].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[14].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[15].TexCoord = XMFLOAT2(1.0f, 1.0f);
	vertex[16].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[17].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[18].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[19].TexCoord = XMFLOAT2(1.0f, 1.0f);
	vertex[20].TexCoord = XMFLOAT2(0.0f, 0.0f);
	vertex[21].TexCoord = XMFLOAT2(1.0f, 0.0f);
	vertex[22].TexCoord = XMFLOAT2(0.0f, 1.0f);
	vertex[23].TexCoord = XMFLOAT2(1.0f, 1.0f);

	renderer.GetDeviceContext()->Unmap(this->BBVertexBuffer, 0);
}

void Model::BuildMeshParts(void)
{
	m_meshParts.clear();

	ID3D11Buffer* sharedVB = VertexBuffer;
	ID3D11Buffer* sharedIB = IndexBuffer;

	for (unsigned int i = 0; i < modelData->SubsetNum; i++)
	{
		StaticMeshPart part = {};
		part.VertexBuffer = sharedVB;
		part.IndexBuffer = sharedIB;
		part.IndexNum = modelData->SubsetArray[i].IndexNum;
		part.StartIndex = modelData->SubsetArray[i].StartIndex;
		part.OpacityTexture = modelData->SubsetArray[i].opacityTexture;
		m_meshParts.push_back(part);
	}
}

void Model::BuildTrianglesByBoundingBox(BOUNDING_BOX box)
{
	// AABB の 8 つの頂点を定義
	XMFLOAT3 v0 = XMFLOAT3(box.minPoint.x, box.minPoint.y, box.minPoint.z);
	XMFLOAT3 v1 = XMFLOAT3(box.maxPoint.x, box.minPoint.y, box.minPoint.z);
	XMFLOAT3 v2 = XMFLOAT3(box.minPoint.x, box.maxPoint.y, box.minPoint.z);
	XMFLOAT3 v3 = XMFLOAT3(box.maxPoint.x, box.maxPoint.y, box.minPoint.z);
	XMFLOAT3 v4 = XMFLOAT3(box.minPoint.x, box.minPoint.y, box.maxPoint.z);
	XMFLOAT3 v5 = XMFLOAT3(box.maxPoint.x, box.minPoint.y, box.maxPoint.z);
	XMFLOAT3 v6 = XMFLOAT3(box.minPoint.x, box.maxPoint.y, box.maxPoint.z);
	XMFLOAT3 v7 = XMFLOAT3(box.maxPoint.x, box.maxPoint.y, box.maxPoint.z);

	// 各面の法線
	XMFLOAT3 normalXPlus = XMFLOAT3(1, 0, 0);   // +X 面の法線
	XMFLOAT3 normalXMinus = XMFLOAT3(-1, 0, 0);  // -X 面の法線
	XMFLOAT3 normalYPlus = XMFLOAT3(0, 1, 0);   // +Y 面の法線
	XMFLOAT3 normalYMinus = XMFLOAT3(0, -1, 0);  // -Y 面の法線
	XMFLOAT3 normalZPlus = XMFLOAT3(0, 0, 1);   // +Z 面の法線
	XMFLOAT3 normalZMinus = XMFLOAT3(0, 0, -1);  // -Z 面の法線

	// 各面を 2 つの三角形で作成（法線を追加）

	// -X 面
	modelData->triangles.push_back(new Triangle(v3, v7, v5, normalXMinus)); // 三角形 1
	modelData->triangles.push_back(new Triangle(v3, v5, v1, normalXMinus)); // 三角形 2

	// +X 面
	modelData->triangles.push_back(new Triangle(v2, v0, v4, normalXPlus)); // 三角形 3
	modelData->triangles.push_back(new Triangle(v2, v4, v6, normalXPlus)); // 三角形 4

	// -Y 面
	modelData->triangles.push_back(new Triangle(v2, v6, v7, normalYPlus)); // 三角形 5
	modelData->triangles.push_back(new Triangle(v2, v7, v3, normalYPlus)); // 三角形 6

	// +Y 面
	modelData->triangles.push_back(new Triangle(v0, v1, v5, normalYMinus)); // 三角形 7
	modelData->triangles.push_back(new Triangle(v0, v5, v4, normalYMinus)); // 三角形 8

	// -Z 面
	modelData->triangles.push_back(new Triangle(v7, v6, v4, normalZMinus)); // 三角形 9
	modelData->triangles.push_back(new Triangle(v7, v4, v5, normalZMinus)); // 三角形 10

	// +Z 面
	modelData->triangles.push_back(new Triangle(v3, v2, v0, normalZPlus)); // 三角形 11
	modelData->triangles.push_back(new Triangle(v3, v0, v1, normalZPlus)); // 三角形 12
}

void Model::BuildTrianglesByWorldMatrix(XMMATRIX worldMatrix, bool alwaysFaceUp)
{
	XMFLOAT3 worldPos1, worldPos2;

	XMVECTOR localAABBMax = XMVectorSet(
		modelData->boundingBox.maxPoint.x,
		modelData->boundingBox.maxPoint.y,
		modelData->boundingBox.maxPoint.z,
		1.0f
	);

	XMVECTOR localAABBMin = XMVectorSet(
		modelData->boundingBox.minPoint.x,
		modelData->boundingBox.minPoint.y,
		modelData->boundingBox.minPoint.z,
		1.0f
	);


	XMVECTOR worldPosMax = XMVector3Transform(localAABBMax, worldMatrix);
	XMVECTOR worldPosMin = XMVector3Transform(localAABBMin, worldMatrix);


	XMStoreFloat3(&worldPos1, worldPosMax);
	XMStoreFloat3(&worldPos2, worldPosMin);

	modelData->boundingBox.maxPoint = XMFLOAT3(
		max(worldPos1.x, worldPos2.x),
		max(worldPos1.y, worldPos2.y),
		max(worldPos1.z, worldPos2.z)
	);

	modelData->boundingBox.minPoint = XMFLOAT3(
		min(worldPos1.x, worldPos2.x),
		min(worldPos1.y, worldPos2.y),
		min(worldPos1.z, worldPos2.z)
	);



	int vertexNum = modelData->VertexNum;
	SimpleArray<XMFLOAT3> triangleVertices(3);
	for (int i = 0; i < vertexNum; i++)
	{
		XMFLOAT3 worldPos;

		XMVECTOR localPosVec = XMVectorSet(
			modelData->VertexArray[i].Position.x,
			modelData->VertexArray[i].Position.y,
			modelData->VertexArray[i].Position.z,
			1.0f
		);

		XMVECTOR worldPosVec = XMVector3Transform(localPosVec, worldMatrix);
		XMStoreFloat3(&worldPos, worldPosVec);

		triangleVertices.push_back(worldPos);

		if ((i + 1) % 3 == 0)
		{
			Triangle* triangle = new Triangle(
				triangleVertices[0],
				triangleVertices[1],
				triangleVertices[2],
				alwaysFaceUp
			);

			modelData->triangles.push_back(triangle);
			triangleVertices.clear();

		}
	}

}

bool Model::BuildOctree(void)
{
	if (!modelData->triangles.getSize())
		return false;

	if (CollisionManager::get_instance().staticOctree == nullptr)
		CollisionManager::get_instance().staticOctree = new OctreeNode(modelData->boundingBox);

	int numTriangles = modelData->triangles.getSize();
	for (int i = 0; i < numTriangles; i++)
	{
		if (!CollisionManager::get_instance().staticOctree->insert(modelData->triangles[i]))
			return false;
	}

	return true;
}

const SimpleArray<Triangle*>* Model::GetTriangles(void) const
{
	return &modelData->triangles;
}

const SimpleArray<StaticMeshPart>& Model::GetMeshParts(void) const
{
	if (m_meshParts.empty())
	{
		// const_castを使っているのは、constメンバ関数内で非constメンバ関数を呼ぶため
		// ただし、const_castはあまり好ましくないので、注意して使うこと
		// ここでは、const_castを使わないとコンパイルエラーになるので、やむを得ず使用している
		const_cast<Model*>(this)->BuildMeshParts();
	}
	return m_meshParts;
}

Model* Model::StoreModel(char* modelPath)
{
	MODEL_POOL* modelPool = GetModel(modelPath);
	if (modelPool == nullptr)
	{
		modelPool = new MODEL_POOL;
		modelPool->pModel = new Model(modelPath);
		modelPool->count = 1;
		modelHashMap.insert(modelPath, *modelPool);
	}
	else
		modelPool->count++;

	return modelPool->pModel;

}

MODEL_POOL* Model::GetModel(char* modelPath)
{
	return modelHashMap.search(modelPath);
}

void Model::RemoveModel(char* modelPath)
{
	modelHashMap.remove(modelPath);
}
