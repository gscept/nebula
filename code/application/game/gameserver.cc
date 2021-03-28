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
    WorldCreateInfo info;
    info.hash = WORLD_DEFAULT;
    this->CreateWorld(info);
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

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            n_delete(this->state.worlds[worldIndex]);
            this->state.worlds[worldIndex] = nullptr;
        }
    }

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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnStart(this->state.worlds[worldIndex]);
            }
        }
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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnStop(this->state.worlds[worldIndex]);
            }
        }
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

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            num = this->state.worlds[worldIndex]->onBeginFrameCallbacks.Size();
            for (i = 0; i < num; i++)
            {
                World* w = this->state.worlds[worldIndex];
                Dataset data = Game::Query(w, w->onBeginFrameCallbacks[i].cache, w->onBeginFrameCallbacks[i].filter);
                w->onBeginFrameCallbacks[i].func(w, data);
            }
        }
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

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            num = this->state.worlds[worldIndex]->onFrameCallbacks.Size();
            for (i = 0; i < num; i++)
            {
                World* w = this->state.worlds[worldIndex];
                Dataset data = Game::Query(w, w->onFrameCallbacks[i].cache, w->onFrameCallbacks[i].filter);
                w->onFrameCallbacks[i].func(w, data);
            }
        }
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
    SizeT numFeatureUnits = this->gameFeatures.Size();
    for (i = 0; i < numFeatureUnits; i++)
    {
        this->gameFeatures[i]->OnEndFrame();
    }

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            SizeT num = this->state.worlds[worldIndex]->onEndFrameCallbacks.Size();
            for (i = 0; i < num; i++)
            {
                World* w = this->state.worlds[worldIndex];
                Dataset data = Game::Query(w, w->onEndFrameCallbacks[i].cache, w->onEndFrameCallbacks[i].filter);
                w->onEndFrameCallbacks[i].func(w, data);
            }
        }
    }

    Game::ReleaseDatasets();

    _start_timer(GameServerManageEntities);

    n_assert(GameServer::HasInstance());
    
    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* world = this->state.worlds[worldIndex];
            // NOTE: The order of the following loops are important!

            // Clean up entities
            while (!world->deallocQueue.IsEmpty())
            {
                auto const cmd = world->deallocQueue.Dequeue();
                if (Game::IsValid(world, cmd.entity))
                {
                    MemDb::TableId const table = world->entityMap[cmd.entity.index].category;
                    MemDb::Row const row = world->entityMap[cmd.entity.index].instance;
                    DeallocateInstance(world, table, row);
                    world->entityMap[cmd.entity.index].category = MemDb::InvalidTableId;
                    world->entityMap[cmd.entity.index].instance = MemDb::InvalidRow;
                    DeallocateEntity(world, cmd.entity);
                }
            }

            // Allocate instances for new entities, reuse invalid instances if possible
            while (!world->allocQueue.IsEmpty())
            {
                auto const cmd = world->allocQueue.Dequeue();
                n_assert(IsValid(world, cmd.entity));

                if (cmd.tid.templateId != Ids::InvalidId16)
                {
                    AllocateInstance(world, cmd.entity, cmd.tid);
                }
                else
                {
                    AllocateInstance(world, cmd.entity, (BlueprintId)cmd.tid.blueprintId);
                }
            }

            // Delete all remaining invalid instances
            Ptr<MemDb::Database> const& db = world->db;

            if (db.isvalid())
            {
                db->ForEachTable([world](MemDb::TableId tid)
                {
                    Defragment(world, tid);
                });
            }
        }
    }
    _stop_timer(GameServerManageEntities);

    Game::ReleaseAllOps();

    for (i = 0; i < numFeatureUnits; i++)
    {
        this->gameFeatures[i]->OnDecay();
    }

    Game::ClearDecayBuffers();

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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnBeforeLoad(this->state.worlds[worldIndex]);
            }
        }
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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnBeforeCleanup(this->state.worlds[worldIndex]);
            }
        }
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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnLoad(this->state.worlds[worldIndex]);
            }
        }
    }

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            num = this->state.worlds[worldIndex]->onLoadCallbacks.Size();
            for (i = 0; i < num; i++)
            {
                World* w = this->state.worlds[worldIndex];
                Dataset data = Game::Query(w, w->onLoadCallbacks[i].cache, w->onLoadCallbacks[i].filter);
                w->onLoadCallbacks[i].func(w, data);
            }
        }
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
        for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnSave(this->state.worlds[worldIndex]);
            }
        }
    }

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            num = this->state.worlds[worldIndex]->onSaveCallbacks.Size();
            for (i = 0; i < num; i++)
            {
                World* w = this->state.worlds[worldIndex];
                Dataset data = Game::Query(w, w->onSaveCallbacks[i].cache, w->onSaveCallbacks[i].filter);
                w->onSaveCallbacks[i].func(w, data);
            }
        }
    }

    Game::ReleaseDatasets();
}

//------------------------------------------------------------------------------
/**
*/
void
GameServer::RenderDebug()
{
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        this->gameFeatures[i]->OnRenderDebug();
    }
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
    @todo   we should handle the registering of the world processors manually
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

    for (uint32_t worldIndex = 0; worldIndex < this->state.numWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            RegisterProcessors(this->state.worlds[worldIndex], { handle });
        }
    }
    return handle;
}

//------------------------------------------------------------------------------
/**
    Setup a new, empty world.
*/
void
GameServer::SetupEmptyWorld(World* world)
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
GameServer::CleanupWorld(World* world)
{
    Game::GameServer::Instance()->NotifyBeforeCleanup();
    world->db->Reset();
}

//------------------------------------------------------------------------------
/**
*/
World*
GameServer::CreateWorld(WorldCreateInfo const& info)
{
    this->state.worldTable.Add(info.hash, this->state.numWorlds);
    n_assert(this->state.numWorlds < 32);
    World*& world = this->state.worlds[this->state.numWorlds++];
    world = n_new(World);
    world->hash = info.hash;
    return world;
}

//------------------------------------------------------------------------------
/**
*/
World*
GameServer::GetWorld(uint32_t worldHash)
{
    return this->state.worlds[this->state.worldTable[worldHash]];
}

//------------------------------------------------------------------------------
/**
    @note   The world index does not get recycled.
    @todo   The world index should be recycled.
*/
void
GameServer::DestroyWorld(uint32_t worldHash)
{
    uint32_t index = this->state.worldTable[worldHash];
    n_delete(this->state.worlds[index]);
    this->state.worlds[index] = nullptr;
    this->state.worldTable.Erase(worldHash);
}

} // namespace Game
