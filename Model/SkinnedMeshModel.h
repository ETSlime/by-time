#pragma once
//=============================================================================
//
// スキン付きメッシュモデル管理クラスちゃん [SkinnedMeshModel.h]
// Author : 
// 骨やアニメーションを制御して、ぷにぷに動くキャラモデルを描画するえらい子ですっ
// モデルの読込・アニメ再生・ボーン行列の生成から八分木構築まで全部まかせて安心なのです
// 
//=============================================================================
#include "Model/FBXLoader.h"
#include "AI/AnimStateMachine.h"
#include "Collision/OctreeNode.h"
#include "Core/Timer.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************
#define MAX_MESH_NUM			64
#define MAX_ANIM_NUM			32
#define MODEL_NAME_LENGTH		64
#define MODEL_PATH_LENGTH		128
#define ANIM_SPD				(1539538600 * 0.55f)
enum VertexDataLocation
{
	Index,
	Vertex,
};

//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct PosNormalTexTanSkinned
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;
	XMFLOAT4 TangentU;
};

struct SkinnedMeshModelPool
{
	SkinnedMeshModel* pModel;
	unsigned int count;

	SkinnedMeshModelPool()
	{
		pModel = nullptr;
		count = 0;
	}

	void AddRef() { count++; }
};

struct BoneTransformData
{
	SimpleArray<XMFLOAT4X4> mModelGlobalRot;
	SimpleArray<XMFLOAT4X4> mModelGlobalScl;
	SimpleArray<XMFLOAT4X4> mModelTranslate;
	SimpleArray<XMFLOAT4X4> mModelGlobalTrans;
	SimpleArray<XMFLOAT4X4> mModelLocalTrans;
	SimpleArray<XMFLOAT4X4> mBoneFinalTransforms;

	HashMap<uint64_t, int, HashUInt64, EqualUInt64> limbHashMap = HashMap<uint64_t, int, HashUInt64, EqualUInt64>(
		MAX_NODE_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	BoneTransformData(int numBones)
	{
		mModelGlobalRot.resize(numBones);
		mModelGlobalScl.resize(numBones);
		mModelTranslate.resize(numBones);
		mModelGlobalTrans.resize(numBones);
		mModelLocalTrans.resize(numBones);
		mBoneFinalTransforms.resize(numBones);
	}

	BoneTransformData()
	{
		mModelGlobalRot.resize(0);
		mModelGlobalScl.resize(0);
		mModelTranslate.resize(0);
		mModelGlobalTrans.resize(0);
		mModelLocalTrans.resize(0);
		mBoneFinalTransforms.resize(0);
	}
};

struct ModelData
{
	ModelData()
	{
		mesh = new MeshData();
		shapeCnt = 0;
		limbCnt = 0;
		VertexArray = nullptr;
		VertexNum = 0;
		IndexArray = nullptr;
		IndexNum = 0;

		armatureNode = nullptr;

		VertexBuffer = nullptr;
		IndexBuffer = nullptr;

		material = nullptr;

		diffuseTexture = nullptr;
		emissiveTexture = nullptr;

		modelID = modelCnt;
		modelCnt++;

	}

	~ModelData()
	{
		SAFE_DELETE(mesh);
		SAFE_DELETE(VertexArray);
		SAFE_DELETE(IndexArray);

		SafeRelease(&VertexBuffer);
		SafeRelease(&IndexBuffer);

		for (UINT i = 0; i < boundingBoxes.getSize(); i++)
		{
			SAFE_DELETE(boundingBoxes[i]);
		}

	}

	MeshData*				mesh;
	SimpleArray<MeshData>	shapes;
	SimpleArray<Subset> Subsets;

	int shapeCnt;
	int limbCnt;
	int modelID;

	static int modelCnt;

	SKINNED_VERTEX_3D* VertexArray;
	unsigned int	VertexNum;
	unsigned int* IndexArray;
	unsigned int	IndexNum;

	FbxNode* armatureNode;

	SimpleArray<int> mBoneHierarchy;
	SimpleArray<XMFLOAT4X4> mBoneOffsets;
	SimpleArray<XMFLOAT4X4> mBoneToParentTransforms;
	SimpleArray<SKINNED_MESH_BOUNDING_BOX*> boundingBoxes;
	SimpleArray<Triangle*> triangles;
	VertexDataLocation normalLoc;
	VertexDataLocation texLoc;

	FbxMaterial* material;

	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;

	ID3D11ShaderResourceView* diffuseTexture;
	ID3D11ShaderResourceView* emissiveTexture;
};

struct SkinnedMeshPart
{
	ID3D11Buffer* VertexBuffer = nullptr;
	ID3D11Buffer* IndexBuffer = nullptr;
	ID3D11ShaderResourceView* OpacityTexture = nullptr;
	UINT IndexNum = 0;
};

class SkinnedMeshModel
{
public:
	friend class FBXLoader;

	void DrawModel();
	void UpdateBoneTransform(SimpleArray<XMFLOAT4X4>* boneTransforms);

	SkinnedMeshModel();
	~SkinnedMeshModel();

	void SetBodyDiffuseTexture(char* texturePath);
	void SetBodyLightMapTexture(char* texturePath);
	void SetBodyNormalMapTexture(char* texturePath);
	void SetHairDiffuseTexture(char* texturePath);
	void SetHairLightMapTexture(char* texturePath);
	void SetFaceDiffuseTexture(char* texturePath);
	void SetFaceLightMapTexture(char* texturePath);

	void LoadTownTexture(void);

	void GetBoneTransformByAnim(FbxNode* currentClipArmatureNode, uint64_t currentClipTime, 
		SimpleArray<XMFLOAT4X4>* boneFinalTransform, AnimationInfo& animInfo);

	void SetBoundingBoxLocationOffset(XMFLOAT3 offset, int boneIdx = 0);
	void SetBoundingBoxSize(XMFLOAT3 size, int boneIdx = 0);
	void SetDrawBoundingBox(bool draw) { drawBoundingBox = draw; }
	void BuildTrianglesByWorldMatrix(XMMATRIX worldMatrix, bool alwaysFaceUp = false);
	void BuildTrianglesByBoundingBox(BOUNDING_BOX bb);
	bool BuildOctree(void);
	UINT GetNumBones(void) { return numBones; }
	BOUNDING_BOX GetBoundingBox(void) const { return boundingBox; }
	const SimpleArray<Triangle*>* GetTriangles(void) const;

	void SetCurrentAnim(AnimationClip* currAnimClp, float startTime = 0);
	AnimClipName GetCurrentAnim(void) { return currentAnimClip->name; }
	AnimationClip* GetAnimationClip(AnimClipName clipName);
	void PlayCurrentAnim(float playSpeed = 1.0f);

	XMMATRIX GetWeaponTransformMtx(void);
	XMMATRIX GetBodyTransformMtx(void);
	XMMATRIX GetBoneFinalTransform(UINT boneIdx = 0);
	const XMMATRIX* GetFinalBoneMatrices(void);

	const SimpleArray<SkinnedMeshPart>& GetMeshParts() const;
	SkinnedModelType GetModelType(void) const { return modelType; }

	static SkinnedMeshModel* StoreModel(char* modelPath, char* modelName, char* modelFullPath, 
		SkinnedModelType modelType, AnimClipName clipName);
	static SkinnedMeshModelPool* GetModel(char* modelFullPath);
	static void RemoveModel(char* modelPath);


private:

	static HashMap<char*, SkinnedMeshModelPool, CharPtrHash, CharPtrEquals> modelHashMap;

	void UpdateLimbGlobalTransform(FbxNode* node, FbxNode* deformNode, int& curIdx, int prevIdx, uint64_t time, BoneTransformData* boneTransformData, AnimationInfo& animInfo);
	void GetBoneTransform(SimpleArray<XMFLOAT4X4>& boneFinalTransform, ModelData* modelData);
	XMFLOAT3 GetAnimationValue(FbxNode** ppAnimationCurve, XMFLOAT3 defaultValue, uint64_t time, AnimationInfo& animInfo);
	float GetAnimationCurveValue(FbxNode** ppAnimationCurveNode, uint64_t time, float defaultValue, AnimationInfo& animInfo);
	void CalculateDrawParameters(ModelData* modelData, float startPercentage, float endPercentage, int& IndexNum, int& StartIndexLocation);

	void DrawSigewinne(ModelData* modelData);
	void DrawKlee(ModelData* modelData);
	void DrawLumine(ModelData* modelData);
	void DrawField(ModelData* modelData);
	void DrawChurch(ModelData* modelData);
	void DrawTownLoD(ModelData* modelData, int LoD);

	void DrawBoundingBox(ModelData* modelData);
	void CreateBoundingBoxVertex(SKINNED_MESH_BOUNDING_BOX* boundingBox) const;

	void BuildMeshParts(void);

	char modelPath[MODEL_PATH_LENGTH];
	char modelName[MODEL_NAME_LENGTH];

	UINT ModelCount;
	UINT MeshDataCnt = 0;
	UINT currentRootNodeID;
	UINT numBones;
	SkinnedModelType modelType;
	bool drawBoundingBox = false;
	BoneTransformData boneTransformData;
	UINT weaponTransformIdx;

	AnimationClip* currentAnimClip;
	FbxNode* armatureNode;

	ModelProperty modelProperty;
	ModelProperty globalModelProperty;
	FbxMaterial globalMaterial;
	CULL_MODE cullMode;

	BOUNDING_BOX boundingBox;
	BindPose bindPose;

	XMMATRIX m_transposedBoneMatrices[BONE_MAX];

	mutable SimpleArray<SkinnedMeshPart> m_meshParts;

	ID3D11ShaderResourceView* bodyDiffuseTexture = nullptr;
	ID3D11ShaderResourceView* bodyLightMapTexture = nullptr;
	ID3D11ShaderResourceView* bodyNormalMapTexture = nullptr;
	ID3D11ShaderResourceView* hairDiffuseTexture = nullptr;
	ID3D11ShaderResourceView* hairLightMapTexture = nullptr;
	ID3D11ShaderResourceView* faceDiffuseTexture = nullptr;
	ID3D11ShaderResourceView* faceLightMapTexture = nullptr;

	ID3D11ShaderResourceView* Area_Mdcity_Lvy01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdCity_Plot02_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdCity_Plot03_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdCity_Plot04_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdCity_Plot05_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdCity_Plot09_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_Mdbuild_Wall06_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_OutDoor_MDSkyBox = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_KnightHQ02_Assembly01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_All_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_Mdbuild_Edge01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Wall09_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Column01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_House_Roof04_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_House_Roof05_Diffuse = nullptr;
	ID3D11ShaderResourceView* Stages_Wood_Pillar_02_T2_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdProps_Gadget02_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Window50_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Window30_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Window_A_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_Mdbuild_ManorWall01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_House_Wall07_Diffuse = nullptr;
	ID3D11ShaderResourceView* Stages_CyTree02_Leaf_Diffuse = nullptr;
	ID3D11ShaderResourceView* Stages_Tree04_Bark_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Wall08_Diffuse = nullptr;

	ID3D11ShaderResourceView* Area_MdBuild_Church01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Church02_Diffuse = nullptr;
	ID3D11ShaderResourceView* Area_MdBuild_Flag_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_Church_Ground01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_Church_GroundPattern01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_Church_Stairs01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_Church_Wall01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_Church_Wall02_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_Mdprops_Church_Squate02_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdProps_Church_Item01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_Mdprops_Church_Lights01_Diffuse = nullptr;
	ID3D11ShaderResourceView* Indoor_MdBuild_WindowEffect04_NoStream = nullptr;

	HashMap<uint64_t, FbxNode*, HashUInt64, EqualUInt64> fbxNodes = 
		HashMap<uint64_t, FbxNode*, HashUInt64, EqualUInt64>(
		MAX_NODE_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	HashMap<uint64_t, ModelData*, HashUInt64, EqualUInt64> meshDataMap = 
		HashMap<uint64_t, ModelData*, HashUInt64, EqualUInt64>(
		MAX_MESH_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	HashMap<int, AnimationClip, HashUInt64, EqualUInt64> animationClips = 
		HashMap<int, AnimationClip, HashUInt64, EqualUInt64>(
		MAX_ANIM_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	HashMap<int, uint64_t, HashUInt64, EqualUInt64> deformerHashMap = 
		HashMap<int, uint64_t, HashUInt64, EqualUInt64>(
		MAX_NODE_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	HashMap<uint64_t, int, HashUInt64, EqualUInt64> deformerIdxHashMap = 
		HashMap<uint64_t, int, HashUInt64, EqualUInt64>(
		MAX_NODE_NUM,
		HashUInt64(),
		EqualUInt64()
	);

	HashMap<uint64_t, uint64_t, HashUInt64, EqualUInt64> deformerToLimb = 
		HashMap<uint64_t, uint64_t, HashUInt64, EqualUInt64>(
		MAX_NODE_NUM,
		HashUInt64(),
		EqualUInt64()
	);
};