#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::FileWatcher

    Win32 implementation of filewatcher

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"

namespace IO
{
struct EventHandlerData;
struct FileWatcherPlatform
{
    HANDLE dirHandle;
    OVERLAPPED overlapped;
    DWORD notifyFilter;
    BYTE buffer[16 * 1024];
    bool recursive;        
};

class FileWatcherImpl 
{
public:
    static void Init();
    static void Shutdown();
    static void CreateWatcher(EventHandlerData& data);
    static void DestroyWatcher(EventHandlerData& data);
    static void Update(EventHandlerData& data);
    static void WaitForEvents(double timeoutSecs);
    static void WakeUp();
};
}
