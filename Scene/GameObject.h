#pragma once

//=============================================================================
//
// ゲームオブジェクト共通基底クラス群 [GameObject.h]
// Author : 
// モデル・当たり判定・Transform・描画・アニメーション制御などをまとめて管理する基底クラスちゃんですっ！
// キャラの共通インターフェース（ISkinnedMeshModelChar）もここにいて、アニメーションのきっかけを提供するの
// 
//=============================================================================
#include "Core/Graphics/Renderer.h"
#include "Model/Model.h"
#include "Model/SkinnedMeshModel.h"
#include "Collision/CollisionManager.h"
#include "Core/Timer.h"
#include "Core/Graphics/ShadowMeshCollector.h"
#include "Scene/Scene.h"
#include "Utility/DebugBoundingBoxRenderer.h"
#include "Core/Camera.h"

//*****************************************************************************
// マクロ定義
//*****************************************************************************
enum class ActionEnum
{
	NONE,
	ATTACK,
};

enum class ModelType
{
	Static,
	SkinnedMesh,
	Instanced,
};

#define DRAW_BOUNDING_BOX       1

//*****************************************************************************
// 構造体定義
//*****************************************************************************

struct Transform
{
	XMMATRIX			mtxWorld;			// ワールドマトリックス
	XMFLOAT3			pos;				// モデルの位置
	XMFLOAT3			rot;				// モデルの向き(回転)
	XMFLOAT3			scl;				// モデルの大きさ(スケール)

	Transform()
	{
		pos = XMFLOAT3(0.0f, 0.0f, 0.0f);
		rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
		scl = XMFLOAT3(1.0f, 1.0f, 1.0f);
		mtxWorld = XMMatrixIdentity();
	}
};

struct Attributes
{
	XMFLOAT4X4		mtxWorld;	// ワールドマトリックス

	bool			load;

	float			spd;		// 移動スピード
	float			dir;		// 向き
	float			targetDir;
	float			size;		// 当たり判定の大きさ
	bool			use;

	bool			stopRun;
	bool			isMoving;
	bool			isRotating;
	bool			isRunning;
	bool			isAttacking;
	bool			isAttacking2;
	bool			isAttacking3;
	bool			isDashing;
	bool			isJumping;
	bool			isGrounded;
	bool			isMoveBlocked;
	bool			isHit1;
	bool			isHit2;
	float			hitTimer;

	bool			charSwitchEffect;

	bool			attackWindow2;
	bool			attackWindow3;
	float			attackWinwdowCnt;

	Attributes()
	{
		load = TRUE;
		spd = 0.0f;
		dir = 0.0f;
		size = 0.0f;
		use = TRUE;
		targetDir = 0.0f;
		stopRun = TRUE;
		isMoving = FALSE;
		isRotating = FALSE;
		isRunning = FALSE;
		isAttacking = FALSE;
		isAttacking2 = FALSE;
		isAttacking3 = FALSE;
		isDashing = FALSE;
		isJumping = FALSE;
		isGrounded = FALSE;
		isMoveBlocked = FALSE;
		isHit1 = FALSE;
		isHit2 = FALSE;
		hitTimer = 0;

		attackWindow2 = FALSE;
		attackWindow3 = FALSE;
		attackWinwdowCnt = 0;

		charSwitchEffect = FALSE;

		mtxWorld = XMFLOAT4X4();
	}
};

struct InstanceModelAttribute
{
	bool			isInstanced = false;
	bool			isCollision = false;
	UINT			instanceCount = 0;
	InstanceData*	instanceData = nullptr;
	ID3D11Buffer*	instanceBuffer = nullptr;
	ID3D11Buffer*	vertexBuffer = nullptr;
	ID3D11Buffer*	indexBuffer = nullptr;
	SimpleArray<InstanceData>* visibleInstanceDataArray = nullptr;
	SimpleArray<bool>* visibleInstanceDraw = nullptr;
	SimpleArray<Collider>* colliderArray = nullptr;
};

struct SkinnedMeshModelInstance
{
	char*				modelPath = nullptr;
	char*				modelName = nullptr;
	char*				modelFullPath = nullptr;
	bool				use = false;
	bool				load = false;
	bool				castShadow = true;
	bool				enableAlphaTest = false;
	SkinnedMeshModel*	pModel = nullptr;
	Transform			transform;

	Attributes		attributes;
	Collider		collider;

	BOOL			isSelected = false;
	BOOL			isCursorIn = false;
	int				editorIdx = -1;

	bool			drawWorldAABB = false;
	ModelType		modelType = ModelType::SkinnedMesh;
	RenderProgressBuffer renderProgress;
};

struct ModelInstance
{
	char*				modelPath = nullptr;
	int					state;
	bool				use = false;
	bool				load = false;
	bool				castShadow = true;
	bool				enableAlphaTest = false;
	Model*				pModel = nullptr;				// モデル情報
	XMFLOAT4			diffuse[MODEL_MAX_MATERIAL];	// モデルの色
	Transform			transform;

	Attributes		attributes;
	Collider		collider;

	BOOL			isSelected = false;
	BOOL			isCursorIn = false;
	int				editorIdx = -1;

	bool			drawWorldAABB = false;
	ModelType		modelType = ModelType::Static;
	RenderProgressBuffer renderProgress;

	// インスタンス化されたモデルかどうか
	InstanceModelAttribute instanceAttribute;

};

// 非テンプレート基底クラス
class IGameObject
{
public:
	virtual ~IGameObject() = default;

	virtual bool GetUse() const = 0;
	virtual bool GetCastShadow() const = 0;
	virtual BOUNDING_BOX GetBoundingBoxWorld() const = 0;
	virtual const XMMATRIX& GetWorldMatrix() const = 0;

	virtual void CollectShadowMesh(ShadowMeshCollector& collector) const = 0;
	virtual ModelType GetModelType() const = 0;
};

template<typename TModel>
class GameObject : public IGameObject, public IDebugUI
{
public:
	GameObject();
	~GameObject();
	void Instantiate(char* modelPath);
	void Instantiate(char* modelPath, char* modelName, SkinnedModelType modelType = SkinnedModelType::Default, 
		AnimClipName clipName = AnimClipName::ANIM_NONE);
	virtual void Update();
	virtual void Draw();
	virtual void DrawEffect() {};
	virtual void RenderDebugInfo(void) override;
	void DrawModelEditor();
	void UpdateModelEditor();

	void CollectShadowMesh(ShadowMeshCollector& collector) const override
	{
		collector.Collect(instance);
	}

	inline const TModel* GetInstance() const { return &instance; }
	inline const TModel& GetInstanceRef() const { return instance; }
	inline InstanceModelAttribute& GetInstancedAttribute() { return instance.instanceAttribute; }
	inline Model* GetModel() { return instance.pModel; }
	inline SkinnedMeshModel* GetSkinnedMeshModel() { return instance.pModel; }
	inline const SkinnedMeshModel* GetSkinnedMeshModelConst() const { return instance.pModel; }
	inline void SetModelType(ModelType type) { instance.modelType = type; }
	inline ModelType GetModelType() const override { return instance.modelType; }
	inline void SetDrawWorldAABB(bool draw) { instance.drawWorldAABB = draw; }
	inline void SetPosition(XMFLOAT3 pos) { instance.transform.pos = pos; }
	inline void SetRotation(XMFLOAT3 rot) { instance.transform.rot = rot; }
	inline void SetScale(XMFLOAT3 scl) { instance.transform.scl = scl; }
	inline void SetTransform(Transform transform) { instance.transform = transform; }
	inline void SetWorldMatrix(XMMATRIX mtxWorld) { instance.transform.mtxWorld = mtxWorld; }
	inline Transform GetTransform() { return instance.transform; }
	inline const Transform* GetTransformP() { return &instance.transform; } // bind
	inline const XMMATRIX& GetWorldMatrix() const override { return instance.transform.mtxWorld; }


	inline XMFLOAT4* GetDiffuse() { return instance.diffuse; }
	inline bool GetUse() const override { return instance.use; }
	inline void SetUse(bool use) { instance.use = use; }
	inline bool GetCastShadow() const override { return instance.castShadow; }
	inline void SetCastShadow(bool shadow) { instance.castShadow = shadow; }
	inline void GetEnableAlphaTest(void) { return instance.enableAlphaTest; }
	inline void SetEnableAlphaTest(bool alpha) { instance.enableAlphaTest = alpha; }

	// インスタンス化されたモデルかどうか
	inline void SetIsInstanced(bool instanced) { instance.instanceAttribute.isInstanced = instanced; }
	inline bool GetIsInstanced() { return instance.instanceAttribute.isInstanced; }
	inline void SetInstanceCount(UINT count) { instance.instanceAttribute.instanceCount = count; }
	inline UINT GetInstanceCount() { return instance.instanceAttribute.instanceCount; }
	inline void SetInstanceBuffer(ID3D11Buffer* buffer) { instance.instanceAttribute.instanceBuffer = buffer; }
	inline ID3D11Buffer* GetInstanceBuffer() { return instance.instanceAttribute.instanceBuffer; }
	inline void SetVertexBuffer(ID3D11Buffer* buffer) { instance.instanceAttribute.vertexBuffer = buffer; }
	inline ID3D11Buffer* GetVertexBuffer() { return instance.instanceAttribute.vertexBuffer; }
	inline void SetIndexBuffer(ID3D11Buffer* buffer) { instance.instanceAttribute.indexBuffer = buffer; }
	inline ID3D11Buffer* GetIndexBuffer() { return instance.instanceAttribute.indexBuffer; }
	inline void SetInstanceData(InstanceData* data) { instance.instanceAttribute.instanceData = data; }
	inline const InstanceData* GetInstanceData() { return instance.instanceAttribute.instanceData; }
	inline SimpleArray<Collider>* GetInstancedColliderArray() { return instance.instanceAttribute.colliderArray; }
	inline void SetInstanceCollision(bool collision) { instance.instanceAttribute.isCollision = collision; }
	inline void InitializeInstancedArray() 
	{ 
		auto& ptr = instance.instanceAttribute.visibleInstanceDataArray;
		const UINT totalCount = instance.instanceAttribute.instanceCount;

		if (ptr == nullptr)
		{
			ptr = new SimpleArray<InstanceData>(totalCount);
		}
		else
		{
			ptr->clear();                     // 前のフレームのデータをリセット
			ptr->reserve(totalCount);         // 必要に応じて拡張
		}

		instance.instanceAttribute.visibleInstanceDraw = new SimpleArray<bool>(totalCount, false);
		instance.instanceAttribute.colliderArray = new SimpleArray<Collider>(totalCount, Collider());

		if (instance.instanceAttribute.isCollision)
		{
			for (UINT i = 0; i < totalCount; i++)
			{
				Collider* collider = &(*instance.instanceAttribute.colliderArray)[i];
				collider->type = ColliderType::STATIC_OBJECT;
				collider->owner = this;
				collider->enable = true;
				CollisionManager::get_instance().RegisterDynamicCollider(collider);
			}
		}

	}


	inline BOOL GetIsModelSelected() { return instance.isSelected; }
	inline BOOL GetIsCursorIn() { return instance.isCursorIn; }
	inline void SetIsCursorIn(BOOL cursorIn) { instance.isCursorIn = cursorIn; }
	inline int GetEditorIndex() { return instance.editorIdx; }
	inline void SetEditorIndex(int idx) { instance.editorIdx = idx; }
	inline const Attributes& GetAttributes() { return instance.attributes; }
	inline void UpdateAttributes(Attributes attributes) { instance.attributes = attributes; }

	// 物理演算用の当たり判定
	inline void SetColliderType(ColliderType type) { instance.collider.type = type; }
	inline void SetColliderOwner(void* owner) { instance.collider.owner = owner; }
	inline void SetColliderEnable(bool enable) { instance.collider.enable = enable; }
	inline void SetColliderBoundingBox(BOUNDING_BOX boundingBox) { instance.collider.aabb = boundingBox; }
	inline const Collider& GetCollider(void) { return instance.collider; }
	inline BOUNDING_BOX GetBoundingBoxWorld(void) const override { return instance.collider.aabb; }


	inline void SetGrounded(bool grounded) { instance.attributes.isGrounded = grounded; }
	inline void SetMoveBlock(bool blocked) { instance.attributes.isMoveBlocked = blocked; }
	inline void SetIsHit(bool isHit) { instance.attributes.isHit1 = isHit; }
	inline bool GetIsHit(void) { return instance.attributes.isHit1; }
	inline void SetIsHit2(bool isHit) { instance.attributes.isHit2 = isHit; }
	inline bool GetIsHit2(void) { return instance.attributes.isHit2; }
	inline int	GetHitTimer(void) { return instance.attributes.hitTimer; }
	inline void SetHitTimer(float hitTimer) { instance.attributes.hitTimer = hitTimer; }
	//inline void SetRenderProgress(float progress) { instance.attributes.renderProgress.progress = progress; }
	//inline void SetSwitchCharEffect(bool effectON) { instance.attributes.charSwitchEffect = effectOn; }
	virtual AnimStateMachine* GetStateMachine() { return nullptr; }

	void SetBoundingboxSize(XMFLOAT3 size)
	{
		UINT numBones = instance.pModel.GetNumBones();
		for (int i = 0; i < numBones; i++)
		{
			instance.pModel.SetBoundingBoxSize(size, i);
		}
	}

	Renderer& renderer = Renderer::get_instance();
	Timer& timer = Timer::get_instance();

protected:
	TModel instance;
	DebugBoundingBoxRenderer m_debugBoundingBoxRenderer;
};

class ISkinnedMeshModelChar
{
public:
	virtual void PlayWalkAnim() {};
	virtual void PlayRunAnim() {};
	virtual void PlayIdleAnim() {};
	virtual void PlayStandingAnim() {};
	virtual void PlayAttackAnim() {};
	virtual void PlayHitAnim() {};
	virtual void PlayDashAnim() {}
	virtual void PlayJumpAnim() {}
	virtual void PlaySurprisedAnim() {}
	virtual void InitAnimInfo() {}

	virtual bool CanWalk() const = 0;
	virtual bool CanStopMoving() const = 0;
	virtual bool CanStopRunning() const { return false; }
	virtual bool CanAttack() const = 0;
	virtual bool CanAttack2() const  { return false; }
	virtual bool CanAttack3() const  { return false; }
	virtual bool CanRun()	const = 0;
	virtual bool CanHit()	const = 0;
	virtual bool CanHit2()	const { return false; }
	virtual bool CanJump()	const { return false; }
	virtual bool CanSurprised() const { return false; }
	virtual bool CanDie()	const { return false; }
	virtual void OnAttackAnimationEnd() {};
	virtual void OnHitAnimationEnd() {};
	virtual void OnDashAnimationEnd() {}
	virtual void OnJumpAnimationEnd() {}
	virtual void OnDieAnimationEnd() {}
	virtual void OnSurprisedEnd() {}
	virtual bool ExecuteAction(ActionEnum action) { return true; }
	bool AlwaysTrue() const { return true; }
};


template <typename T>
GameObject<T>::GameObject()
{
	Scene::get_instance().RegisterGameObject(this);

#ifdef _DEBUG
	DebugProc::get_instance().Register(this);
#endif // DEBUG

#if DRAW_BOUNDING_BOX
	m_debugBoundingBoxRenderer.Initialize();
#endif
}

template <typename T>
GameObject<T>::~GameObject()
{
	if (instance.pModel)
	{
		instance.load = false;
		MODEL_POOL* modelPool = Model::GetModel(instance.modelPath);
		if (modelPool)
		{
			modelPool->count--;
			if (modelPool->count == 0)
			{
				delete modelPool->pModel;
				Model::RemoveModel(instance.modelPath);
			}
		}
	}

	Scene::get_instance().UnregisterGameObject(this);
}

template <>
GameObject<SkinnedMeshModelInstance>::~GameObject();

template <typename T>
void GameObject<T>::Instantiate(char* modelPath)
{
	instance.modelPath = modelPath;
	instance.pModel = Model::StoreModel(modelPath);
	instance.load = true;
	instance.use = true;
}

template <>
void GameObject<SkinnedMeshModelInstance>::Instantiate(char* modelPath, char* modelName, 
	SkinnedModelType modelType, AnimClipName clipName);

template<>
void GameObject< SkinnedMeshModelInstance>::Update();

template <typename T>
void GameObject<T>::Update()
{
	XMMATRIX mtxScl, mtxRot, mtxTranslate, mtxWorld;

	// ワールドマトリックスの初期化
	mtxWorld = XMMatrixIdentity();

	// スケールを反映
	mtxScl = XMMatrixScaling(instance.transform.scl.x, instance.transform.scl.y, instance.transform.scl.z);
	mtxWorld = XMMatrixMultiply(mtxWorld, mtxScl);

	// 回転を反映
	mtxRot = XMMatrixRotationRollPitchYaw(instance.transform.rot.x, instance.transform.rot.y + XM_PI, instance.transform.rot.z);
	mtxWorld = XMMatrixMultiply(mtxWorld, mtxRot);

	// 移動を反映
	mtxTranslate = XMMatrixTranslation(instance.transform.pos.x, instance.transform.pos.y, instance.transform.pos.z);
	mtxWorld = XMMatrixMultiply(mtxWorld, mtxTranslate);

	instance.transform.mtxWorld = mtxWorld;

		// ローカル空間AABBの取得
	XMFLOAT3 localMin = instance.pModel->GetBoundingBox().minPoint;
	XMFLOAT3 localMax = instance.pModel->GetBoundingBox().maxPoint;

	// 8つのコーナー頂点を作成（ローカル空間）
	XMVECTOR localCorners[8] = {
		XMVectorSet(localMin.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMin.z, 1.0f),
		XMVectorSet(localMin.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMin.y, localMax.z, 1.0f),
		XMVectorSet(localMin.x, localMax.y, localMax.z, 1.0f),
		XMVectorSet(localMax.x, localMax.y, localMax.z, 1.0f),
	};

	// ワールド空間AABBの初期化
	XMVECTOR worldMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
	XMVECTOR worldMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1.0f);

	// 全ての角をスキン変換→ワールド変換→AABB更新
	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR worldPt = XMVector3Transform(localCorners[i], instance.transform.mtxWorld);
		worldMin = XMVectorMin(worldMin, worldPt);
		worldMax = XMVectorMax(worldMax, worldPt);
	}

	// AABBをコライダーに格納（ワールド空間）
	XMStoreFloat3(&instance.collider.aabb.minPoint, worldMin);
	XMStoreFloat3(&instance.collider.aabb.maxPoint, worldMax);
}


template <typename T>
void GameObject<T>::Draw()
{
	// ワールドマトリックスの設定
	renderer.SetCurrentWorldMatrix(&instance.transform.mtxWorld);

	instance.pModel->DrawModel();
}


template<typename TModel>
inline void GameObject<TModel>::RenderDebugInfo(void)
{
#if DRAW_BOUNDING_BOX
	if (instance.drawWorldAABB)
	{
		const BOUNDING_BOX& box = instance.collider.aabb;

		XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		m_debugBoundingBoxRenderer.DrawBox(box, Camera::get_instance().GetViewProjMtx(), color);
	}
#endif
}

template<typename T>
inline void GameObject<T>::DrawModelEditor()
{
	if (instance.isCursorIn == TRUE)
	{
		renderer.SetFillMode(D3D11_FILL_WIREFRAME);
		instance.pModel->DrawModel();
		renderer.SetFillMode(D3D11_FILL_SOLID);
	}
	else
	{
		instance.pModel->DrawModel();
	}
}

template<typename T>
inline void GameObject<T>::UpdateModelEditor()
{
	if (instance.isCursorIn == FALSE) return;

	if (IsMouseLeftTriggered())
	{
		instance.isSelected = instance.isSelected == TRUE ? FALSE : TRUE;
		if (instance.isSelected == TRUE)
			MapEditor::get_instance().SetCurSelectedModelIdx(instance.editorIdx);
		else
			MapEditor::get_instance().ResetCurSelectedModelIdx();
	}
}

