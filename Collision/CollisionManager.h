//=============================================================================
//
// 八分木と動的衝突体による当たり判定管理 [CollisionManager.h]
// Author : 
// 静的オブジェクト（八分木）と動的衝突体の交差判定、
// 衝突イベントの発行・購読システムを統合的に管理する
// 
//=============================================================================
#pragma once
#include "Collision/OctreeNode.h"
#include "Utility/SimpleArray.h"
#include "Utility/SingletonBase.h"
#include "Model/Model.h"
//*****************************************************************************
// マクロ定義
//*****************************************************************************

// 衝突器の種類を表す列挙型
enum class ColliderType 
{
    DEFAULT,
    PLAYER,
    ENEMY,
    WALL,
    TREE,
    ITEM,
    STATIC_OBJECT,
    TELEPORTER,
    PLAYER_ATTACK,
    ENEMY_ATTACK,
};

//*****************************************************************************
// 構造体定義
//*****************************************************************************
class Collider 
{
public:
    BOUNDING_BOX aabb; // ワールド座標での衝突判定用
    ColliderType type; // 衝突器の種類
    void* owner;
    bool enable;

    Collider()
    {
        aabb = BOUNDING_BOX();
        type = ColliderType::DEFAULT;
        enable = false;
        owner = nullptr;
    }
};

// 衝突イベント構造体：衝突時に発行されるイベントメッセージ
struct CollisionEvent 
{
    const Collider* colliderA;
    const Collider* colliderB;
};

// コールバック関数
typedef void (*CollisionListener)(const CollisionEvent& event, void* context);

// 衝突イベントリスナーのエントリ構造体
struct CollisionListenerEntry 
{
    CollisionListener listener; // コールバック関数ポインタ
    void* context;              // 任意のコンテキスト情報（呼び出し側で利用可能）
};

// EventBus クラス：イベントを購読・発行するシンプルなメッセージシステム
class EventBus 
{
public:
    // 衝突イベントリスナーのリストを保持する
    SimpleArray<CollisionListenerEntry> collisionListeners;

    // 衝突イベントの購読を登録する（関数ポインタとコンテキストを登録）
    void subscribeCollision(CollisionListener listener, void* context) 
    {
        CollisionListenerEntry entry;
        entry.listener = listener;
        entry.context = context;
        collisionListeners.push_back(entry);
    }

    // 衝突イベントを発行する（登録された全リスナーに通知する）
    void publishCollision(const CollisionEvent& event) 
    {
        for (UINT i = 0; i < collisionListeners.getSize(); ++i) 
        {
            collisionListeners[i].listener(event, collisionListeners[i].context);
        }
    }
};

// CollisionManager クラス：静的オブジェクト（八分木）と動的オブジェクトの衝突管理
class CollisionManager : public SingletonBase<CollisionManager>
{
public:
    // 静的オブジェクト（例：地面モデル）の八分木
    OctreeNode* staticOctree = nullptr;

    // 動的オブジェクト（プレイヤー、敵、その他）の衝突体リスト
    SimpleArray<const Collider*> dynamicColliders;

    // イベントバスのインスタンス
    EventBus eventBus;

    void InitOctree(const BOUNDING_BOX& boundingBox);

    // 動的衝突体の登録
    void RegisterDynamicCollider(const Collider* collider)
    {
        dynamicColliders.push_back(collider);
    }

    // 動的衝突体リストのクリア
    void ClearDynamicColliders() 
    {
        dynamicColliders.clear();
    }

    void Update();

private:
    bool IsSelfCollision(const Collider* collider1, const Collider* collider2);

    //// 精密衝突判定関数（Collider と Triangle 間の衝突判定）
    //bool PreciseCollision(Collider* col, Triangle* tri);

    //// 精密衝突判定関数（動的 Collider 同士の衝突判定）
    //bool PreciseCollision(Collider* col1, Collider* col2);
};


