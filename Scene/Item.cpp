#include "Item.h"

Item::Item(ItemType itemType, Transform trans)
{
	m_itemAttributes.itemType = itemType;
	instance.use = true;		// true:生きてる

	m_itemAttributes.initTrans.pos = trans.pos;
	m_itemAttributes.initTrans.rot = trans.rot;
	m_itemAttributes.initTrans.scl = trans.scl;

	// ユニックID制作処理
	std::random_device rd;

	std::mt19937 gen(rd());

	// １から１００までのランダム数字制作
	std::uniform_int_distribution<> distrib(1, 100);

	int randomNumber = distrib(gen);

	//　IDは現在のチック×ランダム数字
	m_id = GetTickCount64() * randomNumber;

	// 遅延用コンテキスト構築
	auto* ctx = new InitContext{
		this,
	};
	DeferredTaskOptions options{};
	options.async = true; // 非同期実行
	options.debugName = "AsyncFbxModelLoader_Item"; // デバッグ用の名前
	// タスクを DeferredTaskSystem に登録
	RawDeferredTask::Add(
		ctx,
		[](void* ptr)
		{
			auto* ctx = static_cast<InitContext*>(ptr);
			ctx->owner->SetActive(true);
		},
		[](void* ptr)
		{
			auto* ctx = static_cast<InitContext*>(ptr);
			return ctx->owner->GetLoad();
		},
		options
	);
}

Item::~Item()
{
}

void Item::Initialize(void)
{

	instance.transform.pos = m_itemAttributes.initTrans.pos;
	instance.transform.rot = m_itemAttributes.initTrans.rot;
	instance.transform.scl = m_itemAttributes.initTrans.scl;

	instance.castShadow = false; // 影を落とす
}

void Item::Draw(void)
{
	GameObject::Draw();
}

void Item::Use(void)
{
	instance.collider.enable = false;
	m_itemAttributes.used = true;
}

void Item::SetActive(bool enable)
{
	instance.collider.enable = enable;
	instance.attributes.use = enable;
}

void Item::Interact()
{

}

void Item::Update(void)
{
	if (!instance.load)
	{
		GameObject::Update();

		if(instance.load)
			instance.collider.enable = true;

		return;
	}

	if (instance.use == true)		// このエネミーが使われている？
	{

		GameObject::Update();

		instance.transform.rot.y = 10 * timer.GetScaledDeltaTime();

	}

}
