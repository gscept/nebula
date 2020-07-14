#pragma once
//------------------------------------------------------------------------------
/**
    @class Linux::FileWatcher

    Linux implementation of filewatcher

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"

namespace IO
{
    struct EventHandlerData;
    struct FileWatcherPlatform
    {        
        bool recursive;        
    };

class FileWatcherImpl 
{
public:
    static void CreateWatcher(EventHandlerData& data);
    static void DestroyWatcher(EventHandlerData& data);
    static void Update(EventHandlerData& data);
};
}