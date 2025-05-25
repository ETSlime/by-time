#pragma once
//=============================================================================
//
// [AsyncResourceLoader.h]
// Author : 
//
//
//=============================================================================
#include "main.h"
#include "Utility/SingletonBase.h"

class AsyncResourceLoader : public SingletonBase<AsyncResourceLoader>
{
public:
    AsyncResourceLoader();
    ~AsyncResourceLoader();

    void SetFileList(const char** files, int count);
    void Start();
    void Update() {};
    bool IsDone() const { return m_done; }
    int GetProgress() const { return m_progress; }
    void* GetLoadedData(int index) const { return m_loadedData[index]; }
    int GetFileCount() const { return m_fileCount; }

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    void LoadResources();

    HANDLE m_thread;
    volatile bool m_done;
    volatile int m_progress;

    const char** m_fileList;
    int m_fileCount;
    void** m_loadedData;
};