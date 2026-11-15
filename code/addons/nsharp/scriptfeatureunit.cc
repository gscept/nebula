//------------------------------------------------------------------------------
//  @file nsharp/scriptfeatureunit.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "scriptfeatureunit.h"
#include "nsharpserver.h"
#include "io/filewatcher.h"
#include "game/gameserver.h"
#include "coreclr_delegates.h"

namespace Scripting
{
__ImplementClass(Scripting::ScriptFeatureUnit, 'SCFU', Game::FeatureUnit);
__ImplementSingleton(ScriptFeatureUnit);

// Managed entry points that take a blittable argument. The pointers are resolved once at activation
// and cached here, which keeps the per-frame flush from doing a dictionary lookup.
typedef int(CORECLR_DELEGATE_CALLTYPE* FlushEntityCreateQueueFn)(void*, int);
typedef int(CORECLR_DELEGATE_CALLTYPE* CollectGarbageFn)(uint32_t);
typedef int(CORECLR_DELEGATE_CALLTYPE* ClearWorldFn)(uint32_t);

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::ScriptFeatureUnit() :
    engineAssemblyPath("bin:NebulaEngine.dll"),
    mainEntry("NebulaEngine.AppEntry::Main()"),
    exitEntry("NebulaEngine.AppEntry::Exit()"),
    beginFrameEntry("NebulaEngine.AppEntry::OnBeginFrame()"),
    frameEntry("NebulaEngine.AppEntry::OnFrame()"),
    endFrameEntry("NebulaEngine.AppEntry::OnEndFrame()"),
    reloadEntry("NebulaEngine.AppEntry::RequestReload()"),
    flushEntityCreateQueueEntry("NebulaEngine.AppEntry::FlushEntityCreateQueue()"),
    collectGarbageEntry("NebulaEngine.AppEntry::CollectGarbage()"),
    clearWorldEntry("NebulaEngine.AppEntry::ClearWorld()"),
    flushEntityCreates(nullptr),
    collectGarbage(nullptr),
    clearWorld(nullptr)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ScriptFeatureUnit::~ScriptFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::SetScriptAssembly(IO::URI const& path)
{
    this->assemblyPath = path;
}

//------------------------------------------------------------------------------
/**
*/
Scripting::AssemblyId
ScriptFeatureUnit::GetAssemblyId() const
{
    return this->engineAssemblyId;
}

//------------------------------------------------------------------------------
/**
    Native is the source of truth for entities, so a mirrored world is kept in sync by a callback
    rather than by walking the database. The walk here only seeds the queue with entities that
    already existed when the callback was registered, which is the case for entities authored by
    C++ before the script assembly is loaded, and for worlds that exist before a reload.
*/
void
ScriptFeatureUnit::RegisterWorld(Game::World* world)
{
    if (world == nullptr)
        return;

    Game::EntityCreatedCallback callback =
        Game::EntityCreatedCallback::FromMethod<ScriptFeatureUnit, &ScriptFeatureUnit::OnEntityCreated>(this);

    uint32_t worldId = world->GetWorldId();
    if (!this->worldEntityCounts.Contains(worldId))
        this->worldEntityCounts.Add(worldId, world->GetNumEntities());

    world->EnumerateEntities(callback);
    world->SetEntityCreatedCallback(callback);
}

//------------------------------------------------------------------------------
/**
    Called by every registered world when an entity instance is allocated in it. The entity id is
    blittable and carries its world id, so the queue can be flushed to the managed runtime with a
    single call.
*/
void
ScriptFeatureUnit::OnEntityCreated(Game::World* world, Game::Entity entity)
{
    this->entityCreateQueue.Append(uint64_t(entity));
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();
    this->server = NSharpServer::Create();
    this->server->Open();

    if (this->assemblyPath.IsValid())
    {
        this->engineAssemblyId = this->server->LoadAssembly(this->engineAssemblyPath);
        n_assert(this->engineAssemblyId != Scripting::AssemblyId::Invalid());

        Util::String pluginPath = this->assemblyPath.GetHostAndLocalPath();
        this->reloadWatchPath = pluginPath.ExtractDirName();
        Util::String reloadFileName = pluginPath.ExtractFileName();

        if (IO::FileWatcher::HasInstance())
        {
            auto reloadFileFunc = [this, reloadFileName](IO::WatchEvent const& event)
            {
                if ((event.type == IO::Modified || event.type == IO::NameChange || event.type == IO::Created) &&
                    event.file == reloadFileName)
                    this->reloadQueue.Enqueue(this->assemblyPath.GetHostAndLocalPath());
            };

            // WatchFlags values are bit positions, so they have to be set as bits, not or'd
            // together as a mask.
            Util::BitField<8> watchFlags;
            watchFlags.SetBit(IO::WatchFlags::NameChanged);
            watchFlags.SetBit(IO::WatchFlags::SizeChanged);
            watchFlags.SetBit(IO::WatchFlags::Write);
            watchFlags.SetBit(IO::WatchFlags::Creation);

            IO::FileWatcher::Instance()->Watch(
                this->reloadWatchPath,
                false,
                watchFlags,
                reloadFileFunc
            );
        }

        this->flushEntityCreates = this->server->GetUnmanagedFuncPointer(this->engineAssemblyId, this->flushEntityCreateQueueEntry);
        this->collectGarbage = this->server->GetUnmanagedFuncPointer(this->engineAssemblyId, this->collectGarbageEntry);
        this->clearWorld = this->server->GetUnmanagedFuncPointer(this->engineAssemblyId, this->clearWorldEntry);

        // Worlds that already exist when the feature unit is attached are not reported by
        // OnWorldCreated, so mirror them here.
        for (uint32_t worldIndex = 0; worldIndex < Game::GameServer::MaxWorlds; worldIndex++)
        {
            this->RegisterWorld(Game::GameServer::Instance()->GetWorld(worldIndex));
        }

        int result = this->server->ExecUnmanagedCall(this->engineAssemblyId, this->mainEntry, pluginPath);

        n_assert(result == 0);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnDeactivate()
{
    if (IO::FileWatcher::HasInstance() && !this->reloadWatchPath.IsEmpty())
    {
        IO::FileWatcher::Instance()->Unwatch(this->reloadWatchPath);
    }

    for (IndexT i = 0; i < this->worldEntityCounts.Size(); i++)
    {
        Game::World* world = Game::GetWorld(this->worldEntityCounts.KeyAtIndex(i));
        if (world != nullptr)
            world->SetEntityCreatedCallback(Game::EntityCreatedCallback());
    }
    this->worldEntityCounts.Clear();
    this->entityCreateQueue.Clear();
    this->flushEntityCreates = nullptr;
    this->collectGarbage = nullptr;
    this->clearWorld = nullptr;

    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
    {
        this->server->Close();
        FeatureUnit::OnDeactivate();
        return;
    }

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->exitEntry);
    this->server->Close();
    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnStart(Game::World* world)
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->RegisterWorld(world);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnWorldCreated(Game::World* world)
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->RegisterWorld(world);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnWorldDestroyed(Game::World* world)
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid() || world == nullptr)
    return;

    // A world id is a slot that the next world can take, so the mirror has to be dropped when the world
    // goes. Otherwise the entities of the destroyed world would still resolve against the recycled slot,
    // and a new world starting from index 0 generation 0 would produce the same ids.

    uint32_t worldId = world->GetWorldId();
    this->worldEntityCounts.Erase(worldId);

    if (this->clearWorld != nullptr)
        ((ClearWorldFn)this->clearWorld)(worldId);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnBeginFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->reloadQueue.DequeueAll(this->reloads);

    if (!this->reloads.IsEmpty())
    {
        // Only reload the latest, clear the rest.
        this->server->ExecUnmanagedCall(
            this->engineAssemblyId,
            this->reloadEntry,
            this->reloads[this->reloads.Size() - 1]
        );
        this->reloads.Clear();
    }

    // Flush the entities created since the last frame and let the managed side consume them
    if (!this->entityCreateQueue.IsEmpty() && this->flushEntityCreates != nullptr)
    {
        // Deletion is deferred, so an entity that was created and destroyed in the same frame is no
        // longer valid by the time this runs. Filtering it out is what keeps the mirror from getting
        // a stale entry: the entity count would be the same as at the last sweep, so OnDecay would
        // not have swept it again.
        SizeT numValid = 0;
        for (IndexT i = 0; i < this->entityCreateQueue.Size(); i++)
        {
            Game::Entity entity = Game::Entity::FromId(this->entityCreateQueue[i]);
            Game::World* world = Game::GetWorld((Game::WorldId)entity.world);
            if (world != nullptr && world->IsValid(entity))
                this->entityCreateQueue[numValid++] = this->entityCreateQueue[i];
        }

        if (numValid > 0)
            ((FlushEntityCreateQueueFn)this->flushEntityCreates)(this->entityCreateQueue.Begin(), (int)numValid);

        this->entityCreateQueue.Clear();
    }

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->beginFrameEntry);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->frameEntry);
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnEndFrame()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid())
        return;

    this->server->ExecUnmanagedCall(this->engineAssemblyId, this->endFrameEntry);
}

//------------------------------------------------------------------------------
/**
    Entity deletion is deferred, and the game server runs ManageEntities() before OnDecay(), so this
    is the point where the managed side can drop entities that no longer exist natively. Worlds
    whose entity count did not change are ignored
*/
void
ScriptFeatureUnit::OnDecay()
{
    if (this->engineAssemblyId == Scripting::AssemblyId::Invalid() || this->collectGarbage == nullptr)
        return;

    for (IndexT i = 0; i < this->worldEntityCounts.Size(); i++)
    {
        uint32_t worldId = (uint32_t)this->worldEntityCounts.KeyAtIndex(i);
        Game::World* world = Game::GetWorld(worldId);
        if (world == nullptr)
            continue;

        SizeT numEntities = world->GetNumEntities();
        if (numEntities == this->worldEntityCounts.ValueAtIndex(i))
            continue;

        this->worldEntityCounts.ValueAtIndex(i) = numEntities;
        ((CollectGarbageFn)this->collectGarbage)(worldId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ScriptFeatureUnit::OnRenderDebug()
{
    // TODO: implement me!
}

} // namespace Scripting
