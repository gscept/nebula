//------------------------------------------------------------------------------
//  gameserver.cc
//  (C) 2003 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "game/gameserver.h"
#include "core/factory.h"
#include "profiling/profiling.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "jobs2/jobs2.h"

namespace Game
{
__ImplementClass(Game::GameServer, 'GMSV', Core::RefCounted);
__ImplementSingleton(Game::GameServer);

//------------------------------------------------------------------------------
/**
*/
GameServer::GameServer()
    : isOpen(false),
      isStarted(false)
{
    __ConstructSingleton;
    _setup_grouped_timer(GameServerOnBeginFrame, "Game Subsystem");
    _setup_grouped_timer(GameServerOnFrame, "Game Subsystem");
    _setup_grouped_timer(GameServerOnEndFrame, "Game Subsystem");
    _setup_grouped_timer(GameServerManageEntities, "Game Subsystem");
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

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            delete this->state.worlds[worldIndex];
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

    this->CreateWorld(WORLD_DEFAULT);

    for (IndexT i = 0; i < this->gameFeatures.Size(); i++)
    {
        this->gameFeatures[i]->OnActivate();
    }

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            this->state.worlds[worldIndex]->Start();
        }
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
    feature->SetCmdLineArgs(this->GetCmdLineArgs());
    feature->OnAttach();
    if (this->isOpen)
    {
        feature->OnActivate();
    }
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
    if (this->isOpen)
    {
        feature->OnDeactivate();
    }
    feature->OnRemove();
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

    // prefilter all worlds
    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            this->state.worlds[worldIndex]->PrefilterProcessors();
        }
    }

    // call the OnStart method on all gameFeatures
    int i;
    int num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
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
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
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

    N_MARKER_BEGIN(FeaturesBeginFrame, Game)
    // trigger game features to at the beginning of a frame
    IndexT i;
    SizeT num = this->gameFeatures.Size();
    for (i = 0; i < num; i++)
    {
        N_MARKER_DYN_BEGIN(this->gameFeatures[i]->GetClassName().AsCharPtr(), Game);
        this->gameFeatures[i]->OnBeginFrame();
        N_MARKER_END();
    }
    N_MARKER_END()

    N_MARKER_BEGIN(FeaturesBeforeViews, Game)
    // trigger game features to at the beginning of a frame
    for (i = 0; i < num; i++)
    {
        N_MARKER_DYN_BEGIN(this->gameFeatures[i]->GetClassName().AsCharPtr(), Game);
        this->gameFeatures[i]->OnBeforeViews();
        N_MARKER_END();
    }
    N_MARKER_END()

    N_MARKER_BEGIN(WorldPrefilterProcessors, Game)
    // check if caches are valid
    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            this->state.worlds[worldIndex]->PrefilterProcessors();
        }
    }
    N_MARKER_END()

    N_MARKER_BEGIN(WorldBeginFrame, Game)
    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* w = this->state.worlds[worldIndex];
            w->BeginFrame();
        }
    }
    N_MARKER_END()

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
    N_MARKER_BEGIN(GameServerOnFrame, Game)
    for (i = 0; i < num; i++)
    {
        N_MARKER_DYN_BEGIN(this->gameFeatures[i]->GetClassName().AsCharPtr(), Game);
        this->gameFeatures[i]->OnFrame();
        N_MARKER_END();
    }

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* w = this->state.worlds[worldIndex];
            w->SimFrame();
        }
    }

    Game::ReleaseDatasets();
    N_MARKER_END();
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
    N_MARKER_BEGIN(GameServerOnEndFrame, Game);
    for (i = 0; i < numFeatureUnits; i++)
    {
        N_MARKER_DYN_BEGIN(this->gameFeatures[i]->GetClassName().AsCharPtr(), Game);
        this->gameFeatures[i]->OnEndFrame();
        N_MARKER_END();
    }

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* w = this->state.worlds[worldIndex];
            w->EndFrame();
        }
    }

    Game::ReleaseDatasets();
    N_MARKER_END();
    _start_timer(GameServerManageEntities);

    n_assert(GameServer::HasInstance());
    N_MARKER_BEGIN(ManageEntities, Game);
    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* world = this->state.worlds[worldIndex];
            world->ManageEntities();
        }
    }
    N_MARKER_END();
    _stop_timer(GameServerManageEntities);

    N_MARKER_BEGIN(DecayFeatures, Game);
    for (i = 0; i < numFeatureUnits; i++)
    {
        N_MARKER_DYN_BEGIN(this->gameFeatures[i]->GetClassName().AsCharPtr(), Game);
        this->gameFeatures[i]->OnDecay();
        N_MARKER_END();
    }
    N_MARKER_END();

    N_MARKER_BEGIN(DecayWorlds, Game);
    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* world = this->state.worlds[worldIndex];
            world->ClearDecayBuffers();
        }
    }

    N_MARKER_END();
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
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
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
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
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
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnLoad(this->state.worlds[worldIndex]);
            }
        }
    }

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* w = this->state.worlds[worldIndex];
            w->OnLoad();
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
        for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
        {
            if (this->state.worlds[worldIndex] != nullptr)
            {
                this->gameFeatures[i]->OnSave(this->state.worlds[worldIndex]);
            }
        }
    }

    for (uint32_t worldIndex = 0; worldIndex < MaxWorlds; worldIndex++)
    {
        if (this->state.worlds[worldIndex] != nullptr)
        {
            World* w = this->state.worlds[worldIndex];
            w->OnSave();
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
    world->Reset();
}

//------------------------------------------------------------------------------
/**
*/
World*
GameServer::CreateWorld(WorldHash hash)
{
    n_assert(this->state.numWorlds < MaxWorlds);

    // A world id is the slot the world lives in, so a slot that has been emptied can be taken again.
    // Without this, opening and closing a world would cost a slot for the rest of the session
    IndexT slot = InvalidIndex;
    for (IndexT i = 0; i < (IndexT) MaxWorlds; i++)
    {
        if (this->state.worlds[i] == nullptr)
        {
            slot = i;
            break;
        }
    }
    n_assert(slot != InvalidIndex);

    this->state.worldTable.Add(hash, (WorldId) slot);

    World*& world = this->state.worlds[slot];
    world = new World(hash, (WorldId) slot);
    this->state.numWorlds++;
    return world;
}

//------------------------------------------------------------------------------
/**
*/
World*
GameServer::GetWorld(WorldHash worldHash)
{
    return this->state.worlds[this->state.worldTable[worldHash]];
}

//------------------------------------------------------------------------------
/**
*/
World*
GameServer::GetWorld(WorldId worldId)
{
    return this->state.worlds[worldId];
}

//------------------------------------------------------------------------------
/**
    The slot the world was in becomes free, which is what CreateWorld picks up on the next call.
*/
void
GameServer::DestroyWorld(WorldHash worldHash)
{
    WorldId index = this->state.worldTable[worldHash];
    delete this->state.worlds[index];
    this->state.worlds[index] = nullptr;
    this->state.worldTable.Erase(worldHash);
    this->state.numWorlds--;
}

} // namespace Game
