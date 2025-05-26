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
	float timeoutSec;				  // タイムアウト秒数（-1 は無制限）
	float timeoutElapsed;			  // タイムアウト経過時間（秒）
	float runningTime;				  // 実行開始時間（秒）
	bool isRunning;					  // タスクが実行中かどうか
	bool isFinished;				  // タスクが完了したかどうか
	bool runAsync;                    // 非同期実行
	char* debugName;			      // デバッグ用の名前
}; // タスクの情報を保持する構造体


struct DeferredTaskOptions 
{
	bool async = false;
	float delay = 0.0f;
	float timeout = -1.0f;
	const char* debugName = nullptr;
}; // タスクのオプション設定

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

class DeferredTaskSystem : public SingletonBase<DeferredTaskSystem>, public IDebugUI
{
public:

	void Init(void);
	void ShutDown(bool wait = true);

	// タスクを追加
	void AddTask(DeferredFunc execFunc, void* param, DeferredCanRunFunc canRunFunc = nullptr, const DeferredTaskOptions& options = DeferredTaskOptions{});

	// タスクを更新（経過時間を進めて実行可能なタスクを実行）
	void Update(void);

	virtual void RenderImGui(void);
	virtual const char* GetPanelName(void) const { return "Deferred Task Monitor"; };

private:

	void WaitAll(void); // すべての非同期タスクが完了するまで待機
	void CancelAll(void); // すべてのタスクをキャンセル（実行中のタスクは終了するまで待つ）

	bool m_shutdown = false;
	SimpleArray<DeferredTask> m_tasks;
	Timer& m_timer = Timer::get_instance(); // タイマーインスタンス
};