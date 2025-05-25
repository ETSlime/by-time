#include "DeferredTaskSystem.h"

void DeferredTaskSystem::AddTask(DeferredFunc execFunc, void* param, DeferredCanRunFunc canRunFunc, bool async, float delay)
{
	DeferredTask task;
	task.executeFunc = execFunc;
	task.canRunFunc = canRunFunc;
	task.param = param;
	task.runAsync = async;
    task.delayTime = delay;
    task.elapsedTime = 0.0f;
    task.isRunning = false;
	m_tasks.push_back(task);
}

void DeferredTaskSystem::Update(void)
{
	// タスクの経過時間を進める
    for (int i = 0; i < m_tasks.getSize(); ++i) 
    {
        DeferredTask& task = m_tasks[i];

        if (task.isRunning) continue; // タスクが実行中の場合はスキップ

        task.elapsedTime += m_timer.GetDeltaTime(); // タスクの経過時間を進める
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
                struct TaskContext 
                {
                    DeferredFunc func;
                    void* param;
                };
                TaskContext* ctx = new TaskContext{ task.executeFunc, task.param };

                ThreadPool::RunAsync([](void* param) 
                    {
                    TaskContext* ctx = (TaskContext*)param;
                    ctx->func(ctx->param);
                    delete ctx;
                    }, ctx);
            }
            else 
            {
                task.executeFunc(task.param);
            }

            m_tasks.erase(i--);
        }
    }
}

