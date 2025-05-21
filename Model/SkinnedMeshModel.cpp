#include "Model/SkinnedMeshModel.h"
#include "Utility/InputManager.h"
#include "Utility/Debugproc.h"
#include "AI/AnimStateMachine.h"
#include "Collision/CollisionManager.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_MODEL_NUM		(30)

//*****************************************************************************
// グローバル変数
//*****************************************************************************
HashMap<char*, SkinnedMeshModelPool, CharPtrHash, CharPtrEquals> SkinnedMeshModel::modelHashMap(
    MAX_MODEL_NUM,
    CharPtrHash(),
    CharPtrEquals()
);

int ModelData::modelCnt = 0;

static FBXLoader& fbxLoader = FBXLoader::get_instance();
static TextureMgr& mTexMgr = TextureMgr::get_instance();
static Timer& t = Timer::get_instance();

void SkinnedMeshModel::UpdateBoneTransform(SimpleArray<XMFLOAT4X4>* boneTransforms)
{

    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;

        if (boneTransforms)
        {
            boneTransformData.mBoneFinalTransforms.clear();



            int numBone = boneTransforms->getSize();
            for (int i = 0; i < numBone; i++)
            {
                boneTransformData.mBoneFinalTransforms.push_back((*boneTransforms)[i]);
            }
        }
        else
        {
            ModelData* modelData = it.value;
            FbxNode* armatureNode = modelData->armatureNode;


            if (armatureNode)
            {
                boneTransformData.mModelGlobalScl.clear();
                boneTransformData.mModelGlobalRot.clear();
                boneTransformData.mModelTranslate.clear();

                boneTransformData.mModelGlobalTrans.clear();
                boneTransformData.mModelLocalTrans.clear();
                boneTransformData.mBoneFinalTransforms.clear();

                boneTransformData.limbHashMap.clear();

                int curIdx = 0;
                int prevIdx = -1;

                UpdateLimbGlobalTransform(currentAnimClip->armatureNode, armatureNode, curIdx, prevIdx, currentAnimClip->currentTime, &boneTransformData, currentAnimClip->animInfo);
                GetBoneTransform(boneTransformData.mBoneFinalTransforms, modelData);
            }
        }
    }
}

void SkinnedMeshModel::DrawModel()
{

    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;
        FbxNode* armatureNode = modelData->armatureNode;

        if (armatureNode)
        {
            XMMATRIX	boneMatrices[BONE_MAX];
            int size = boneTransformData.mBoneFinalTransforms.getSize();
            for (int i = 0; i < size; i++)
            {
                boneMatrices[i] = XMMatrixTranspose(XMLoadFloat4x4(&boneTransformData.mBoneFinalTransforms[i]));
                //XMMATRIX dbg = boneMatrices[i];
                //dbg = boneMatrices[i];
            }
            Renderer::get_instance().SetBoneMatrix(boneMatrices);
        }
        else
        {
            XMMATRIX	boneMatrices[BONE_MAX];
            boneMatrices[0] = XMMatrixTranspose(XMMatrixIdentity());
            Renderer::get_instance().SetBoneMatrix(boneMatrices);
        }

        // 頂点バッファ設定
        UINT stride = sizeof(SKINNED_VERTEX_3D);
        UINT offset = 0;
        Renderer::get_instance().GetDeviceContext()->IASetVertexBuffers(0, 1, &modelData->VertexBuffer, &stride, &offset);

        // インデックスバッファ設定
        Renderer::get_instance().GetDeviceContext()->IASetIndexBuffer(modelData->IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // プリミティブトポロジ設定
        Renderer::get_instance().GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        MATERIAL material;
        ZeroMemory(&material, sizeof(material));
        material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        if (modelData->diffuseTexture)
            material.noTexSampling = FALSE;
        else
            material.noTexSampling = TRUE;
        if (modelType == SkinnedModelType::Town_LOD0 ||
            modelType == SkinnedModelType::Town_LOD1 ||
            modelType == SkinnedModelType::Town_LOD2)
            material.noTexSampling = FALSE;

        Renderer::get_instance().SetMaterial(material);

        Renderer::get_instance().SetCullingMode(CULL_MODE_NONE);

        switch (modelType)
        {
        case SkinnedModelType::Sigewinne:
            DrawSigewinne(modelData);
            break;
        case SkinnedModelType::Klee:
            DrawKlee(modelData);
            break;
        case SkinnedModelType::Lumine:
            DrawLumine(modelData);
            break;
        case SkinnedModelType::Field:
            DrawField(modelData);
            break;
        case SkinnedModelType::Town_LOD0:
            //Renderer::get_instance().SetFillMode(D3D11_FILL_WIREFRAME);
            DrawTownLoD(modelData, 0);
            //Renderer::get_instance().SetFillMode(D3D11_FILL_SOLID);
            break;
        case SkinnedModelType::Town_LOD1:
            //Renderer::get_instance().SetFillMode(D3D11_FILL_WIREFRAME);
            DrawTownLoD(modelData, 1);
            //Renderer::get_instance().SetFillMode(D3D11_FILL_SOLID);
            break;
        case SkinnedModelType::Town_LOD2:
            //Renderer::get_instance().SetFillMode(D3D11_FILL_WIREFRAME);
            DrawTownLoD(modelData, 2);
            //Renderer::get_instance().SetFillMode(D3D11_FILL_SOLID);
            break;
        case SkinnedModelType::Church:
            DrawChurch(modelData);
            break;
        case SkinnedModelType::Weapon:
        default:
            Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &modelData->diffuseTexture);
            Renderer::get_instance().GetDeviceContext()->DrawIndexed(modelData->IndexNum, 0, 0);
            break;
        }

#ifdef _DEBUG
        //if (drawBoundingBox)
        //    DrawBoundingBox(modelData);
#endif

        // カリング設定を戻す
        Renderer::get_instance().SetCullingMode(CULL_MODE_BACK);
    }
}

void SkinnedMeshModel::SetBodyDiffuseTexture(char* texturePath)
{
    bodyDiffuseTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetBodyLightMapTexture(char* texturePath)
{
    bodyLightMapTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetBodyNormalMapTexture(char* texturePath)
{
    bodyNormalMapTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetHairDiffuseTexture(char* texturePath)
{
    hairDiffuseTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetHairLightMapTexture(char* texturePath)
{
    hairLightMapTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetFaceDiffuseTexture(char* texturePath)
{
    faceDiffuseTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::SetFaceLightMapTexture(char* texturePath)
{
    faceLightMapTexture = TextureMgr::get_instance().CreateTexture(texturePath);
}

void SkinnedMeshModel::LoadTownTexture(void)
{
    Area_Mdcity_Lvy01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_Mdcity_Lvy01_Diffuse.png");
    Area_Mdbuild_Wall06_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_Mdbuild_Wall06_Diffuse.png");
    Indoor_OutDoor_MDSkyBox = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Indoor_OutDoor_MDSkyBox.png");
    Area_MdBuild_KnightHQ02_Assembly01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_KnightHQ02_Assembly01_Diffuse.png");
    Area_MdBuild_House_Roof05_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_House_Roof05_Diffuse.png");
    Area_MdBuild_All_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_All_Diffuse.png");
    Area_Mdbuild_Edge01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_Mdbuild_Edge01_Diffuse.png");
    Area_MdBuild_Wall09_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Wall09_Diffuse.png");
    Area_MdBuild_Column01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Column01_Diffuse.png");
    Area_MdCity_Plot02_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdCity_Plot02_Diffuse.png");
    Area_MdCity_Plot05_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdCity_Plot05_Diffuse.png");
    Area_MdCity_Plot09_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdCity_Plot09_Diffuse.png");
    Area_MdCity_Plot04_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdCity_Plot04_Diffuse.png");
    Area_MdCity_Plot03_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdCity_Plot03_Diffuse.png");
    Area_MdBuild_House_Roof04_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_House_Roof04_Diffuse.png");
    Area_MdBuild_House_Roof05_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_House_Roof05_Diffuse.png");
    Stages_Wood_Pillar_02_T2_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Stages_Wood_Pillar_02_T2_Diffuse.png");
    Area_MdProps_Gadget02_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdProps_Gadget02_Diffuse.png");
    Area_MdBuild_Window50_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Window50_Diffuse.png");
    Area_MdBuild_Window_A_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Window_A_Diffuse.png");
    Area_MdBuild_Window30_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Window30_Diffuse.png");
    Area_Mdbuild_ManorWall01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_Mdbuild_ManorWall01_Diffuse.png");
    Area_MdBuild_House_Wall07_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_House_Wall07_Diffuse.png");
    Stages_CyTree02_Leaf_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Stages_CyTree02_Leaf_Diffuse.png");
    Stages_Tree04_Bark_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Stages_Tree04_Bark_Diffuse.png");
    Area_MdBuild_Wall08_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Knight/Area_MdBuild_Wall08_Diffuse.png");


    Area_MdBuild_Church01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Area_MdBuild_Church01_Diffuse.png");
    Area_MdBuild_Church02_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Area_MdBuild_Church02_Diffuse.png");
    Area_MdBuild_Flag_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Area_MdBuild_Flag_Diffuse.png");
    Indoor_MdBuild_Church_Ground01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_Church_Ground01_Diffuse.png");
    Indoor_MdBuild_Church_GroundPattern01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_Church_GroundPattern01_Diffuse.png");
    Indoor_MdBuild_Church_Stairs01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_Church_Stairs01_Diffuse.png");
    Indoor_MdBuild_Church_Wall01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_Church_Wall01_Diffuse.png");
    Indoor_MdBuild_Church_Wall02_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_Church_Wall02_Diffuse.png");
    Indoor_Mdprops_Church_Squate02_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_Mdprops_Church_Squate02_Diffuse.png");
    Indoor_MdProps_Church_Item01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdProps_Church_Item01_Diffuse.png");
    Indoor_Mdprops_Church_Lights01_Diffuse = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_Mdprops_Church_Lights01_Diffuse.png");
    Indoor_MdBuild_WindowEffect04_NoStream = TextureMgr::get_instance().CreateTexture("data/MODEL/Environment/Church/Indoor_MdBuild_WindowEffect04_NoStream.png");
}

void SkinnedMeshModel::GetBoneTransformByAnim(FbxNode* currentClipArmatureNode, uint64_t currentClipTime, 
    SimpleArray<XMFLOAT4X4>* boneFinalTransform, AnimationInfo& animInfo)
{
    (*boneFinalTransform).clear();
    
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;
        FbxNode* armatureNode = modelData->armatureNode;


        if (armatureNode)
        {
            boneTransformData.mModelGlobalScl.clear();
            boneTransformData.mModelGlobalRot.clear();
            boneTransformData.mModelTranslate.clear();

            boneTransformData.mModelGlobalTrans.clear();
            boneTransformData.mModelLocalTrans.clear();

            boneTransformData.limbHashMap.clear();

            int curIdx = 0;
            int prevIdx = -1;

            UpdateLimbGlobalTransform(currentClipArmatureNode, armatureNode, curIdx, prevIdx, currentClipTime, &boneTransformData, animInfo);
            GetBoneTransform(*boneFinalTransform, modelData);
        }
    }
}

void SkinnedMeshModel::SetBoundingBoxLocationOffset(XMFLOAT3 offset, int boneIdx)
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;
        int numBoundigBoxes = modelData->boundingBoxes.getSize();
        if (boneIdx >= numBoundigBoxes)
            return;


        for (int i = 0; i < numBoundigBoxes; i++)
        {
            XMFLOAT3 size = XMFLOAT3(
                (modelData->boundingBoxes[i]->baseMaxPoint.x - modelData->boundingBoxes[i]->baseMinPoint.x),
                (modelData->boundingBoxes[i]->baseMaxPoint.y - modelData->boundingBoxes[i]->baseMinPoint.y),
                (modelData->boundingBoxes[i]->baseMaxPoint.z - modelData->boundingBoxes[i]->baseMinPoint.z)
            );

            XMFLOAT3 offsetVal = XMFLOAT3(
                size.x * offset.x,
                size.y * offset.y,
                size.z * offset.z
            );

            XMFLOAT3 newMaxPoint = XMFLOAT3(
                modelData->boundingBoxes[i]->baseMaxPoint.x + offsetVal.x,
                modelData->boundingBoxes[i]->baseMaxPoint.y + offsetVal.y,
                modelData->boundingBoxes[i]->baseMaxPoint.z + offsetVal.z
            );


            XMFLOAT3 newMinPoint = XMFLOAT3(
                modelData->boundingBoxes[i]->baseMinPoint.x + offsetVal.x,
                modelData->boundingBoxes[i]->baseMinPoint.y + offsetVal.y,
                modelData->boundingBoxes[i]->baseMinPoint.z + offsetVal.z
            );

            modelData->boundingBoxes[i]->maxPoint = newMaxPoint;
            modelData->boundingBoxes[i]->minPoint = newMinPoint;
            modelData->boundingBoxes[i]->baseMaxPoint = newMaxPoint;
            modelData->boundingBoxes[i]->baseMinPoint = newMinPoint;

            CreateBoundingBoxVertex(modelData->boundingBoxes[i]);
        }
    }
}

void SkinnedMeshModel::SetBoundingBoxSize(XMFLOAT3 size, int boneIdx)
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;
        int numBoundigBoxes = modelData->boundingBoxes.getSize();
        if (boneIdx >= numBoundigBoxes)
            return;

        
        for (int i = 0; i < numBoundigBoxes; i++)
        {
            XMFLOAT3 oldSize = XMFLOAT3(
                (modelData->boundingBoxes[i]->baseMaxPoint.x - modelData->boundingBoxes[i]->baseMinPoint.x),
                (modelData->boundingBoxes[i]->baseMaxPoint.y - modelData->boundingBoxes[i]->baseMinPoint.y),
                (modelData->boundingBoxes[i]->baseMaxPoint.z - modelData->boundingBoxes[i]->baseMinPoint.z));

            XMFLOAT3 newSize = XMFLOAT3(
                oldSize.x * size.x,
                oldSize.y * size.y,
                oldSize.z * size.z);

            XMFLOAT3 newMaxPoint = XMFLOAT3(
                modelData->boundingBoxes[i]->baseMaxPoint.x + (newSize.x - oldSize.x) / 2,
                modelData->boundingBoxes[i]->baseMaxPoint.y + (newSize.y - oldSize.y) / 2,
                modelData->boundingBoxes[i]->baseMaxPoint.z + (newSize.z - oldSize.z) / 2);

            XMFLOAT3 newMinPoint = XMFLOAT3(
                modelData->boundingBoxes[i]->baseMinPoint.x - (newSize.x - oldSize.x) / 2,
                modelData->boundingBoxes[i]->baseMinPoint.y - (newSize.y - oldSize.y) / 2,
                modelData->boundingBoxes[i]->baseMinPoint.z - (newSize.z - oldSize.z) / 2);

            modelData->boundingBoxes[i]->maxPoint = newMaxPoint;
            modelData->boundingBoxes[i]->minPoint = newMinPoint;
            modelData->boundingBoxes[i]->baseMaxPoint = newMaxPoint;
            modelData->boundingBoxes[i]->baseMinPoint = newMinPoint;

            CreateBoundingBoxVertex(modelData->boundingBoxes[i]);

            boundingBox.maxPoint = newMaxPoint;
            boundingBox.minPoint = newMinPoint;
        }
    }
}

void SkinnedMeshModel::BuildTrianglesByWorldMatrix(XMMATRIX worldMatrix, bool alwaysFaceUp)
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;
        
        //XMFLOAT3 worldPos1, worldPos2;

        //XMVECTOR localAABBMax = XMVectorSet(
        //    modelData->boundingBoxes[0]->baseMaxPoint.x,
        //    modelData->boundingBoxes[0]->baseMaxPoint.y,
        //    modelData->boundingBoxes[0]->baseMaxPoint.z,
        //    1.0f
        //);

        //XMVECTOR localAABBMin = XMVectorSet(
        //    modelData->boundingBoxes[0]->baseMinPoint.x,
        //    modelData->boundingBoxes[0]->baseMinPoint.y,
        //    modelData->boundingBoxes[0]->baseMinPoint.z,
        //    1.0f
        //);

        XMMATRIX boneTransform = GetBoneFinalTransform();

        //XMVECTOR skinnedAABBMax = XMVector3Transform(localAABBMax, boneTransform);
        //XMVECTOR skinnedAABBMin = XMVector3Transform(localAABBMin, boneTransform);

        //XMVECTOR worldPosMax = XMVector3Transform(skinnedAABBMax, worldMatrix);
        //XMVECTOR worldPosMin = XMVector3Transform(skinnedAABBMin, worldMatrix);


        //XMStoreFloat3(&worldPos1, worldPosMax);
        //XMStoreFloat3(&worldPos2, worldPosMin);

        //boundingBox.maxPoint = XMFLOAT3(
        //    max(worldPos1.x, worldPos2.x),
        //    max(worldPos1.y, worldPos2.y),
        //    max(worldPos1.z, worldPos2.z)
        //);

        //boundingBox.minPoint = XMFLOAT3(
        //    min(worldPos1.x, worldPos2.x),
        //    min(worldPos1.y, worldPos2.y),
        //    min(worldPos1.z, worldPos2.z)
        //);

        

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

            XMVECTOR skinnedPosVec = XMVector3Transform(localPosVec, boneTransform);
            XMVECTOR worldPosVec = XMVector3Transform(skinnedPosVec, worldMatrix);
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
}

void SkinnedMeshModel::BuildTrianglesByBoundingBox(BOUNDING_BOX box)
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;

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
}

bool SkinnedMeshModel::BuildOctree(void)
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;

        if (!modelData->triangles.getSize())
            return false;

        if (!modelData->boundingBoxes.getSize())
            return false;

        if (CollisionManager::get_instance().staticOctree == nullptr)
            CollisionManager::get_instance().staticOctree = new OctreeNode(boundingBox);

        int numTriangles = modelData->triangles.getSize();
        for (int i = 0; i < numTriangles; i++)
        {
            if (!CollisionManager::get_instance().staticOctree->insert(modelData->triangles[i]))
                return false;
        }
    }

    return true;
}

const SimpleArray<Triangle*>* SkinnedMeshModel::GetTriangles(void) const
{
    for (auto& it : meshDataMap)
    {
        ModelData* modelData = it.value;

        return &modelData->triangles;
    }

    return nullptr;
}

void SkinnedMeshModel::SetCurrentAnim(AnimationClip* currAnimClip, float startTime)
{
    if (currAnimClip)
    {
        this->currentAnimClip = currAnimClip;
        this->currentAnimClip->currentTime = static_cast<uint64_t>(currAnimClip->stopTime * startTime);
    }
}

AnimationClip* SkinnedMeshModel::GetAnimationClip(AnimClipName clipName)
{
    AnimationClip* animClip = animationClips.search(clipName);
    if (animClip)
        return animClip;

    return nullptr;
}

void SkinnedMeshModel::PlayCurrentAnim(float playSpeed)
{
    currentAnimClip->currentTime += static_cast<uint64_t>(ANIM_SPD * playSpeed * t.GetScaledDeltaTime());
}

XMMATRIX SkinnedMeshModel::GetWeaponTransformMtx(void)
{
    if (boneTransformData.mBoneFinalTransforms.getSize() > weaponTransformIdx)
    {
        XMFLOAT4X4 transform =  boneTransformData.mBoneFinalTransforms[weaponTransformIdx];

        return XMLoadFloat4x4(&transform);
    }

    return XMMatrixIdentity();
}

XMMATRIX SkinnedMeshModel::GetBodyTransformMtx(void)
{
    if (boneTransformData.mBoneFinalTransforms.getSize() > 1)
    {
        XMFLOAT4X4 transform = boneTransformData.mBoneFinalTransforms[1];

        return XMLoadFloat4x4(&transform);
    }

    return XMMatrixIdentity();
}

XMMATRIX SkinnedMeshModel::GetBoneFinalTransform(UINT boneIdx)
{
    if (boneTransformData.mBoneFinalTransforms.getSize() > boneIdx )
    {
        XMFLOAT4X4 transform = boneTransformData.mBoneFinalTransforms[boneIdx];

        return XMLoadFloat4x4(&transform);
    }

    return XMMatrixIdentity();
}

const XMMATRIX* SkinnedMeshModel::GetFinalBoneMatrices(void)
{
    UINT size = boneTransformData.mBoneFinalTransforms.getSize();
    if (size == 0) return nullptr;

    for (UINT i = 0; i < size; i++) 
    {
        m_transposedBoneMatrices[i] =
            XMMatrixTranspose(XMLoadFloat4x4(&boneTransformData.mBoneFinalTransforms[i]));
    }
    return m_transposedBoneMatrices;
}

SkinnedMeshModel::SkinnedMeshModel()
{
    armatureNode = nullptr;
    currentRootNodeID = 0;
    weaponTransformIdx = 0;

    armatureNode = nullptr;

    bodyDiffuseTexture = nullptr;
    bodyLightMapTexture = nullptr;
    bodyNormalMapTexture = nullptr;
    hairDiffuseTexture = nullptr;
    hairLightMapTexture = nullptr;
    faceDiffuseTexture = nullptr;
    faceLightMapTexture = nullptr;

    drawBoundingBox = true;
}

SkinnedMeshModel::~SkinnedMeshModel()
{
    SafeRelease(&bodyDiffuseTexture);
    SafeRelease(&bodyLightMapTexture);
    SafeRelease(&bodyNormalMapTexture);
    SafeRelease(&hairDiffuseTexture);
    SafeRelease(&hairLightMapTexture);
    SafeRelease(&faceDiffuseTexture);
    SafeRelease(&faceLightMapTexture);

    for (auto& it : fbxNodes)
    {
        SAFE_DELETE(it.value);
    }

    for (auto& it : meshDataMap)
    {
        SAFE_DELETE(it.value);
    }
}

XMMATRIX CreateRotationMatrix(float pitch, float yaw, float roll)
{
    XMMATRIX Rz = XMMATRIX(
        cos(roll), -sin(roll), 0, 0,
        sin(roll), cos(roll), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    XMMATRIX Ry = XMMATRIX(
        cos(yaw), 0, sin(yaw), 0,
        0, 1, 0, 0,
        -sin(yaw), 0, cos(yaw), 0,
        0, 0, 0, 1
    );

    XMMATRIX Rx = XMMATRIX(
        1, 0, 0, 0,
        0, cos(pitch), -sin(pitch), 0,
        0, sin(pitch), cos(pitch), 0,
        0, 0, 0, 1
    );

    return XMMatrixTranspose(Rz * Ry * Rx);
}

SkinnedMeshModel* SkinnedMeshModel::StoreModel(char* modelPath, char* modelName, char* modelFullPath, 
    SkinnedModelType modelType, AnimClipName clipName)
{
    SkinnedMeshModelPool* modelPool = GetModel(modelFullPath);
    if (modelPool == nullptr)
    {
        modelPool = new SkinnedMeshModelPool;
        modelPool->pModel = new SkinnedMeshModel();
        bool loadSuccess;
        loadSuccess = fbxLoader.LoadModel(Renderer::get_instance().GetDevice(), mTexMgr,
            *modelPool->pModel, modelPath, modelName,
            nullptr, clipName, modelType);
        if (!loadSuccess)
            return nullptr;
        modelPool->count = 1;
        modelHashMap.insert(modelFullPath, *modelPool);

        return modelPool->pModel;
    }
    else
    {
        modelPool->count++;
        SkinnedMeshModel* model = new SkinnedMeshModel();
        *model = *modelPool->pModel;
        return model;
    }
}

SkinnedMeshModelPool* SkinnedMeshModel::GetModel(char* modelFullPath)
{
    return modelHashMap.search(modelFullPath);
}

void SkinnedMeshModel::RemoveModel(char* modelPath)
{
    modelHashMap.remove(modelPath);
}

const SimpleArray<SkinnedMeshPart>& SkinnedMeshModel::GetMeshParts() const
{
    if (m_meshParts.empty())
    {
        const_cast<SkinnedMeshModel*>(this)->BuildMeshParts();
    }
    return m_meshParts;
}

void SkinnedMeshModel::UpdateLimbGlobalTransform(FbxNode* node, FbxNode* deformNode, 
    int& curIdx, int prevIdx, uint64_t time, BoneTransformData* boneTransformData, AnimationInfo& animInfo)
{
    ModelProperty* modelProperty = static_cast<ModelProperty*>(node->nodeData);

    if (modelProperty)
    {
        modelProperty->modelIdx = curIdx;
        boneTransformData->limbHashMap.insert(deformNode->nodeID, curIdx);
        XMMATRIX mtxLocalScl, mtxLocalRot, mtxLocalTranslate, mtxLcl, mtxGlobalTrans;

        mtxLcl = XMMatrixIdentity();

        LimbNodeAnimation* limbNodeAnimation = static_cast<LimbNodeAnimation*>(node->limbNodeAnimation);
        if (limbNodeAnimation)
        {
            FbxNode** animationCurveNodeScl = fbxNodes.search(limbNodeAnimation->LclScl);
            XMFLOAT3 animationValueScl = GetAnimationValue(animationCurveNodeScl, modelProperty->Scaling, time, animInfo);
            mtxLocalScl = XMMatrixScaling(animationValueScl.x, animationValueScl.y, animationValueScl.z);

            FbxNode** animationCurveNodeRot = fbxNodes.search(limbNodeAnimation->LclRot);
            XMFLOAT3 animationValueRot = GetAnimationValue(animationCurveNodeRot, modelProperty->Rotation, time, animInfo);
            mtxLocalRot = CreateRotationMatrix(XMConvertToRadians(animationValueRot.x), XMConvertToRadians(animationValueRot.y), XMConvertToRadians(animationValueRot.z));

            FbxNode** animationCurveNodeTranslation = fbxNodes.search(limbNodeAnimation->LclTranslation);
            XMFLOAT3 animationValueTranslation = GetAnimationValue(animationCurveNodeTranslation, modelProperty->Translation, time, animInfo);
            mtxLocalTranslate = XMMatrixTranslation(animationValueTranslation.x, animationValueTranslation.y, animationValueTranslation.z);

        }
        else
        {
            mtxLocalScl = XMMatrixScaling(modelProperty->Scaling.x, modelProperty->Scaling.y, modelProperty->Scaling.z);
            mtxLocalRot = CreateRotationMatrix(XMConvertToRadians(modelProperty->Rotation.x), XMConvertToRadians(modelProperty->Rotation.y), XMConvertToRadians(modelProperty->Rotation.z));
            mtxLocalTranslate = XMMatrixTranslation(modelProperty->Translation.x, modelProperty->Translation.y, modelProperty->Translation.z);
        }



        XMFLOAT4X4 localTrans;
        XMFLOAT4X4 globalTranslate, globalRot, globalScl, globalTrans;
        XMMATRIX parentTranslate, mtxParentRot, mtxParentScl, parentTrans, mtxGlobalTranslate;

        if (prevIdx == -1)
        {
            mtxParentScl = XMMatrixIdentity();
            parentTranslate = XMMatrixIdentity();
            mtxParentRot = XMMatrixIdentity();
            parentTrans = XMMatrixIdentity();

        }
        else
        {
            mtxParentScl = XMLoadFloat4x4(&boneTransformData->mModelGlobalScl[prevIdx]);
            parentTranslate = XMLoadFloat4x4(&boneTransformData->mModelTranslate[prevIdx]);
            mtxParentRot = XMLoadFloat4x4(&boneTransformData->mModelGlobalRot[prevIdx]);
            parentTrans = XMLoadFloat4x4(&boneTransformData->mModelGlobalTrans[prevIdx]);
        }

        mtxLcl = XMMatrixMultiply(mtxLcl, mtxLocalScl);
        mtxLcl = XMMatrixMultiply(mtxLcl, mtxLocalRot);
        mtxLcl = XMMatrixMultiply(mtxLcl, mtxLocalTranslate);

        XMStoreFloat4x4(&localTrans, mtxLcl);
        XMStoreFloat4x4(&globalScl, XMMatrixMultiply(mtxParentScl, mtxLocalScl));

        XMVECTOR localTranslationVec = XMVectorSetW(mtxLocalTranslate.r[3], 0.0f);
        XMVECTOR globalTranslationVec = XMVector3Transform(localTranslationVec, parentTrans);
        mtxGlobalTranslate = XMMatrixTranslationFromVector(globalTranslationVec);
        XMStoreFloat4x4(&globalTranslate, mtxGlobalTranslate);

        // eInheritRSrs: lGlobalRS = lParentGRM * lParentGSM * lLRM * lLSM;
        mtxGlobalTrans = mtxLocalScl * mtxLocalRot * mtxParentScl * mtxParentRot;
        // eInheritRrSs: lGlobalRS = lParentGRM * lLRM * lParentGSM * lLSM;
        //mtxGlobalTrans = mtxLocalScl * mtxParentScl * mtxLocalRot * mtxParentRot;
        mtxGlobalTrans = XMMatrixMultiply(mtxGlobalTrans, mtxGlobalTranslate);
        globalTranslationVec = XMVector3Transform(localTranslationVec, parentTrans);
        XMStoreFloat4x4(&globalTrans, mtxGlobalTrans);
        XMStoreFloat4x4(&globalRot, XMMatrixMultiply(mtxLocalRot, mtxParentRot));

        boneTransformData->mModelGlobalScl.push_back(globalScl);
        boneTransformData->mModelGlobalRot.push_back(globalRot);
        boneTransformData->mModelTranslate.push_back(globalTranslate);
        boneTransformData->mModelGlobalTrans.push_back(globalTrans);
        boneTransformData->mModelLocalTrans.push_back(localTrans);

        SimpleArray<FbxNode*> childNodes = node->childNodes;
        for (UINT i = 0; i < childNodes.getSize(); i++)
        {
            if (childNodes[i]->nodeType == FbxNodeType::LimbNode)
            {
                int prev = modelProperty->modelIdx;
                curIdx++;
                UpdateLimbGlobalTransform(node->childNodes[i], deformNode->childNodes[i], curIdx, prev, time, boneTransformData, animInfo);
            }
            else if (childNodes[i]->nodeType == FbxNodeType::Mesh)
            {
                SimpleArray<FbxNode*> limbNodes = childNodes[i]->childNodes;
                for (UINT j = 0; j < limbNodes.getSize(); j++)
                {
                    if (limbNodes[j]->nodeType == FbxNodeType::LimbNode)
                    {
                        int prev = modelProperty->modelIdx;
                        curIdx++;
                        UpdateLimbGlobalTransform(node->childNodes[i]->childNodes[j], deformNode->childNodes[i]->childNodes[j], curIdx, prev, time, boneTransformData, animInfo);
                    }
                }
            }
        }
    }
}

void SkinnedMeshModel::GetBoneTransform(SimpleArray<XMFLOAT4X4>& boneFinalTransform, ModelData* modelData)
{
    int numBones = modelData->mBoneHierarchy.getSize();

    if (numBones > 0)
    {
        if (boneFinalTransform.getSize() != numBones)
            boneFinalTransform.resize(numBones);

        SimpleArray<XMFLOAT4X4> toRootTransforms(numBones);
        toRootTransforms[0] = modelData->mBoneToParentTransforms[0];
        for (int i = 1; i < numBones; i++)
        {
            XMMATRIX toParent = XMLoadFloat4x4(&modelData->mBoneToParentTransforms[i]);
            int parentIndex = modelData->mBoneHierarchy[i];
            XMMATRIX parentToRoot;
            if (parentIndex == -1)
                parentToRoot = XMMatrixIdentity();
            else
                parentToRoot = XMLoadFloat4x4(&toRootTransforms[parentIndex]);
            XMMATRIX toRoot = XMMatrixMultiply(toParent, parentToRoot);
            XMStoreFloat4x4(&toRootTransforms[i], toRoot);
        }

        for (int i = 0; i < numBones; ++i)
        {
            int limbIdx = 0;
            int deformerIdx = 0;
            int* pLimbIdx = nullptr;
            int* pDeformerIdx = nullptr;
            uint64_t* pDeformerID = nullptr;
            uint64_t* pLimbID = nullptr;

            pDeformerID = deformerHashMap.search(i);
            if (pDeformerID == nullptr) continue;

            pDeformerIdx = deformerIdxHashMap.search(*pDeformerID);
            if (pDeformerIdx)
                deformerIdx = *pDeformerIdx;

            pLimbID = deformerToLimb.search(*pDeformerID);
            if (pLimbID == nullptr)
                limbIdx = i;
            else
            {
                pLimbIdx = boneTransformData.limbHashMap.search(*pLimbID);
                if (pLimbIdx == nullptr) continue;
                limbIdx = *pLimbIdx;
            }



            XMMATRIX mtxInverseRootTransform;

            XMMATRIX mtxGlobalTrans = XMLoadFloat4x4(&boneTransformData.mModelGlobalTrans[limbIdx]);
            XMMATRIX mtxLocalTrans = XMLoadFloat4x4(&boneTransformData.mModelLocalTrans[limbIdx]);

            XMMATRIX offset = XMLoadFloat4x4(&modelData->mBoneOffsets[deformerIdx]);
            XMMATRIX toRoot = XMLoadFloat4x4(&toRootTransforms[deformerIdx]);
            XMMATRIX mtxTrans = XMMatrixIdentity();


            XMFLOAT4X4* pBindPose = nullptr;
            if (pLimbID)
                pBindPose = bindPose.mtxBindPoses.search(*pLimbID);
            XMMATRIX bindPose, inverseBindPose;
            if (pBindPose == nullptr)
            {
                bindPose = XMMatrixIdentity();
                inverseBindPose = XMMatrixIdentity();
            }
            else
            {
                bindPose = XMLoadFloat4x4(pBindPose);
                inverseBindPose = XMMatrixInverse(nullptr, XMLoadFloat4x4(pBindPose));
            }

            if (i > 0)
            {
                XMMATRIX rootTransform = XMLoadFloat4x4(&boneFinalTransform[0]);
                rootTransform.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
                mtxInverseRootTransform = XMMatrixInverse(nullptr, rootTransform);
            }
            else
            {
                mtxInverseRootTransform = XMMatrixIdentity();
            }



            XMMATRIX inverseTransformLink = XMMatrixInverse(nullptr, offset);
            XMMATRIX newBindPose = XMMatrixMultiply(inverseTransformLink, toRoot);
            XMMATRIX newBindPoseChange = newBindPose * bindPose;
            XMVECTOR row0 = newBindPoseChange.r[0];
            XMVECTOR row1 = newBindPoseChange.r[1];
            XMVECTOR row2 = newBindPoseChange.r[2];
            XMVECTOR row3 = newBindPoseChange.r[3];

            row0 = XMVectorSetZ(row0, -XMVectorGetZ(row0));
            row1 = XMVectorSetZ(row1, -XMVectorGetZ(row1));
            row2 = XMVectorSetX(row2, -XMVectorGetX(row2));
            row2 = XMVectorSetY(row2, -XMVectorGetY(row2));
            row2 = XMVectorSetW(row2, -XMVectorGetW(row2));
            row3 = XMVectorSetZ(row3, -XMVectorGetZ(row3));

            newBindPoseChange = XMMATRIX(row0, row1, row2, row3);
            newBindPose = newBindPoseChange;

            XMVECTOR bindPoseTranslationVec = XMVectorSetW(newBindPose.r[3], 0.0f);
            XMVECTOR inverseBindPoseTranslationVec = XMVector3Transform(bindPoseTranslationVec, mtxInverseRootTransform);
            newBindPose.r[3] = inverseBindPoseTranslationVec;

            if (pLimbID == nullptr)
            {
                mtxTrans = mtxLocalTrans * toRoot * offset;
            }
            else
                mtxTrans = XMMatrixMultiply(toRoot, offset);

            XMMATRIX mtxTransTmp = mtxGlobalTrans;
            row0 = mtxTransTmp.r[0];
            row1 = mtxTransTmp.r[1];
            row2 = mtxTransTmp.r[2];
            row3 = mtxTransTmp.r[3];

            if (i > 0)
            {
                row0 = XMVectorSetZ(row0, -XMVectorGetZ(row0));
                row1 = XMVectorSetZ(row1, -XMVectorGetZ(row1));
                row2 = XMVectorSetX(row2, -XMVectorGetX(row2));
                row2 = XMVectorSetY(row2, -XMVectorGetY(row2));
                row3 = XMVectorSetZ(row3, -XMVectorGetZ(row3));
            }
            mtxTransTmp = XMMATRIX(row0, row1, row2, row3);
            mtxGlobalTrans = mtxTransTmp;

            {

                float temp = newBindPose.r[3].m128_f32[1];
                newBindPose.r[3].m128_f32[1] = newBindPose.r[3].m128_f32[2];
                newBindPose.r[3].m128_f32[2] = temp;

                newBindPose.r[3].m128_f32[2] = -newBindPose.r[3].m128_f32[2];
            }

            mtxTrans = newBindPose * mtxGlobalTrans;


            float* pMtxTrans = reinterpret_cast<float*>(&mtxTrans);

            for (int i = 0; i < 16; i++)
            {
                if (fabs(pMtxTrans[i]) < SMALL_NUM_THRESHOLD)
                {
                    pMtxTrans[i] = 0.0f;
                }
            }

            //if (i == cnt - 1 && pLimbID)
            //{
            //    //PrintDebugProc("%d\n", *pLimbID);
            //    PrintDebugProc("%f, %f, %f, %f \n", pMtxTrans[0],
            //        pMtxTrans[1], pMtxTrans[2], pMtxTrans[3]);
            //    PrintDebugProc("%f, %f, %f, %f \n", mtxTrans.r[1].m128_f32[0],
            //        mtxTrans.r[1].m128_f32[1], mtxTrans.r[1].m128_f32[2], mtxTrans.r[1].m128_f32[3]);
            //    PrintDebugProc("%f, %f, %f, %f \n", mtxTrans.r[2].m128_f32[0],
            //        mtxTrans.r[2].m128_f32[1], mtxTrans.r[2].m128_f32[2], mtxTrans.r[2].m128_f32[3]);
            //    PrintDebugProc("%f, %f, %f, %f \n", mtxTrans.r[3].m128_f32[0],
            //        mtxTrans.r[3].m128_f32[1], mtxTrans.r[3].m128_f32[2], mtxTrans.r[3].m128_f32[3]);
            //}

            XMFLOAT4X4 finalTransform;
            XMStoreFloat4x4(&finalTransform, mtxTrans);
            boneFinalTransform.push_back(finalTransform);
        }

    }
}

XMFLOAT3 SkinnedMeshModel::GetAnimationValue(FbxNode** ppAnimationCurve, XMFLOAT3 defaultValue, uint64_t time, AnimationInfo& animInfo)
{
	if (ppAnimationCurve)
	{
		XMFLOAT3 animationValue;
		AnimationCurveNode* animationCurveNode = static_cast<AnimationCurveNode*>((*ppAnimationCurve)->nodeData);
		if (animationCurveNode)
		{
			FbxNode** animationCurveX = fbxNodes.search(animationCurveNode->dX);
			FbxNode** animationCurveY = fbxNodes.search(animationCurveNode->dY);
			FbxNode** animationCurveZ = fbxNodes.search(animationCurveNode->dZ);
			animationValue.x = GetAnimationCurveValue(animationCurveX, time, defaultValue.x, animInfo);
			animationValue.y = GetAnimationCurveValue(animationCurveY, time, defaultValue.y, animInfo);
			animationValue.z = GetAnimationCurveValue(animationCurveZ, time, defaultValue.z, animInfo);
			return animationValue;
		}
		else
			return defaultValue;

	}
	else
		return defaultValue;
}

float SkinnedMeshModel::GetAnimationCurveValue(FbxNode** ppAnimationCurveNode, uint64_t time, float defaultValue, AnimationInfo& animInfo)
{
	if (ppAnimationCurveNode)
	{
		AnimationCurve* animationCurve = static_cast<AnimationCurve*>((*ppAnimationCurveNode)->nodeData);
		if (animationCurve)
		{
            if (animationCurve->KeyAttrRefCount == 1)
                return animationCurve->KeyValue[0];
            else if (animationCurve->KeyAttrRefCount == 0)
                return 0;

            int keyCount = animationCurve->KeyAttrRefCount;
            uint64_t startTime = animationCurve->KeyTime[0];
            uint64_t endTime = animationCurve->KeyTime[keyCount - 1];

            if (animInfo.playMode == AnimPlayMode::LOOP)
            {
                if (time < startTime)
                {
                    time = endTime - static_cast<uint64_t>(fmod(startTime - time, endTime - startTime));
                }
                else if (time > endTime)
                {
                    time = static_cast<uint64_t>(fmod(time - startTime, endTime - startTime)) + startTime;
                }
            }
            else if (animInfo.playMode == AnimPlayMode::ONCE)
            {
                if (time > endTime)
                    time = endTime;
            }



			for (int i = 0; i < animationCurve->KeyAttrRefCount; i++)
			{
                if (time >= animationCurve->KeyTime[i] && time <= animationCurve->KeyTime[i + 1])
                {
                    float lerpPercent = static_cast<float>(time - animationCurve->KeyTime[i]) / 
                        ((static_cast<float>(animationCurve->KeyTime[i + 1]) - static_cast<float>(animationCurve->KeyTime[i])));

                    float v0 = animationCurve->KeyValue[i];
                    float v1 = animationCurve->KeyValue[i + 1];

                    float v = v0 + (v1 - v0) * lerpPercent;
                    return v;
                }

			}
			return defaultValue;
		}
		else
			return defaultValue;
	}
	else
		return defaultValue;
}

void SkinnedMeshModel::CalculateDrawParameters(ModelData* modelData, float startPercentage, float endPercentage, int& IndexNum, int& StartIndexLocation)
{

    StartIndexLocation = static_cast<int>(floor(startPercentage * modelData->IndexNum));
    StartIndexLocation = (StartIndexLocation / 3) * 3;

    int EndIndexLocation = static_cast<int>(ceil(endPercentage * modelData->IndexNum));
    EndIndexLocation = (EndIndexLocation / 3) * 3;

    IndexNum = EndIndexLocation - StartIndexLocation;
}

void SkinnedMeshModel::DrawSigewinne(ModelData* modelData)
{
    // ポリゴン描画
    int IndexNum, StartIndexLocation;

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, faceDiffuseTexture);
    if (faceLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, faceLightMapTexture);
    CalculateDrawParameters(modelData, 0, 0.001894f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, bodyDiffuseTexture);
    if (bodyLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, bodyDiffuseTexture);
    CalculateDrawParameters(modelData, 0.001894f, 0.00715f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, faceDiffuseTexture);
    if (faceLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, faceLightMapTexture);
    CalculateDrawParameters(modelData, 0.00715f, 0.047f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, hairDiffuseTexture);
    if (hairLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, hairLightMapTexture);
    CalculateDrawParameters(modelData, 0.047f, 0.44f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, bodyDiffuseTexture);
    if (bodyLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, bodyDiffuseTexture);
    CalculateDrawParameters(modelData, 0.44f, 0.916f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, faceDiffuseTexture);
    if (faceLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, faceLightMapTexture);
    CalculateDrawParameters(modelData, 0.916f, 0.996f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
}

void SkinnedMeshModel::DrawKlee(ModelData* modelData)
{
    // ポリゴン描画
    int IndexNum, StartIndexLocation;

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, hairDiffuseTexture);
    if (hairLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, hairLightMapTexture);
    CalculateDrawParameters(modelData, 0, 0.149f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, bodyDiffuseTexture);
    if (bodyLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, bodyDiffuseTexture);
    CalculateDrawParameters(modelData, 0.149f, 0.854f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, faceDiffuseTexture);
    if (faceLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, faceLightMapTexture);
    CalculateDrawParameters(modelData, 0.8628f, 0.9966f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
}

void SkinnedMeshModel::DrawLumine(ModelData* modelData)
{
    // ポリゴン描画
    int IndexNum, StartIndexLocation;

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, hairDiffuseTexture);
    if (hairLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, hairLightMapTexture);
    CalculateDrawParameters(modelData, 0, 0.1323f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, bodyDiffuseTexture);
    if (bodyLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, bodyDiffuseTexture);
    CalculateDrawParameters(modelData, 0.1323f, 0.847f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_DIFFUSE, faceDiffuseTexture);
    if (faceLightMapTexture)
        ShaderResourceBinder::get_instance().BindShaderResource(ShaderStage::PS, SLOT_TEX_LIGHT, faceLightMapTexture);
    CalculateDrawParameters(modelData, 0.847f, 1.0f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
}

void SkinnedMeshModel::DrawField(ModelData* modelData)
{
    Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &modelData->diffuseTexture);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(modelData->IndexNum, 0, 0);
}

void SkinnedMeshModel::DrawChurch(ModelData* modelData)
{
    int IndexNum, StartIndexLocation;

    Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Indoor_Mdprops_Church_Lights01_Diffuse);
    CalculateDrawParameters(modelData, 0, 0.262f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Flag_Diffuse);
    CalculateDrawParameters(modelData, 0.262f, 0.33f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

    Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Indoor_MdProps_Church_Item01_Diffuse);
    CalculateDrawParameters(modelData, 0.33f, 0.42f, IndexNum, StartIndexLocation);
    Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
}

void SkinnedMeshModel::DrawTownLoD(ModelData* modelData, int LoD)
{
    int IndexNum, StartIndexLocation;

    if (LoD == 0)
    {
        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Wall09_Diffuse);
        CalculateDrawParameters(modelData, 0, 0.25f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.25f, 0.79f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof05_Diffuse);
        CalculateDrawParameters(modelData, 0.79f, 0.81f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.81f, 0.84f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window50_Diffuse);
        CalculateDrawParameters(modelData, 0.84f, 0.92f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
    }
    else if (LoD == 1)
    {
        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof05_Diffuse);
        CalculateDrawParameters(modelData, 0, 0.006f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.006f, 0.105f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.105f, 0.115f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.115f, 0.123f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.123f, 0.126f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Wall09_Diffuse);
        CalculateDrawParameters(modelData, 0.126f, 0.13f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Wall07_Diffuse);
        CalculateDrawParameters(modelData, 0.13f, 0.1305f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof05_Diffuse);
        CalculateDrawParameters(modelData, 0.1305f, 0.1365f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.1365f, 0.2355f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.2355f, 0.2455f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.2455f, 0.2535f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.2535f, 0.2565f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Wall09_Diffuse);
        CalculateDrawParameters(modelData, 0.2565f, 0.2616f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof05_Diffuse);
        CalculateDrawParameters(modelData, 0.2616f, 0.2665f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.2665f, 0.3655f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.3655f, 0.3755f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.3755f, 0.3835f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.3835f, 0.3865f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Wall09_Diffuse);
        CalculateDrawParameters(modelData, 0.3865f, 0.391f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.391f, 0.39385f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof04_Diffuse);
        CalculateDrawParameters(modelData, 0.39385f, 0.39626f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.39626f, 0.471f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.471f, 0.483f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.483f, 0.4874f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.4874f, 0.4895f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof04_Diffuse);
        CalculateDrawParameters(modelData, 0.4895f, 0.49191f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.49191f, 0.56665f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.56665f, 0.57865f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.57865f, 0.58378f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.58378f, 0.6708f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.6708f, 0.672f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.672f, 0.6752f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.6752f, 0.677f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.677f, 0.67754f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof04_Diffuse);
        CalculateDrawParameters(modelData, 0.67754f, 0.67882f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Wall07_Diffuse);
        CalculateDrawParameters(modelData, 0.67882f, 0.682f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.682f, 0.6878f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);


        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Wood_Pillar_02_T2_Diffuse);
        CalculateDrawParameters(modelData, 0.6878f, 0.77482f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.77482f, 0.7761f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.7761f, 0.7793f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_ManorWall01_Diffuse);
        CalculateDrawParameters(modelData, 0.7793f, 0.7811f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_Mdbuild_Edge01_Diffuse);
        CalculateDrawParameters(modelData, 0.7811f, 0.78164f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Roof04_Diffuse);
        CalculateDrawParameters(modelData, 0.78164f, 0.78292f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_House_Wall07_Diffuse);
        CalculateDrawParameters(modelData, 0.78292f, 0.7861f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Window_A_Diffuse);
        CalculateDrawParameters(modelData, 0.7861f, 0.7919f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_Column01_Diffuse);
        CalculateDrawParameters(modelData, 0.7919f, 0.8339f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_CyTree02_Leaf_Diffuse);
        CalculateDrawParameters(modelData, 0.8339f, 0.843f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Tree04_Bark_Diffuse);
        CalculateDrawParameters(modelData, 0.843f, 0.845f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_CyTree02_Leaf_Diffuse);
        CalculateDrawParameters(modelData, 0.845f, 0.86f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Tree04_Bark_Diffuse);
        CalculateDrawParameters(modelData, 0.86f, 0.865f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_CyTree02_Leaf_Diffuse);
        CalculateDrawParameters(modelData, 0.865f, 0.878f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_Tree04_Bark_Diffuse);
        CalculateDrawParameters(modelData, 0.878f, 0.885f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Stages_CyTree02_Leaf_Diffuse);
        CalculateDrawParameters(modelData, 0.885f, 1.0f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
    }
    else if (LoD == 2)
    {
        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_All_Diffuse);
        CalculateDrawParameters(modelData, 0, 0.265f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdCity_Plot05_Diffuse);
        CalculateDrawParameters(modelData, 0.265f, 0.277f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdCity_Plot04_Diffuse);
        CalculateDrawParameters(modelData, 0.277f, 0.29f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdCity_Plot03_Diffuse);
        CalculateDrawParameters(modelData, 0.29f, 0.29576f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdCity_Plot02_Diffuse);
        CalculateDrawParameters(modelData, 0.29576f, 0.479485f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_All_Diffuse);
        CalculateDrawParameters(modelData, 0.479485f, 0.564f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdProps_Gadget02_Diffuse);
        CalculateDrawParameters(modelData, 0.59f, 0.60255f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdBuild_All_Diffuse);
        CalculateDrawParameters(modelData, 0.60255f, 0.986f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);

        Renderer::get_instance().GetDeviceContext()->PSSetShaderResources(0, 1, &Area_MdCity_Plot09_Diffuse);
        CalculateDrawParameters(modelData, 0.986f, 1.0f, IndexNum, StartIndexLocation);
        Renderer::get_instance().GetDeviceContext()->DrawIndexed(IndexNum, StartIndexLocation, 0);
    }
}


void SkinnedMeshModel::DrawBoundingBox(ModelData* modelData)
{
    Renderer::get_instance().SetFillMode(D3D11_FILL_WIREFRAME);

    int numBoundingBox = modelData->boundingBoxes.getSize();
    for (int i = 0; i < numBoundingBox; i++)
    {

        // 頂点バッファ設定
        UINT stride = sizeof(SKINNED_VERTEX_3D);
        UINT offset = 0;
        Renderer::get_instance().GetDeviceContext()->IASetVertexBuffers(0, 1, &modelData->boundingBoxes[i]->BBVertexBuffer, &stride, &offset);

        // プリミティブトポロジ設定
        Renderer::get_instance().GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        MATERIAL material;
        ZeroMemory(&material, sizeof(material));
        material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        material.noTexSampling = TRUE;
        Renderer::get_instance().SetMaterial(material);

        Renderer::get_instance().GetDeviceContext()->Draw(24, 0);
    }

    Renderer::get_instance().SetFillMode(D3D11_FILL_SOLID);
}

void SkinnedMeshModel::CreateBoundingBoxVertex(SKINNED_MESH_BOUNDING_BOX* boundingBox) const
{
    D3D11_MAPPED_SUBRESOURCE msr;
    Renderer::get_instance().GetDeviceContext()->Map(boundingBox->BBVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    SKINNED_VERTEX_3D* vertex = (SKINNED_VERTEX_3D*)msr.pData;
   
    for (int i = 0; i < 24; i++)
    {
        vertex[i].Weights = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
        vertex[i].BoneIndices = XMFLOAT4(static_cast<float>(boundingBox->boneIdx), 0.0f, 0.0f, 0.0f);
    }


    // 頂点座標の設定
    vertex[0].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[1].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[2].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);

    vertex[3].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[4].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);
    vertex[5].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);

    vertex[6].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);
    vertex[7].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);
    vertex[8].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);

    vertex[9].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);
    vertex[10].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);
    vertex[11].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);

    vertex[12].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);
    vertex[13].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);
    vertex[14].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);

    vertex[15].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->maxPoint.y, boundingBox->minPoint.z);
    vertex[16].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);
    vertex[17].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->maxPoint.y, boundingBox->maxPoint.z);

    vertex[18].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[19].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[20].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);

    vertex[21].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->minPoint.z);
    vertex[22].Position = XMFLOAT3(boundingBox->maxPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);
    vertex[23].Position = XMFLOAT3(boundingBox->minPoint.x, boundingBox->minPoint.y, boundingBox->maxPoint.z);


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

    Renderer::get_instance().GetDeviceContext()->Unmap(boundingBox->BBVertexBuffer, 0);
}

void SkinnedMeshModel::BuildMeshParts(void)
{
    m_meshParts.clear();
    for (const auto& it : meshDataMap)
    {
        SkinnedMeshPart part = {};
        part.VertexBuffer = it.value->VertexBuffer;
        part.IndexBuffer = it.value->IndexBuffer;
        part.IndexNum = it.value->IndexNum;
        m_meshParts.push_back(part);
    }
}
