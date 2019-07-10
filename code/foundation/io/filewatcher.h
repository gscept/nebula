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
    Util::String folder;
    Util::String file;
};
using WatchDelegate = std::function<void(WatchEvent const&)>;

//------------------------------------------------------------------------------
struct EventHandlerData
{
    WatchDelegate callback;
    Util::String folder;
	WatchFlags flags;
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
    void Watch(Util::String const& folder, bool recursive, WatchFlags flags, WatchDelegate const& callback);
    /// unregister
    void Unwatch(Util::String const& folder);

private:
   
	WatchFlags watcherFlags;
    Util::Dictionary<Util::String, EventHandlerData> watchers;    
};

//------------------------------------------------------------------------------
class FileWatcherThread : public Threading::Thread
{
	__DeclareClass(FileWatcherThread);
public:
	/// constructor
	FileWatcherThread();
	/// destructor
	virtual ~FileWatcherThread();

	/// set file watcher object
	void SetWatcher(const Ptr<FileWatcher>& watcher);
private:
	/// this method runs in the thread context
	void DoWork() override;

	Ptr<FileWatcher> watcher;
};

//------------------------------------------------------------------------------
/**
*/
inline void 
FileWatcherThread::SetWatcher(const Ptr<FileWatcher>& watcher)
{
	this->watcher = watcher;
}

} // namespace IO