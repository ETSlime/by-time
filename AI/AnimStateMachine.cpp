//=============================================================================
//
// AnimStateMachine処理 [AnimStateMachine.cpp]
// Author : 
//
//=============================================================================
#include "AI/AnimStateMachine.h"
#include "Model/SkinnedMeshModel.h"
#include "Utility/Debugproc.h"
#include "Scene/GameObject.h"
SimpleArray<XMFLOAT4X4>* AnimationClip::GetBoneMatrices(SimpleArray<XMFLOAT4X4>* currBoneTransform)
{
    if (model)
    {
        model->GetBoneTransformByAnim(armatureNode, currentTime, currBoneTransform, animInfo);
        return currBoneTransform;
    }
    else
        return nullptr;
}

void AnimationState::StartBlend(AnimationClip* prevClip)
{
    this->prevClip = prevClip; // 旧アニメーションを保存
    blendFactor = 0.0f; // ブレンド開始
}

void AnimationState::Update(float deltaTime)
{
    if (blendFactor < 1.0f)
    {
        blendFactor += deltaTime * 2.0f;
    }
    if (blendFactor > 1.0f) blendFactor = 1.0f;

    UpdateBlendedMatrix();
}

NextStateInfo AnimationState::GetNextState(ISkinnedMeshModelChar* character)
{
    for (const auto& transition : transitions) 
    {
        if (transition.value.condition && (character->*transition.value.condition)())
        {
            if (currentClip && currentClip->IsFinished())
            {
                if (onEndCallback)
                    (character->*onEndCallback)();
                return NextStateInfo(transition.key, transition.value.animBlend);
            }
            else if (!transition.value.waitForAnimationEnd) 
            {
                return NextStateInfo(transition.key, transition.value.animBlend);
            }
        }
    }
    return NextStateInfo(currentStateName, false); // 変更なし
}

void AnimationState::AddTransition(uint64_t currentState, uint64_t nextState,
    bool (ISkinnedMeshModelChar::* condition)() const, bool waitForAnimationEnd, bool animBlend)
{
    transitions[nextState] = { currentState, nextState, condition, waitForAnimationEnd, animBlend };
}

void AnimationState::SetEndCallback(void (ISkinnedMeshModelChar::* callback)())
{
    onEndCallback = callback;
}

float AnimationState::GetCurrentAnimTime(void)
{
    if (currentClip->stopTime > 0)
        return static_cast<float>(static_cast<double>(currentClip->currentTime) / currentClip->stopTime);
    
    return 0.0f;
}

void AnimationState::UpdateBlendedMatrix(void)
{
    if (!currentClip) return;  // アニメーションがない場合は空リストを返す

    SimpleArray<XMFLOAT4X4>* prevMatrices = nullptr;
    SimpleArray<XMFLOAT4X4>* currMatrices = nullptr;

    currMatrices = currentClip->GetBoneMatrices(&currentClip->currBoneTransform);
    int boneCount; 
    boneCount = currMatrices->getSize();

    blendedMatrices.clear();

    if (!prevClip || blendFactor >= 1.0f)// || ) blendOn == false)
    {
        // 前のアニメーションがない場合、またはブレンド完了時
        for (int i = 0; i < boneCount; i++)
        {
            blendedMatrices.push_back((*currMatrices)[i]);
        }

        return;
    }

    prevMatrices = prevClip->GetBoneMatrices(&prevClip->currBoneTransform);

    for (int i = 0; i < boneCount; i++)
    {
        // 各ボーンの変換行列を分解（縮小、回転、平行移動）
        XMVECTOR prevScale, prevRot, prevTrans;
        XMVECTOR currScale, currRot, currTrans;
        XMMATRIX prevMtx = XMLoadFloat4x4(&(*prevMatrices)[i]);
        XMMATRIX currMtx = XMLoadFloat4x4(&(*currMatrices)[i]);

        XMMatrixDecompose(&prevScale, &prevRot, &prevTrans, prevMtx);
        XMMatrixDecompose(&currScale, &currRot, &currTrans, currMtx);

        // 位置（平行移動）の補間
        XMVECTOR blendedTrans = XMVectorLerp(prevTrans, currTrans, blendFactor);

        // 回転（四元数）の補間
        XMVECTOR blendedRot = XMQuaternionSlerp(prevRot, currRot, blendFactor);

        // 拡大縮小の補間
        XMVECTOR blendedScale = XMVectorLerp(prevScale, currScale, blendFactor);

        // ブレンド後の行列を再構築
        XMMATRIX blendedMtx = XMMatrixScalingFromVector(blendedScale) *
            XMMatrixRotationQuaternion(blendedRot) *
            XMMatrixTranslationFromVector(blendedTrans);

        XMFLOAT4X4 blended;
        XMStoreFloat4x4(&blended, blendedMtx);
        blendedMatrices.push_back(blended);
    }
}

SimpleArray<XMFLOAT4X4>* AnimationState::GetBlendedMatrix(void)
{
    return &blendedMatrices;
}

void AnimStateMachine::AddState(uint64_t stateName, AnimationClip* clip)
{
    AnimationState* state = new AnimationState(stateName, clip);
    animStates.insert(stateName, state);
}

void AnimStateMachine::SetEndCallback(uint64_t stateName, void (ISkinnedMeshModelChar::* callback)())
{
    AnimationState** animState = animStates.search(stateName);
    if (animState) 
    {
        (*animState)->SetEndCallback(callback);
    }
}

void AnimStateMachine::SetCurrentState(uint64_t newStateName)
{
    AnimationState** newState = animStates.search(newStateName);
    if (newState)
    {
        if (currentState)
        {
            if (currentState->currentStateName == newStateName)
                return;
            (*newState)->currentClip->currentTime = 0;
            if (blendOn)
                (*newState)->StartBlend(currentState->currentClip);
        }
        currentState = *newState;
    }
}

uint64_t AnimStateMachine::GetCurrentState(void)
{
    if (currentState)
        return currentState->currentStateName;

    return UINT64_MAX;
}

void AnimStateMachine::Update(float deltaTime, ISkinnedMeshModelChar* character)
{
    if (currentState)
    {
        // 条件をチェックして次の状態に移行
        NextStateInfo nextStateInfo = currentState->GetNextState(character);
        if (nextStateInfo.nextState != currentState->currentStateName)
        {
            blendOn = nextStateInfo.animBlend;
            SetCurrentState(nextStateInfo.nextState);

        }

        currentState->Update(deltaTime);
    }
}

void AnimStateMachine::AddTransition(uint64_t from, uint64_t to, bool (ISkinnedMeshModelChar::* condition)() const, bool waitForAnimationEnd, bool animBlend)
{
    AnimationState** animState = animStates.search(from);
    if (animState)
    {
        (*animState)->AddTransition(from, to, condition, waitForAnimationEnd, animBlend);
    }
}

SimpleArray<XMFLOAT4X4>* AnimStateMachine::GetBoneMatrices()
{
    if (currentState)
    {
        return currentState->GetBlendedMatrix();
    }
    return nullptr;
}

AnimationClip* AnimStateMachine::GetCurrentAnimClip()
{
    if (currentState)
    {
        return currentState->currentClip;
    }

    return nullptr;
}
