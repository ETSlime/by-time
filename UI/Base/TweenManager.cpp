//=============================================================================
//
// [TweenManager.cpp]
// Author : 
// 
//=============================================================================
#include "UI/Base/TweenManager.h"

void TweenTask::Update(float deltaTime, UIElement* target)
{
    if (finished || !target) return;

    elapsed += deltaTime;
    float t = min(elapsed / duration, 1.0f);
    t = ApplyEase(t, ease);

    switch (type) 
    {
    case TweenTargetType::Position:
        target->SetPosition(Lerp(startVec2, endVec2, t));
        break;
    case TweenTargetType::Scale:
        target->SetScale(Lerp(startVec2, endVec2, t));
        break;
    case TweenTargetType::Rotation:
        target->SetRotation(LerpFloat(startFloat, endFloat, t));
        break;
    }

    if (elapsed >= duration) 
    {
        finished = true;
        if (onComplete) onComplete(target);
    }
}

float TweenTask::ApplyEase(float t, EaseType ease)
{
    switch (ease) 
    {
    case EaseType::EaseIn: return t * t;
    case EaseType::EaseOut: return t * (2 - t);
    case EaseType::EaseInOut: return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
    default: return t;
    }
}

XMFLOAT2 TweenTask::Lerp(const XMFLOAT2& a, const XMFLOAT2& b, float t)
{
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

float TweenTask::LerpFloat(float a, float b, float t)
{
    return a + (b - a) * t;
}

void TweenManager::Update(void)
{
    for (auto& task : m_tasks)
    {
        if (task->target && !task->tween.finished)
            task->tween.Update(m_timer.GetDeltaTime(), task->target);


        if (task->tween.finished)
        {
            if (task->tween.onComplete)
                task->tween.onComplete(task->target); // 完了したタスクのコールバックを呼び出す
            m_toRemove.push_back(task); // 完了したタスクを削除予定リストに追加
        }
    }

    // 完了したタスクを削除
    for (auto& task : m_toRemove)
    {
        int index = m_tasks.find_index(task);
        if (index >= 0)
        {
			SAFE_DELETE(task); // メモリ解放
            m_tasks.erase(index);
        }
    }
}

void TweenManager::AddMove(UIElement* target, XMFLOAT2 to, float duration, EaseType ease, void(*callback)(UIElement*))
{
    TweenTask t;
    t.type = TweenTargetType::Position;
    t.duration = duration;
    t.ease = ease;
    t.startVec2 = target->GetPosition();
    t.endVec2 = to;
    t.onComplete = callback;

    m_tasks.push_back(new TweenEntry(target, t));
}

void TweenManager::AddScale(UIElement* target, XMFLOAT2 to, float duration, EaseType ease, void(*callback)(UIElement*))
{
    TweenTask t;
    t.type = TweenTargetType::Scale;
    t.duration = duration;
    t.ease = ease;
    t.startVec2 = target->GetScale();
    t.endVec2 = to;
    t.onComplete = callback;

    m_tasks.push_back(new TweenEntry(target, t));
}

void TweenManager::AddRotate(UIElement* target, float to, float duration, EaseType ease, void(*callback)(UIElement*))
{
    TweenTask t;
    t.type = TweenTargetType::Rotation;
    t.duration = duration;
    t.ease = ease;
    t.startFloat = target->GetRotation();
    t.endFloat = to;
    t.onComplete = callback;

    m_tasks.push_back(new TweenEntry(target, t));
}