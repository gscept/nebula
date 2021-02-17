//------------------------------------------------------------------------------
//  gameserver.cc
//  (C) 2003 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/gameserver.h"
#include "core/factory.h"
#include "profiling/profiling.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "basegamefeature/basegamefeatureunit.h"

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
    _setup_grouped_timer(GameServerManageEntities, "Game Subsystem");
    this->state.templateDatabase = MemDb::Database::Create();
    this->state.ownerId = MemDb::TypeRegistry::GetPropertyId("Owner"_atm);
    state.world.db = MemDb::Database::Create();
    // always attach the base game feature
    this->AttachGameFeature(BaseGameFeature::BaseGameFeatureUnit::Create());
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
    _discard_timer(GameServerManageEntities);

    this->state.templateDatabase = nullptr;

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

    num = this->state.world.onBeginFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->state.world.onBeginFrameCallbacks[i].cache, this->state.world.onBeginFrameCallbacks[i].filter);
        this->state.world.onBeginFrameCallbacks[i].func(data);
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

    num = this->state.world.onFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->state.world.onBeginFrameCallbacks[i].cache, this->state.world.onFrameCallbacks[i].filter);
        this->state.world.onFrameCallbacks[i].func(data);
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

    num = this->state.world.onEndFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->state.world.onBeginFrameCallbacks[i].cache, this->state.world.onEndFrameCallbacks[i].filter);
        this->state.world.onEndFrameCallbacks[i].func(data);
    }

    Game::ReleaseDatasets();

    _start_timer(GameServerManageEntities);

    n_assert(GameServer::HasInstance());
    GameServer::State* const state = &GameServer::Singleton->state;

    World& world = state->world;
    // NOTE: The order of the following loops are important!

    // Clean up any managed property instances.
    for (auto c = world.categoryDecayMap.Begin(); c != world.categoryDecayMap.End(); c++)
    {
        MemDb::TableId tid = *c.val;
        world.db->Clean(tid);
    }

    // Clean up entities
    while (!world.deallocQueue.IsEmpty())
    {
        auto const cmd = world.deallocQueue.Dequeue();
        world.DeallocateInstance(cmd.category, cmd.instance);
    }

    // Allocate instances for new entities, reuse invalid instances if possible
    while (!world.allocQueue.IsEmpty())
    {
        auto const cmd = world.allocQueue.Dequeue();
        n_assert(IsValid(cmd.entity));

        if (cmd.tid.templateId != Ids::InvalidId16)
        {
            world.AllocateInstance(cmd.entity, cmd.tid);
        }
        else
        {
            world.AllocateInstance(cmd.entity, (BlueprintId)cmd.tid.blueprintId);
        }
    }

    // Delete all remaining invalid instances
    Ptr<MemDb::Database> const& db = world.db;

    if (db.isvalid())
    {
        db->ForEachTable([](MemDb::TableId tid)
        {
            GameServer::Singleton->state.world.DefragmentCategoryInstances(tid);
        });
    }

    _stop_timer(GameServerManageEntities);

    Game::ReleaseAllOps();

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

    num = this->state.world.onLoadCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->state.world.onBeginFrameCallbacks[i].cache, this->state.world.onLoadCallbacks[i].filter);
        this->state.world.onLoadCallbacks[i].func(data);
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

    num = this->state.world.onSaveCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->state.world.onBeginFrameCallbacks[i].cache, this->state.world.onSaveCallbacks[i].filter);
        this->state.world.onSaveCallbacks[i].func(data);
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
    ProcessorInfo processor = info;
    
    ProcessorHandle handle;
    if (!this->processorHandlePool.Allocate(handle))
        this->processors.Append(std::move(processor));
    else
        this->processors[Ids::Index(handle)] = std::move(processor);

    // TODO: we should handle the registering of the world processors manually
    this->state.world.RegisterProcessor({ handle });

    return handle;
}

//------------------------------------------------------------------------------
/**
    Setup a new, empty world.
*/
void
GameServer::SetupEmptyWorld()
{
    Game::GameServer::Instance()->NotifyBeforeLoad();
}

//------------------------------------------------------------------------------
/**
    Cleanup the game world. This should undo the stuff in SetupWorld().
    Override this method in a subclass if your app needs different
    behaviour.
*/
void
GameServer::CleanupWorld()
{
    Game::GameServer::Instance()->NotifyBeforeCleanup();
    this->state.world.db->Reset();
}

} // namespace Game
