//=============================================================================
//
// 当たり判定処理 [CollisionManager.cpp]
// Author : 
//
//=============================================================================
#include "Collision/CollisionManager.h"
#include "Scene/GameObject.h"
#include "Scene/Enemy.h"
#include "Scene/Player.h"
#include "Utility/Debugproc.h"

constexpr float HEIGHT_EPSILON = 5.0f;
constexpr float SMOOTHING_FACTOR = 0.1f;
// 最大許容斜面角度（ラジアン、例：45度）
const float MAX_SLOPE_ANGLE = 0.785398f; // 約45度
// 階段として許容する最大高さ差（ステップオフセット）
const float STEP_OFFSET = 15.0f;

void CollisionManager::InitOctree(const BOUNDING_BOX& boundingBox)
{
    if (staticOctree == nullptr)
        staticOctree = new OctreeNode(boundingBox);
    else
    {
        delete staticOctree;
        staticOctree = new OctreeNode(boundingBox);
    }
}

void CollisionManager::Update()
{
    int numColliders = dynamicColliders.getSize();

    
    for (int i = 0; i < numColliders; i++)
    {
        const Collider* dynamicCol = dynamicColliders[i];

        if (dynamicCol->type == ColliderType::WALL ||
            dynamicCol->type == ColliderType::STATIC_OBJECT ||
            dynamicCol->enable == false) continue;

        // 動的オブジェクトと静的オブジェクト（地面）の衝突検出
        if (staticOctree)
        {
            auto colliderOwner = static_cast<GameObject<SkinnedMeshModelInstance>*>(dynamicCol->owner);
            Transform transform = colliderOwner->GetTransform();


            // 動的オブジェクトの AABB を元に、静的八分木から候補となる三角形をクエリする
            SimpleArray<Triangle*> candidates;
            staticOctree->queryRange(dynamicCol->aabb, candidates);

            bool hasCollision = false;
            bool hasValidSlope = false;
            bool isObstacle = false;
            float bestY = -FLT_MAX;
            float minHeightDiff = FLT_MAX;
            float minDistSq = FLT_MAX; // 最短距離
            float currentY = dynamicCol->aabb.minPoint.y;//transform.pos.y;
            XMVECTOR obstacleNormal{};
            // 候補となる各三角形に対して、精密な衝突判定を実施する
            int numCandidates = candidates.getSize();
            for (int j = 0; j < numCandidates; j++)
            {
                SimpleArray<float> hitYArray;
                if (dynamicCol->aabb.intersects(candidates[j]->aabb))
                {
                    hasCollision = true;
                    // 動的オブジェクトがプレイヤーと敵の場合のみ、斜面の高さ補正を計算する
                    if (dynamicCol->type == ColliderType::PLAYER ||
                        dynamicCol->type == ColliderType::ENEMY ||
                        dynamicCol->type == ColliderType::TREE)
                    {
                        //障害物の中心を求める
                        XMFLOAT3 obstacleCenter =
                        {
                            (candidates[j]->aabb.minPoint.x + candidates[j]->aabb.maxPoint.x) * 0.5f,
                            (candidates[j]->aabb.minPoint.y + candidates[j]->aabb.maxPoint.y) * 0.5f,
                            (candidates[j]->aabb.minPoint.z + candidates[j]->aabb.maxPoint.z) * 0.5f
                        };

                        // 障害物の法線 n
                        XMFLOAT3 n = candidates[j]->normal;
                        XMVECTOR currObstacleNormal = XMVectorSet(n.x, 0, n.z, 0);
                        currObstacleNormal = XMVector3Normalize(currObstacleNormal);

                        // 障害物中心との距離の平方を計算（X, Z 平面での距離のみ）
                        float distX = transform.pos.x - obstacleCenter.x;
                        float distZ = transform.pos.z - obstacleCenter.z;
                        float distSq = distX * distX + distZ * distZ;

                        // より近い障害物を見つけたら更新
                        if (distSq < minDistSq)
                        {
                            minDistSq = distSq;
                            obstacleNormal = currObstacleNormal;
                        }

                        // n.y が 0 に近い場合は、斜面判定を無視
                        if (fabs(n.y) < 0.001f)
                        {
                            isObstacle = true;
                            continue;
                        }

                        // 斜面角度を計算する（acos(n.y)；n は正規化済み）
                        float slopeAngle = acosf(n.y);

                        float posX = transform.pos.x;
                        float posZ = transform.pos.z;

                        // 候補となる三角形の基準点（v0）の高さを取得する
                        float baseY = candidates[j]->v0.y;

                        // 候補三角形のv0との水平方向の差を計算する
                        float deltaX = posX - candidates[j]->v0.x;
                        float deltaZ = posZ - candidates[j]->v0.z;

                        // 法線 n の x, z 成分に水平方向の差をかけ、合計する
                        float horizontalComponent = n.x * deltaX + n.z * deltaZ;

                        // colliderが平面上でどの高さに来るかを計算する
                        float heightAdjustment = horizontalComponent / n.y;

                        // 衝突面上のcolliderの y 座標（hitY）を求める
                        float candidateHitY = baseY - horizontalComponent / n.y;

                        float heightDiff = fabs(candidateHitY - currentY);
                        if (slopeAngle <= MAX_SLOPE_ANGLE || heightDiff < STEP_OFFSET)
                        {
                            // 斜面が許容範囲内の場合
                            if (heightDiff < minHeightDiff)
                            {
                                minHeightDiff = heightDiff;
                                //bestY = candidateHitY;
                            }
                            if (heightDiff < STEP_OFFSET || bestY == -FLT_MAX)
                                bestY = max(candidateHitY, bestY);
                            hasValidSlope = true;
                        }
                        else
                            isObstacle = true;
                    }

                    CollisionEvent collisionEvent;
                    collisionEvent.colliderA = dynamicCol;
                    // 静的オブジェクトは Triangle* で管理しているので、ここでは衝突対象の種類を WALL とする
                    static Collider dummyStatic;
                    dummyStatic.aabb = candidates[j]->aabb;
                    dummyStatic.type = ColliderType::WALL;
                    collisionEvent.colliderB = &dummyStatic;
                    eventBus.publishCollision(collisionEvent);
                }
            }


            if (dynamicCol->type == ColliderType::PLAYER ||
                dynamicCol->type == ColliderType::ENEMY)
            {
                if (hasCollision)
                {
                    if (hasValidSlope)
                    {
                        if (fabs(currentY - bestY) > HEIGHT_EPSILON)
                        {
                            // 現在のcolliderの位置を XMVECTOR に変換
                            XMVECTOR currentPos = XMLoadFloat3(&transform.pos);

                            // 目標のcolliderの位置は、x, z はそのままで y を hitY に設定する
                            XMVECTOR targetPos = XMVectorSet(transform.pos.x, bestY - 5.0f, transform.pos.z, 0.0f);

                            // XMVectorLerp を用いて、現在位置と目標位置の間を補間する
                            XMVECTOR lerpedPos = XMVectorLerp(currentPos, targetPos, SMOOTHING_FACTOR);

                            // 補間結果をcolliderの位置に戻す
                            XMStoreFloat3(&transform.pos, lerpedPos);

                            colliderOwner->SetTransform(transform);
                        }
                    }
                    if (isObstacle)
                    {
                        // 高い障害物の場合：移動ベクトルの調整（滑るように動く）

                        Attributes attributes = colliderOwner->GetAttributes();

                        // 意図する移動ベクトルの計算
                        float intendedMoveX = sinf(transform.rot.y) * attributes.spd;
                        float intendedMoveZ = cosf(transform.rot.y) * attributes.spd;

                        // 意図する移動ベクトルを DirectX の XMVECTOR に変換（Y成分は 0）
                        XMVECTOR intendedMove = XMVectorSet(intendedMoveX, 0, intendedMoveZ, 0);
                        intendedMove = XMVector3Normalize(intendedMove);  // 正規化

                        // 障害物との衝突がある場合、障害物法線（水平成分のみ）との内積を計算
                        float dot = XMVectorGetX(XMVector3Dot(intendedMove, obstacleNormal));

                        // dot > 0 なら、障害物方向に移動しようとしているので、その成分を除去
                        if (dot > 0.0f)
                        {
                            XMVECTOR projection = XMVectorScale(obstacleNormal, dot);
                            XMVECTOR allowedMove = XMVectorSubtract(intendedMove, projection);
                            allowedMove = XMVector3Normalize(allowedMove);

                            // 修正後の移動ベクトルに、元の移動スカラー（速度）をかける
                            float spd = attributes.spd; // 元の移動スカラー
                            XMVECTOR finalMove = XMVectorScale(allowedMove, spd);

                            // 最終的な移動量を取得して、transform.pos に加算する
                            XMFLOAT3 moveDelta;
                            XMStoreFloat3(&moveDelta, finalMove);
                            transform.pos.x += moveDelta.x;
                            transform.pos.z += moveDelta.z;
                        }
                        else
                        {
                            // dot が 0 以下の場合は、障害物方向には移動していないので、そのまま加算する
                            transform.pos.x += intendedMoveX;
                            transform.pos.z += intendedMoveZ;
                        }
                    }

                    colliderOwner->SetTransform(transform);
                }

                // isMoveBlockedの更新を行う
                bool isMoveBlocked = colliderOwner->GetAttributes().isMoveBlocked;
                if (isObstacle && !isMoveBlocked)
                {
                    colliderOwner->SetMoveBlock(true);
                }
                else if (!isObstacle && isMoveBlocked)
                {
                    colliderOwner->SetMoveBlock(false);
                }
                bool isGrounded = colliderOwner->GetAttributes().isGrounded;
                // 着地状態（isGrounded）の更新を行う
                if (hasCollision && !isGrounded)
                {
                    colliderOwner->SetGrounded(true);
                }
                else if (!hasCollision && isGrounded)
                {
                    colliderOwner->SetGrounded(false);
                }
            }


        }

        // 動的オブジェクト同士（例：プレイヤーと敵、その他）の衝突検出
        for (int j = i + 1; j < numColliders; j++)
        {
            const Collider* dynamicCol2 = dynamicColliders[j];

            if (dynamicCol2->enable == false) continue;

            else if (IsSelfCollision(dynamicCol, dynamicCol2)) continue;

            bool isObstacle = false;
            if (dynamicColliders[i]->aabb.intersects(dynamicColliders[j]->aabb))
            {
                if ((dynamicCol->type == ColliderType::ENEMY && dynamicCol2->type == ColliderType::PLAYER_ATTACK)
                    || (dynamicCol2->type == ColliderType::ENEMY && dynamicCol->type == ColliderType::PLAYER_ATTACK))
                {
                    const Collider* enemyCol = dynamicCol->type == ColliderType::ENEMY ? dynamicCol : dynamicCol2;
                    auto colliderOwner = static_cast<Enemy*>(enemyCol->owner);

                    if (colliderOwner->GetAttributes().hitTimer <= 0)
                    {
                        colliderOwner->SetHitTimer(ENEMY_HIT_WINDOW);
                        if (!colliderOwner->GetIsHit())
                        {
                            colliderOwner->ReduceHP(10.0f);
                            colliderOwner->SetIsHit(true);
                            colliderOwner->SetIsHit2(false);
                        }
                        else if (!colliderOwner->GetIsHit2())
                        {
                            colliderOwner->ReduceHP(10.0f);
                            colliderOwner->SetIsHit2(true);
                            colliderOwner->SetIsHit(false);
                        }
                    }
                }
                else if ((dynamicCol->type == ColliderType::PLAYER && dynamicCol2->type == ColliderType::ENEMY_ATTACK)
                    || (dynamicCol2->type == ColliderType::PLAYER && dynamicCol->type == ColliderType::ENEMY_ATTACK))
                {
                    const Collider* playerCol = dynamicCol->type == ColliderType::PLAYER ? dynamicCol : dynamicCol2;
                    auto colliderOwner = static_cast<GameObject<SkinnedMeshModelInstance>*>(playerCol->owner);

                    if (colliderOwner->GetAttributes().hitTimer <= 0)
                    {
                        colliderOwner->SetHitTimer(PLAYER_HIT_WINDOW);
                        if (!colliderOwner->GetIsHit())
                        {
                            colliderOwner->SetIsHit(true);
                        }
                    }
                }
                else
                {
                    isObstacle = true;

                    auto colliderOwner = static_cast<GameObject<SkinnedMeshModelInstance>*>(dynamicCol->owner);
                    Transform transform = colliderOwner->GetTransform();

                    // 障害物の AABB 情報
                    const BOUNDING_BOX& obstacleAABB = dynamicCol2->aabb;  // 衝突した障害物の AABB

                    // 各面との距離を計算
                    float distLeft = fabs(transform.pos.x - obstacleAABB.minPoint.x);
                    float distRight = fabs(transform.pos.x - obstacleAABB.maxPoint.x);
                    float distFront = fabs(transform.pos.z - obstacleAABB.minPoint.z);
                    float distBack = fabs(transform.pos.z - obstacleAABB.maxPoint.z);

                    // 最も近い面を特定し、推定法線を決定
                    XMFLOAT3 estimatedNormal = { 0, 0, 0 };

                    float minDist = distLeft;
                    estimatedNormal.x = 1.0f; // 法線向プレイヤー方向（プレイヤーが minX より左なら → 法線は右）

                    // 左右の面の距離が近い場合は、X 軸方向の成分を優先
                    if (distRight < minDist) 
                    {
                        minDist = distRight;
                        estimatedNormal.x = -1.0f; // プレイヤーが maxX より右 → 法線は左
                        estimatedNormal.z = 0.0f;
                    }

                    // 後ろの面の距離が近い場合は、Z 軸方向の成分を優先
                    if (distFront < minDist) 
                    {
                        minDist = distFront;
                        estimatedNormal.x = 0.0f;
                        estimatedNormal.z = 1.0f; // プレイヤーが minZ より前 → 法線は奥
                    }
                    if (distBack < minDist) 
                    {
                        minDist = distBack;
                        estimatedNormal.x = 0.0f;
                        estimatedNormal.z = -1.0f; // プレイヤーが maxZ より後ろ → 法線は手前
                    }

                    // 推定された法線を XMVECTOR に変換
                    XMVECTOR estimatedObstacleNormal = XMVectorSet(estimatedNormal.x, 0, estimatedNormal.z, 0);
                    estimatedObstacleNormal = XMVector3Normalize(estimatedObstacleNormal);

                    // 意図する移動ベクトルを計算（スカラー速度と回転角を使用）
                    Attributes attributes = colliderOwner->GetAttributes();
                    float intendedMoveX = sinf(transform.rot.y) * attributes.spd;
                    float intendedMoveZ = cosf(transform.rot.y) * attributes.spd;

                    // XMVECTOR に変換（Y 成分は 0）
                    XMVECTOR intendedMove = XMVectorSet(intendedMoveX, 0, intendedMoveZ, 0);
                    intendedMove = XMVector3Normalize(intendedMove);

                    // 内積を計算
                    float dot = XMVectorGetX(XMVector3Dot(intendedMove, estimatedObstacleNormal));

                    if (dot > 0.0f)
                    {
                        // 障害物方向の成分を除去
                        XMVECTOR projection = XMVectorScale(estimatedObstacleNormal, dot);
                        XMVECTOR allowedMove = XMVectorSubtract(intendedMove, projection);
                        allowedMove = XMVector3Normalize(allowedMove);

                        // 修正後の移動ベクトルに速度スカラーをかける
                        float spd = attributes.spd;
                        XMVECTOR finalMove = XMVectorScale(allowedMove, spd);

                        // 最終的な移動量を取得してプレイヤーの位置を更新
                        XMFLOAT3 moveDelta;
                        XMStoreFloat3(&moveDelta, finalMove);
                        transform.pos.x += moveDelta.x;
                        transform.pos.z += moveDelta.z;

                    }
                    else
                    {
                        // 障害物の方向ではないので、そのまま移動
                        transform.pos.x += intendedMoveX;
                        transform.pos.z += intendedMoveZ;
                    }

                    colliderOwner->SetTransform(transform);

                    // isMoveBlockedの更新を行う
                    bool isMoveBlocked = colliderOwner->GetAttributes().isMoveBlocked;
                    if (isObstacle && !isMoveBlocked)
                    {
                        colliderOwner->SetMoveBlock(true);
                    }
                    else if (!isObstacle && isMoveBlocked)
                    {
                        colliderOwner->SetMoveBlock(false);
                    }
                }

                CollisionEvent collisionEvent;
                collisionEvent.colliderA = dynamicColliders[i];
                collisionEvent.colliderB = dynamicColliders[j];
                eventBus.publishCollision(collisionEvent);
            }
        }
    }
}

bool CollisionManager::IsSelfCollision(const Collider* col1, const Collider* col2)
{

    return (col1->type == ColliderType::PLAYER && col2->type == ColliderType::PLAYER_ATTACK) ||
        (col2->type == ColliderType::PLAYER && col1->type == ColliderType::PLAYER_ATTACK) ||
        (col1->type == ColliderType::ENEMY && col2->type == ColliderType::ENEMY_ATTACK) ||
        (col2->type == ColliderType::ENEMY && col1->type == ColliderType::ENEMY_ATTACK);
}

//bool CollisionManager::PreciseCollision(Collider* col, Triangle* tri)
//{
//	return false;
//}
//
//bool CollisionManager::PreciseCollision(Collider* col1, Collider* col2)
//{
//	return false;
//}
