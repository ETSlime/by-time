//=============================================================================
//
// Townèàóù [Town.cpp]
// Author : 
//
//=============================================================================
#include "Town.h"
#include "Core/Graphics/ShadowMeshCollector.hpp"

// ì«Ç›çûÇﬁÉÇÉfÉãñº
#define	MODEL_ENVIRONMENT_PATH		"data/MODEL/Environment/"
#define	MODEL_TOWN_PATH				"data/MODEL/Environment/Knight"
#define	MODEL_CHURCH_PATH			"data/MODEL/Environment/Church"
#define	MODEL_SKYBOX_NAME			"skybox.fbx"
#define	MODEL_LOD0_NAME				"LOD0.fbx"
#define	MODEL_LOD1_NAME				"LOD1.fbx"
#define	MODEL_LOD2_NAME				"LOD2.fbx"
#define	MODEL_CHURCH_NAME			"church.fbx"

#define SIZE_SCALE				(0.5f)
#define SKYBOX_SIZE				(225850.0f)
#define PLOT_LOD0_SIZE			(5250.0f * SIZE_SCALE)
#define PLOT_LOD1_SIZE			(5250.0f * SIZE_SCALE)
#define PLOT_LOD2_SIZE			(18850.0f * SIZE_SCALE)
#define CHURCH_SIZE			(5250.0f)

Town::Town()
{
	XMMATRIX worldMatrix;


	GameObject<SkinnedMeshModelInstance>* skyBox = new GameObject<SkinnedMeshModelInstance>();
	skyBox->Instantiate(MODEL_TOWN_PATH, MODEL_SKYBOX_NAME, SkinnedModelType::Skybox);
	skyBox->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	skyBox->SetScale(XMFLOAT3(SKYBOX_SIZE, SKYBOX_SIZE, SKYBOX_SIZE));
	skyBox->Update();
	worldMatrix = skyBox->GetWorldMatrix();
	//skyBox->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix);
	//skyBox->GetSkinnedMeshModel()->BuildOctree();
	skyBox->SetCastShadow(false);

	models.push_back(skyBox);

	GameObject<SkinnedMeshModelInstance>* LoD0 = new GameObject<SkinnedMeshModelInstance>();
	LoD0->Instantiate(MODEL_TOWN_PATH, MODEL_LOD0_NAME, SkinnedModelType::Town_LOD0);
	LoD0->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	LoD0->SetScale(XMFLOAT3(PLOT_LOD0_SIZE, PLOT_LOD0_SIZE, PLOT_LOD0_SIZE));
	LoD0->SetRotation(XMFLOAT3(0, -XM_PI * 0.2f, 0.0f));
	LoD0->SetPosition(XMFLOAT3(PLOT_LOD0_SIZE * 0.63f, PLOT_LOD0_SIZE * 0.2f, -PLOT_LOD0_SIZE * 2.14f));
	LoD0->GetSkinnedMeshModel()->LoadTownTexture();
	LoD0->Update();
	worldMatrix = LoD0->GetWorldMatrix();
	LoD0->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix, true);
	LoD0->GetSkinnedMeshModel()->BuildOctree();
	LoD0->SetCastShadow(true);

	models.push_back(LoD0);

	GameObject<SkinnedMeshModelInstance>* LoD1 = new GameObject<SkinnedMeshModelInstance>();
	LoD1->Instantiate(MODEL_TOWN_PATH, MODEL_LOD1_NAME, SkinnedModelType::Town_LOD1);
	LoD1->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	LoD1->SetScale(XMFLOAT3(PLOT_LOD1_SIZE, PLOT_LOD1_SIZE, PLOT_LOD1_SIZE));
	LoD1->SetRotation(XMFLOAT3(0, -XM_PI * 0.5f, 0.0f));
	LoD1->SetPosition(XMFLOAT3(PLOT_LOD1_SIZE * 0.76f, -PLOT_LOD1_SIZE * 0.26f, -PLOT_LOD1_SIZE * 1.34f));
	LoD1->GetSkinnedMeshModel()->LoadTownTexture();
	LoD1->Update();
	worldMatrix = LoD1->GetWorldMatrix();
	LoD1->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix, true);
	LoD1->GetSkinnedMeshModel()->BuildOctree();
	LoD1->SetCastShadow(true);

	models.push_back(LoD1);

	GameObject<SkinnedMeshModelInstance>* LoD2 = new GameObject<SkinnedMeshModelInstance>();
	LoD2->Instantiate(MODEL_TOWN_PATH, MODEL_LOD2_NAME, SkinnedModelType::Town_LOD2);
	LoD2->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	LoD2->SetScale(XMFLOAT3(PLOT_LOD2_SIZE, PLOT_LOD2_SIZE, PLOT_LOD2_SIZE));
	LoD2->SetRotation(XMFLOAT3(0, -XM_PI * 0.5f, 0.0f));
	LoD2->SetPosition(XMFLOAT3(PLOT_LOD2_SIZE * 0.2f, -PLOT_LOD2_SIZE * 0.15f, -PLOT_LOD2_SIZE * 0.5f));
	LoD2->GetSkinnedMeshModel()->LoadTownTexture();
	LoD2->Update();
	worldMatrix = LoD2->GetWorldMatrix();
	LoD2->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix, true);
	LoD2->GetSkinnedMeshModel()->BuildOctree();
	LoD2->SetCastShadow(true);

	models.push_back(LoD2);

	//GameObject<SkinnedMeshModelInstance>* church = new GameObject<SkinnedMeshModelInstance>();
	//church->Instantiate(MODEL_CHURCH_PATH, MODEL_CHURCH_NAME, ModelType::Church);
	//church->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	//church->SetScale(XMFLOAT3(CHURCH_SIZE, CHURCH_SIZE, CHURCH_SIZE));
	//church->GetSkinnedMeshModel()->LoadTownTexture();
	//church->Update();
	//worldMatrix = church->GetWorldMatrix();
	//church->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix);
	//church->GetSkinnedMeshModel()->BuildOctree();
	//church->SetRenderShadow(false);

	//models.push_back(church);

	//GameObject<SkinnedMeshModelInstance>* plot5 = new GameObject<SkinnedMeshModelInstance>();
	//plot5->Instantiate(MODEL_TOWN_PATH, MODEL_PLOT2_NAME);
	//plot5->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	//plot5->SetScale(XMFLOAT3(PLOT_SIZE, PLOT_SIZE, PLOT_SIZE));
	//plot5->Update();
	//worldMatrix = plot5->GetWorldMatrix();
	//plot5->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix);
	//plot5->GetSkinnedMeshModel()->BuildOctree();
	//plot5->SetRenderShadow(false);

	//models.push_back(plot5);

	//GameObject<SkinnedMeshModelInstance>* plot9 = new GameObject<SkinnedMeshModelInstance>();
	//plot9->Instantiate(MODEL_TOWN_PATH, MODEL_PLOT2_NAME);
	//plot9->GetSkinnedMeshModel()->SetDrawBoundingBox(false);
	//plot9->SetScale(XMFLOAT3(PLOT_SIZE, PLOT_SIZE, PLOT_SIZE));
	//plot9->Update();
	//worldMatrix = plot9->GetWorldMatrix();
	//plot9->GetSkinnedMeshModel()->BuildTrianglesByWorldMatrix(worldMatrix);
	//plot9->GetSkinnedMeshModel()->BuildOctree();
	//plot9->SetRenderShadow(false);

	//models.push_back(plot9);

}

void Town::Update()
{
	int modelCnt = models.getSize();
	for (int i = 0; i < modelCnt; i++)
		models[i]->Update();
}

void Town::Draw()
{
	int modelCnt = models.getSize();
	for (int i = 0; i < modelCnt; i++)
	{
		if (m_Renderer.GetRenderMode() == RenderMode::SKINNED_MESH)
		{
			if (models[i]->GetSkinnedMeshModel()->GetModelType() == SkinnedModelType::Skybox)
			{
				m_Camera.SetCameraType(CameraType::SKYBOX);
				models[i]->Draw();
				m_Camera.SetCameraType(CameraType::SCENE);
			}
			else
				models[i]->Draw();

		}

	}

}
