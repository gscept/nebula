//------------------------------------------------------------------------------
//  filewatcher.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/filewatcher.h"

namespace IO
{
__ImplementClass(IO::FileWatcher, 'FIWT', Threading::Thread);
__ImplementInterfaceSingleton(IO::FileWatcher);

using namespace IO;
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
FileWatcher::FileWatcher() :
    interval(0.2)
{
    __ConstructInterfaceSingleton;    
}

//------------------------------------------------------------------------------
/**
*/
FileWatcher::~FileWatcher()
{
    if (this->IsRunning())
        this->Stop();
    n_assert(this->watchers.IsEmpty());    
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::SetSpeed(double speed)
{    
    this->interval = speed;
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::Setup()
{
    n_assert(!this->IsRunning());
    this->SetPriority(Threading::Thread::Low);
    this->SetThreadAffinity(System::Cpu::Core4);
    this->SetName("FileWatcher thread");    
    this->Start();
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::Update()
{
    for (auto& w : this->watchers)
    {        
        FileWatcherImpl::Update(w.Value());        
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::Watch(Util::StringAtom const& folder, bool recursive, WatchFlags flags, WatchDelegate const& callback)
{
    EventHandlerData data = { callback, folder,flags };
    data.data.recursive = recursive;
    this->watcherQueue.Enqueue(data);               
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::Unwatch(Util::StringAtom const& folder)
{    
    EventHandlerData data;
    data.folder = folder;
    this->watcherQueue.Enqueue(data);
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcher::CheckQueue()
{
    static Util::Array<EventHandlerData> newEvents(10,10);
    this->watcherQueue.DequeueAll(newEvents);
    for (auto & e : newEvents)
    {
        if (e.callback)
        {
            n_assert(!this->watchers.Contains(e.folder));
            this->watchers.Add(e.folder, e);
            EventHandlerData& eventData = this->watchers[e.folder];
            FileWatcherImpl::CreateWatcher(eventData);
        }
        else
        {
            n_assert(this->watchers.Contains(e.folder));
            auto & eventData = this->watchers[e.folder];
            FileWatcherImpl::DestroyWatcher(eventData);
            this->watchers.Erase(e.folder);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
FileWatcher::DoWork()
{
    this->ioServer = IO::IoServer::Create();
    while (!this->ThreadStopRequested())
    {        
        this->CheckQueue();
        this->Update();
        Core::SysFunc::Sleep(this->interval);
    }
    // clear queue before shutting down
    this->CheckQueue();
}

} // namespace IO
