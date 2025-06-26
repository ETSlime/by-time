#pragma once
//=============================================================================
//
// [TweenManager.h]
// Author : 
// 
//=============================================================================
#include "UI/Base/UIElement.h"

enum class TweenTargetType 
{
    Position,
    Scale,
    Rotation,
};

enum class EaseType 
{
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
};


class TweenTask 
{
public:
	TweenTask(void) = default;
    void Update(float deltaTime, UIElement* target);

    TweenTargetType type;
    float duration;
    float elapsed = 0.0f;
    EaseType ease;

	XMFLOAT2 startVec2, endVec2; // 開始と終了の位置・スケール
	float startFloat = 0.0f, endFloat = 0.0f; // 開始と終了の回転

	// コールバック関数
    void (*onComplete)(UIElement*) = nullptr;

    bool finished = false;

private:
    float ApplyEase(float t, EaseType ease);

    XMFLOAT2 Lerp(const XMFLOAT2& a, const XMFLOAT2& b, float t);

    float LerpFloat(float a, float b, float t);
};

struct TweenEntry
{
	TweenEntry(UIElement* target, const TweenTask& tweenTask)
		: target(target), tween(tweenTask) {}

    UIElement* target;
    TweenTask tween;
};

class TweenManager 
{
public:
    void Update(void);

	// タスクを追加する関数群
	void AddMove(UIElement* target, XMFLOAT2 to, float duration, EaseType ease = EaseType::EaseInOut, void(*callback)(UIElement*) = nullptr); // 位置を移動
	void AddScale(UIElement* target, XMFLOAT2 to, float duration, EaseType ease = EaseType::EaseInOut, void(*callback)(UIElement*) = nullptr); // スケールを変更
	void AddRotate(UIElement* target, float to, float duration, EaseType ease = EaseType::EaseInOut, void(*callback)(UIElement*) = nullptr); // 回転を変更

private:

	SimpleArray<TweenEntry*> m_tasks; // 現在のタスク一覧
	SimpleArray<TweenEntry*> m_toRemove; // 削除予定のタスク

	Timer& m_timer = Timer::get_instance();
};
