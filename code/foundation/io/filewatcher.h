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
#include <functional>
#include "util/bitfield.h"
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
    NameChanged = 0,
    SizeChanged = 1,
    Write = 2,
    Access = 3,
    Creation = 4,
};

//------------------------------------------------------------------------------
struct WatchEvent
{
    WatchEventType type;
    Util::StringAtom folder;
    Util::String relativePath;
    Util::String file;
};
using WatchDelegate = std::function<void(WatchEvent const&)>;

//------------------------------------------------------------------------------
struct EventHandlerData
{
    WatchDelegate callback;
    Util::StringAtom folder;
    Util::BitField<8> flags;
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
    void Watch(Util::StringAtom const& folder, bool recursive, Util::BitField<8> flags, WatchDelegate const& callback);
    /// unregister
    void Unwatch(Util::StringAtom const& folder);
    /// check if folder is watched
    bool IsWatched(Util::StringAtom const& folder) const;
        
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
