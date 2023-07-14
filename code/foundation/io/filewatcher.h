#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::FileWatcher

    For registering callbacks for file modification events.
    
    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "core/refcounted.h"
#include "core/singleton.h"
#include "core/ptr.h"
#include "util/stringatom.h"
#include "util/dictionary.h"
#include "io/uri.h"
#include "io/ioserver.h"
#include "threading/thread.h"
#include "threading/safequeue.h"
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
    NameChange,
    Modified,
};

enum WatchFlags
{
    NameChanged = N_BIT(0),
    SizeChanged = N_BIT(1),
    Write = N_BIT(2),
    Access = N_BIT(3),
    Creation = N_BIT(4),
};

//------------------------------------------------------------------------------
struct WatchEvent
{
    WatchEventType type;
    Util::StringAtom folder;
    Util::String file;
};
using WatchDelegate = std::function<void(WatchEvent const&)>;

//------------------------------------------------------------------------------
struct EventHandlerData
{
    WatchDelegate callback;
    Util::StringAtom folder;
    WatchFlags flags;
    FileWatcherPlatform data;
};

//------------------------------------------------------------------------------
class FileWatcher : public Threading::Thread
{
    __DeclareClass(FileWatcher);
    __DeclareInterfaceSingleton(FileWatcher);

public:
    /// constructor
    FileWatcher();
    /// destructor
    virtual ~FileWatcher();

    /// starts watcher thread
    void Setup();

    void SetSpeed(double speed);
    
    /// Register Folder
    void Watch(Util::StringAtom const& folder, bool recursive, WatchFlags flags, WatchDelegate const& callback);
    /// unregister
    void Unwatch(Util::StringAtom const& folder);
        
private:
    /// checks for file modifications and calls registered callbacks
    void Update();
    ///
    void CheckQueue();
    ///
    void DoWork();
    
    Ptr<IO::IoServer> ioServer;
    Util::Dictionary<Util::StringAtom, EventHandlerData> watchers;    
    Threading::SafeQueue< EventHandlerData> watcherQueue;
    double interval;

};

} // namespace IO
