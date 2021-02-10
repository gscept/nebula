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

    num = this->onBeginFrameCallbacks.Size();
    for (i = 0; i < num; i++)
    {
#if NEBULA_ENABLE_PROFILING
        this->onBeginFrameTimers[i]->Start();
#endif
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].cache, this->onBeginFrameCallbacks[i].filter);
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
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].cache, this->onFrameCallbacks[i].filter);
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
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].cache, this->onEndFrameCallbacks[i].filter);
        this->onEndFrameCallbacks[i].func(data);
#if NEBULA_ENABLE_PROFILING
        this->onEndFrameTimers[i]->Stop();
#endif
    }

    Game::ReleaseDatasets();

    n_assert(GameServer::HasInstance());
    GameServer::State* const state = &GameServer::Singleton->state;

    World& world = state->world;
    // NOTE: The order of the following loops are important!

    // Clean up any managed property instances.
    for (IndexT c = 0; c < world.categoryArray.Size(); c++)
    {
        MemDb::TableId tid = world.categoryArray[c].managedPropertyTable;
        if (tid != MemDb::TableId::Invalid())
            world.db->Clean(tid);
    }

    // Clean up entities
    // @todo    We can improve performance by working with one world at a time here, having one cmd queue per world.
    while (!world.deallocQueue.IsEmpty())
    {
        auto const cmd = world.deallocQueue.Dequeue();
        EntityMapping mapping = world.entityMap[cmd.entity.index];
        Category const& category = world.GetCategory(mapping.category);
        world.DeallocateInstance(cmd.entity);
    }

    // Allocate instances for new entities, reuse invalid instances if possible
    // @todo    We can improve performance by working with one world at a time here, having one cmd queue per world.
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
        for (IndexT c = 0; c < world.categoryArray.Size(); c++)
        {
            Category& cat = world.categoryArray[c];
            DefragmentCategoryInstances(cat);
        }
    }

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

    num = this->onLoadCallbacks.Size();
    for (i = 0; i < num; i++)
    {
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].cache, this->onLoadCallbacks[i].filter);
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
        Dataset data = Game::Query(this->onBeginFrameCallbacks[i].cache, this->onSaveCallbacks[i].filter);
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

//------------------------------------------------------------------------------
/**
*/
void
GameServer::AddTableToCaches(MemDb::TableId tid, MemDb::TableSignature signature)
{
    // this is just to compress the code a bit
    const Util::Array<CallbackInfo>* cbArrays[] = {
        &this->onBeginFrameCallbacks,
        &this->onFrameCallbacks,
        &this->onEndFrameCallbacks,
        &this->onLoadCallbacks,
        &this->onSaveCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            if (MemDb::TableSignature::CheckBits(signature, GetInclusiveTableMask(cbinfo.filter)) &&
                !MemDb::TableSignature::HasAny(signature, GetExclusiveTableMask(cbinfo.filter)))
                cbinfo.cache.Append(tid);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DefragmentCategoryInstances(Category const& cat)
{
    World& world = GameServer::Singleton->state.world;
    Ptr<MemDb::Database> db = world.db;
    MemDb::Table& table = db->GetTable(cat.instanceTable);
    MemDb::ColumnIndex ownerColumnId = db->GetColumnId(cat.instanceTable, GameServer::Singleton->state.ownerId);

    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    auto* const map = &(world.entityMap);
    SizeT numErased = db->Defragment(cat.instanceTable, [map, cat, &ownerColumnId, &table](InstanceId from, InstanceId to)
    {
        Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from.id];
        Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to.id];
        if (!IsValid(fromEntity))
        {
            // we need to add this instances new index to the to the freeids list, since it's been deleted.
            // the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
            // and it will automatically be defragged
            table.freeIds.Append(to.id);
        }
        else
        {
            (*map)[fromEntity.index].instance = to;
            (*map)[toEntity.index].instance = from;
        }
    });
}

} // namespace Game
