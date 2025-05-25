#pragma once
// DeferredTaskBinder.h
#include "Core/Async/DeferredTaskSystem.h"

// ==========================
// クラスのメンバー関数を繋ぐバインダ
// ==========================
template<typename T>
class DeferredTaskBinder 
{
public:
    typedef void (T::* DeferredMethodExec)();
    typedef bool (T::* DeferredMethodCond)() const;

    static void DispatchExec(void* ptr) 
    {
        TaskContext* ctx = static_cast<TaskContext*>(ptr);
        T* obj = static_cast<T*>(ctx->object);
        (obj->*ctx->execFunc)();
        delete ctx;
    }

    static bool DispatchCond(void* ptr) {
        TaskContext* ctx = static_cast<TaskContext*>(ptr);
        T* obj = static_cast<T*>(ctx->object);
        return ctx->condFunc ? (obj->*ctx->condFunc)() : true;
    }

    static void Bind(T* object, DeferredMethodExec exec, DeferredMethodCond cond = nullptr, bool async = false, float delay = 0.0f) {
        TaskContext* ctx = new TaskContext();
        ctx->object = object;
        ctx->execFunc = exec;
        ctx->condFunc = cond;

        GenericDeferredTaskSystem::AddTask(
            DispatchExec,
            ctx,
            cond ? DispatchCond : nullptr,
            async,
            delay
        );
    }

private:
    struct TaskContext {
        T* object;
        DeferredMethodExec execFunc;
        DeferredMethodCond condFunc;
    };
};

// ==========================
// 非メンバー関数を繋ぐラッパー型
// ==========================
class RawDeferredTask {
public:
    typedef void (*RawExecFunc)(void*);
    typedef bool (*RawCondFunc)(void*);

    static void DispatchExec(void* param) {
        RawDeferredTask* task = (RawDeferredTask*)param;
        task->execFunc(task->context);
        delete task;
    }

    static bool DispatchCond(void* param) {
        RawDeferredTask* task = (RawDeferredTask*)param;
        return task->condFunc ? task->condFunc(task->context) : true;
    }

    static void Add(void* context, RawExecFunc exec, RawCondFunc cond = nullptr, bool async = false, float delay = 0.0f) {
        RawDeferredTask* task = new RawDeferredTask{ context, exec, cond };
        DeferredTaskSystem::get_instance().AddTask(DispatchExec, task, cond ? DispatchCond : nullptr, async, delay);
    }

private:
    void* context;
    RawExecFunc execFunc;
    RawCondFunc condFunc;
};
