#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::ScriptFeatureUnit

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "game/world.h"
#include "assemblyid.h"
#include "threading/safequeue.h"
#include "util/array.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace Scripting
{

class NSharpServer;

class ScriptFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(ScriptFeatureUnit)
    __DeclareSingleton(ScriptFeatureUnit)

public:

    /// constructor
    ScriptFeatureUnit();
    /// destructor
    ~ScriptFeatureUnit();
    
    /// call this before attaching the feature unit
    void SetScriptAssembly(IO::URI const& path);
    Scripting::AssemblyId GetAssemblyId() const;

    void OnAttach() override;

    void OnActivate() override;
    void OnDeactivate() override;
    void OnStart(Game::World* world) override;
    void OnWorldCreated(Game::World* world) override;
    void OnWorldDestroyed(Game::World* world) override;
    void OnBeginFrame() override;
    void OnFrame() override;
    void OnEndFrame() override;
    void OnDecay() override;
    virtual void OnRenderDebug();

private:
    IO::URI engineAssemblyPath;
    IO::URI assemblyPath;
    Scripting::AssemblyId engineAssemblyId;
    Ptr<Scripting::NSharpServer> server;
    Threading::SafeQueue<Util::String> reloadQueue;
    Util::Array<Util::String> reloads;
    Util::String reloadWatchPath;

    /// Register a world with the managed mirror. Sets the entity callback, seeds the queue with the
    /// entities that already exist in the world, and remembers the entity count for the next decay sweep.
    void RegisterWorld(Game::World* world);

    /// Called by every registered world when an entity instance is allocated in it
    void OnEntityCreated(Game::World* world, Game::Entity entity);

    /// Worlds mirrored in the managed runtime, mapped to the entity count they had at the last decay sweep.
    Util::Dictionary<uint32_t, SizeT> worldEntityCounts;

    /// Entity ids created since the last flush
    Util::Array<uint64_t> entityCreateQueue;

    // Cached managed entry points, resolved once at activation so the frame flush does not look them
    // up through hostfxr every frame.
    void* flushEntityCreates;
    void* collectGarbage;
    void* clearWorld;

    // Cached entry point names. The server caches the resolved pointers, these are kept as members so the
    // frame callbacks do not build a Util::String from a literal every frame.
    Util::String mainEntry;
    Util::String exitEntry;
    Util::String beginFrameEntry;
    Util::String frameEntry;
    Util::String endFrameEntry;
    Util::String reloadEntry;
    Util::String flushEntityCreateQueueEntry;
    Util::String collectGarbageEntry;
    Util::String clearWorldEntry;
};

} // namespace Scripting
//------------------------------------------------------------------------------
