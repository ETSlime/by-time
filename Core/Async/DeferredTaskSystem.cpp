//=============================================================================
//
// [DeferredTaskSystem.cpp]
// Author : 
//
//
//=============================================================================
#include "DeferredTaskSystem.h"

void DeferredTaskSystem::Init(void)
{
    m_tasks.clear(); // タスクリストを初期化
#ifdef _DEBUG
    DebugProc::get_instance().Register(this); // デバッグUIに登録
#endif // DEBUG
}

void DeferredTaskSystem::ShutDown(bool wait)
{
    if (wait)
		WaitAll(); // すべての非同期タスクが完了するまで待機
    else 
		CancelAll(); // すべてのタスクをキャンセル
	m_tasks.clear(); // タスクリストをクリア
	m_shutdown = true; // シャットダウンフラグを設定
}

void DeferredTaskSystem::AddTask(DeferredFunc execFunc, void* param, DeferredCanRunFunc canRunFunc, const DeferredTaskOptions& options)
{
    if (m_shutdown) return;

	// タスクの初期化
	DeferredTask task;
	task.executeFunc = execFunc; // タスクを実行する関数
	task.canRunFunc = canRunFunc; // タスクが実行可能かどうかを判定する関数（nullptr の場合は常に実行可能）
	task.param = param; // タスクに渡すパラメータ
	task.runAsync = options.async; // 非同期実行フラグ
	task.delayTime = options.delay; // 遅延時間を設定
	task.elapsedTime = 0.0f; // 遅延時間経過時間を初期化
	task.timeoutSec = options.timeout; // タイムアウト秒数を設定（-1 は無制限）
	task.timeoutElapsed = 0.0f; // タイムアウト経過時間を初期化
	task.runningTime = 0.0f; // 実行開始時間を初期化
	task.isRunning = false; // タスクはまだ実行されていない
	task.isFinished = false; // タスクはまだ完了していない
	task.debugName = options.debugName ? const_cast<char*>(options.debugName) : nullptr; // デバッグ用の名前を設定
	m_tasks.push_back(task);
}

void DeferredTaskSystem::Update(void)
{
	// タスクの経過時間を進める
    for (UINT i = 0; i < m_tasks.getSize(); ++i) 
    {
        DeferredTask& task = m_tasks[i];

        if (task.isRunning && task.isFinished)
        {
			m_tasks.erase(i); // タスクが完了している場合はリストから削除
            if (i > 0) --i;
            continue;
        }

        if (task.isRunning)
        {
			if (task.runAsync)
				task.runningTime += m_timer.GetDeltaTime(); // 非同期実行の場合は経過時間を進める
            continue; // タスクが実行中の場合はスキップ
        }

        task.elapsedTime += m_timer.GetDeltaTime(); // タスクの経過時間を進める
        task.timeoutElapsed += m_timer.GetDeltaTime(); // タイムアウト経過時間を進める

        // タイムアウト処理
        if (task.timeoutSec > 0.0f && task.timeoutElapsed > task.timeoutSec) 
        {
            OutputDebugStringA("[DeferredTaskSystem] タスクがタイムアウトしました\n");
            m_tasks.erase(i); // タイムアウトしたタスクは削除する
            if (i > 0) --i;
            continue;
        }

		if (task.elapsedTime < task.delayTime) continue; // 遅延時間が設定されている場合はまだ実行しない

        bool canRun = true;
        if (task.canRunFunc) 
            canRun = task.canRunFunc(task.param); // 実行可能かどうかを判定する関数がある場合はそれを呼び出す

		// タスクを実行する
        if (canRun) 
        {
            task.isRunning = true;

            if (task.runAsync) 
            {
				// タスクを非同期で実行するためのコンテキストを作成
                struct TaskContext 
                {
                    DeferredFunc func;
                    void* param;
                    bool* finished;
                };
                TaskContext* ctx = new TaskContext{ task.executeFunc, task.param, &task.isFinished };

				// 非同期実行の場合はスレッドプールを使用してタスクを実行
                ThreadPool::RunAsync([](void* param) 
                    {
                    TaskContext* ctx = static_cast<TaskContext*>(param);
					ctx->func(ctx->param); // タスクを実行
					*ctx->finished = true; // タスクの完了フラグを設定
                    SAFE_DELETE(ctx);
                    }, ctx);
            }
            else 
            {
                task.executeFunc(task.param);
                task.isFinished = true;
            }
        }
    }
}

void DeferredTaskSystem::RenderImGui(void)
{
    ImGui::Text("Active Tasks: %d", static_cast<int>(m_tasks.getSize()));
    ImGui::Separator();

    for (const auto& task : m_tasks) 
    {
        ImGui::Text("Name: %s", task.debugName);
        ImGui::Text("  Delay: %.2f / %.2f", task.elapsedTime, task.delayTime);
        ImGui::Text("  Timeout: %.2f / %.2f", task.timeoutElapsed, task.timeoutSec);
        ImGui::Text("  Running Time: %.2f", task.runningTime);
        ImGui::Text("  Status: %s",
            task.isRunning ? (task.isFinished ? "Finished" : "Running") : "Pending");
        ImGui::Separator();
    }
}

void DeferredTaskSystem::WaitAll(void) 
{
    bool stillWaiting = true;
    while (stillWaiting) 
    {
        stillWaiting = false;
        for (const auto& task : m_tasks) 
        {
            if (task.runAsync && task.isRunning && !task.isFinished) 
            {
                stillWaiting = true;
                break;
            }
        }
        if (stillWaiting) Sleep(1); // 少し待つ
    }
}

void DeferredTaskSystem::CancelAll() 
{
    for (auto& task : m_tasks) 
    {
        if (task.isRunning && task.runAsync && !task.isFinished)
        {
            // 実際には ThreadPool 側でキャンセルをサポートする必要がある
            OutputDebugStringA("[DeferredTaskSystem] 非同期タスクがキャンセルされました（強制終了の可能性あり）\n");
        }
    }
    m_tasks.clear();
}

