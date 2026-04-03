#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::FileWatcher

    Linux implementation of filewatcher

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"
#include "util/dictionary.h"
#include "util/string.h"

namespace IO
{
    struct EventHandlerData;
    struct FileWatcherPlatform
    {
        bool recursive;
        int inotifyFd = -1;
        Util::String rootPath;
        Util::Dictionary<int, Util::String> wdToRelativePath;
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
private:
    static int epollFd;
    static int wakeupFd;
};
}