#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::FileWatcher

    Win32 implementation of filewatcher

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"
#include "util/pinnedarray.h"
#include "util/fixedarray.h"

namespace IO
{
struct EventHandlerData;
struct FileWatcherPlatform
{
    HANDLE dirHandle;
    OVERLAPPED overlapped;

    Util::Array<HANDLE> handles;
    Util::Array<OVERLAPPED> overlaps;
    Util::PinnedArray<0xFFF, Util::FixedArray<BYTE>> buffers;
    Util::Array<Util::String> relativePaths;
    Util::Array<Util::String> fullPaths;
    DWORD notifyFilter;
    BYTE buffer[16 * 1024];
    Util::Dictionary<HANDLE, Util::String> relativePathLookup;
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
