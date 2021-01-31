//------------------------------------------------------------------------------
//  gameserver.cc
//  (C) 2003 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/gameserver.h"
#include "core/factory.h"
#include "profiling/profiling.h"

namespace Game
{
__ImplementClass(Game::GameServer, 'GMSV', Core::RefCounted);
__ImplementSingleton(Game::GameServer);

//------------------------------------------------------------------------------
/**
*/
GameServer::GameServer() :
    isOpen(false),
    isStarted(false)
{
    __ConstructSingleton;
    _setup_grouped_timer(GameServerOnBeginFrame, "Game Subsystem");
    _setup_grouped_timer(GameServerOnFrame, "Game Subsystem");
    _setup_grouped_timer(GameServerOnEndFrame, "Game Subsystem");
}

//------------------------------------------------------------------------------
/**
*/
GameServer::~GameServer()
{
    n_assert(!this->isOpen);
    _discard_timer(GameServerOnBeginFrame);
    _discard_timer(GameServerOnFrame);
    _discard_timer(GameServerOnEndFrame);

    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Initialize the game server object.
*/
bool
GameServer::Open()
{
    n_assert(!this->isOpen);
    n_assert(!this->isStarted);
    this->isOpen = true;

    for (IndexT i = 0; i < this->gameFeatures.Size(); i++)
    {
        Ptr<Game::FeatureUnit> const& feature = this->gameFeatures[i];
        feature->SetCmdLineArgs(this->GetCmdLineArgs());
        feature->OnActivate();
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the game server object.
*/
void
GameServer::Close()
{
    n_assert(!this->isStarted);
    n_assert(this->isOpen);

    // remove all gameFeatures
    for (IndexT i = 0; i < this->gameFeatures.Size(); i++)
    {
        this->gameFeatures[i]->OnDeactivate();
    }
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::AttachGameFeature(const Ptr<FeatureUnit>& feature)
{
    n_assert(0 != feature);
    n_assert(InvalidIndex == this->gameFeatures.FindIndex(feature));
    this->gameFeatures.Append(feature);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::RemoveGameFeature(const Ptr<FeatureUnit>& feature)
{
    n_assert(0 != feature);
    IndexT index = this->gameFeatures.FindIndex(feature);
    n_assert(InvalidIndex != index);
    feature->OnDeactivate();
    this->gameFeatures.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
    Start the game world, called after loading has completed.
*/
bool
GameServer::Start()
{
    n_assert(this->isOpen);
    n_assert(!this->isStarted);

    // call the OnStart method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnStart();
    }

    this->isStarted = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
GameServer::HasStarted() const
{
    return this->isStarted;
}

//------------------------------------------------------------------------------
/**
    Stop the game world. 
*/
void
GameServer::Stop()
{
    n_assert(this->isOpen);
    n_assert(this->isStarted);

    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnStop();
    }

    this->isStarted = false;
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::OnBeginFrame()
{
    if (!this->isStarted)
        return;

    _start_timer(GameServerOnBeginFrame);

    // trigger game features to at the beginning of a frame
    IndexT i;
    SizeT num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnBeginFrame();
    }

    num = this->onBeginFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
#if NEBULA_ENABLE_PROFILING
        this->onBeginFrameTimers[i]->Start();
#endif
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].filter);
        this->onBeginFrameCallbacks[i].func(data);
#if NEBULA_ENABLE_PROFILING
        this->onBeginFrameTimers[i]->Stop();
#endif
    }

    Game::ReleaseDatasets();

    _stop_timer(GameServerOnBeginFrame);
}

//------------------------------------------------------------------------------
/**
    Trigger the game server. If your application introduces new or different
    manager objects, you may also want to override the Game::GameServer::Trigger()
    method if those gameFeatures need per-frame callbacks.
*/
void
GameServer::OnFrame()
{
    if (!this->isStarted)
        return;

    _start_timer(GameServerOnFrame);

    // call trigger functions on game features   
    IndexT i;
    SizeT num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnFrame();
    }

    num = this->onFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
#if NEBULA_ENABLE_PROFILING
        this->onFrameTimers[i]->Start();
#endif
        Dataset data = Game::Query(this->onFrameCallbacks[i].filter);
        this->onFrameCallbacks[i].func(data);
#if NEBULA_ENABLE_PROFILING
        this->onFrameTimers[i]->Stop();
#endif
    }

    Game::ReleaseDatasets();

    _stop_timer(GameServerOnFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::OnEndFrame()
{
    if (!this->isStarted)
        return;

    _start_timer(GameServerOnEndFrame);

    IndexT i;
    SizeT num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnEndFrame();
    }

    num = this->onEndFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
#if NEBULA_ENABLE_PROFILING
        this->onEndFrameTimers[i]->Start();
#endif
        Dataset data = Game::Query(this->onEndFrameCallbacks[i].filter);
        this->onEndFrameCallbacks[i].func(data);
#if NEBULA_ENABLE_PROFILING
        this->onEndFrameTimers[i]->Stop();
#endif
    }

    Game::ReleaseDatasets();

    _stop_timer(GameServerOnEndFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyBeforeLoad()
{
    // call the SetupDefault method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnBeforeLoad();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyBeforeCleanup()
{
    // call the SetupDefault method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnBeforeCleanup();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyGameLoad()
{
    // call the OnLoad method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnLoad();
    }

    num = this->onLoadCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->onLoadCallbacks[i].filter);
        this->onLoadCallbacks[i].func(data);
    }

    Game::ReleaseDatasets();
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::NotifyGameSave()
{
    // call the OnSave method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnSave();
    }

    num = this->onSaveCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->onSaveCallbacks[i].filter);
        this->onSaveCallbacks[i].func(data);
    }

    Game::ReleaseDatasets();
}

//------------------------------------------------------------------------------
/**
*/
bool
GameServer::IsFeatureAttached(const Util::String& stringName) const
{
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        if (this->gameFeatures[i]->GetRtti()->GetName() == stringName)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<FeatureUnit>> const&
GameServer::GetGameFeatures() const
{
    return this->gameFeatures;
}

//------------------------------------------------------------------------------
/**
*/
ProcessorHandle
GameServer::CreateProcessor(ProcessorCreateInfo const& info)
{
    ProcessorInfo processor;
    processor.async = info.async;
    processor.name = info.name;
    processor.OnDeactivate = info.OnDeactivate;

    ProcessorHandle handle;
    this->processorHandlePool.Allocate(handle);

    if (this->processors.Size() <= Ids::Index(handle))
        this->processors.Append(std::move(processor));
    else
        this->processors[Ids::Index(handle)] = std::move(processor);

    // Setup frame callbacks
    if (info.OnBeginFrame != nullptr)
    {
        this->onBeginFrameCallbacks.Append({ handle, info.filter, info.OnBeginFrame });
#if NEBULA_ENABLE_PROFILING
        Ptr<Debug::DebugTimer> timer = Debug::DebugTimer::Create();
        timer->Setup(info.name, "Processors - OnBeginFrame");
        this->onBeginFrameTimers.Append(timer);
#endif
    }

    if (info.OnFrame != nullptr)
    {
        this->onFrameCallbacks.Append({ handle, info.filter, info.OnFrame });
#if NEBULA_ENABLE_PROFILING
        Ptr<Debug::DebugTimer> timer = Debug::DebugTimer::Create();
        timer->Setup(info.name, "Processors - OnFrame");
        this->onFrameTimers.Append(timer);
#endif
    }

    if (info.OnEndFrame != nullptr)
    {
        this->onEndFrameCallbacks.Append({ handle, info.filter, info.OnEndFrame });
#if NEBULA_ENABLE_PROFILING
        Ptr<Debug::DebugTimer> timer = Debug::DebugTimer::Create();
        timer->Setup(info.name, "Processors - OnEndFrame");
        this->onEndFrameTimers.Append(timer);
#endif
    }

    if (info.OnLoad != nullptr)
        this->onLoadCallbacks.Append({ handle, info.filter, info.OnLoad });

    if (info.OnSave != nullptr)
        this->onSaveCallbacks.Append({ handle, info.filter, info.OnSave });
    
    if (info.OnActivate != nullptr)
    {
        info.OnActivate();
    }

    return handle;
}

} // namespace Game
