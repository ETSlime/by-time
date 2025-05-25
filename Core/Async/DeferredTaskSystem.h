#pragma once
//=============================================================================
//
// [DeferredTaskSystem.h]
// Author : 
//
//
//=============================================================================
#include "main.h"
#include "Utility/SimpleArray.h"
#include "Utility/SingletonBase.h"
#include "Core/Timer.h"

// ==========================
// ジェネリック延期タスク系統（任意のクラスで使用可）
// ==========================
typedef void (*DeferredFunc)(void*);
typedef bool (*DeferredCanRunFunc)(void*);

struct DeferredTask
{
	DeferredFunc executeFunc;         // 実行する関数
	DeferredCanRunFunc canRunFunc;    // 実行可否判定
	void* param;                      // 関数に渡すパラメータ
	float delayTime;                  // 遅延時間（秒）
	float elapsedTime;                // 経過時間（秒）
	bool isRunning;					  // タスクが実行中かどうか
	bool runAsync;                    // 非同期実行
};

// ==========================
// ThreadPool 再利用
// ==========================
class ThreadPool 
{
public:
	static void Init() {}
	static void Shutdown() {}

	static void RunAsync(void (*func)(void*), void* param) 
	{
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)func, param, 0, nullptr);
	}
};

class DeferredTaskSystem : public SingletonBase<DeferredTaskSystem>
{
public:
	// タスクを追加
	void AddTask(DeferredFunc execFunc, void* param, DeferredCanRunFunc canRunFunc = nullptr, 
		bool async = false, float delay = 0.0f);

	// タスクを更新（経過時間を進めて実行可能なタスクを実行）
	void Update(void);

private:
	SimpleArray<DeferredTask> m_tasks;
	Timer& m_timer = Timer::get_instance(); // タイマーインスタンス
};