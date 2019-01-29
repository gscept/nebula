#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FileWatcher

    For registering callbacks for file modification events.
    
    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/string.h"
#include "util/dictionary.h"
#include "io/uri.h"
#include "util/delegate.h"
#if __WIN32__
#include "io/win32/win32filewatcher.h"
#elif __linux__
#include "io/posix/linuxfilewatcher.h"
#else
#error "not implemented"
#endif 

//------------------------------------------------------------------------------
namespace IO
{

//------------------------------------------------------------------------------
enum WatchEventType
{
    Created,
    Deleted,
    Modified,
};

//------------------------------------------------------------------------------
struct WatchEvent
{
    WatchEventType type;
    Util::String folder;
    Util::String file;
};
using WatchDelegate = Util::Delegate<void(WatchEvent const&)>;

//------------------------------------------------------------------------------
struct EventHandlerData
{
    WatchDelegate callback;
    Util::String folder;
    FileWatcherPlatform data;
};

//------------------------------------------------------------------------------
class FileWatcher : public Core::RefCounted
{
    __DeclareClass(FileWatcher);
    __DeclareSingleton(FileWatcher);

public:
    /// constructor
    FileWatcher();
    /// destructor
    virtual ~FileWatcher();
    /// checks for file modifications and calls registered callbacks
    void Update();
    /// Register Folder
    void Watch(Util::String const& folder, bool recursive, WatchDelegate const& callback);
    /// unregister
    void Unwatch(Util::String const& folder);

private:
   
    Util::Dictionary<Util::String, EventHandlerData> watchers;    
};
}