#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::GameServer

    The game server setups and runs the game world.
    Functionality and queries on the game world are divided amongst
    several FeaturesUnits.
    This keeps the game server's interface small and clean, and lets applications
    easily extend functionality by implementing new, or deriving from existing
    game features.

    To add or replace FeatureUnit objects, derive from Game::FeatureUnit and 
    add your features on application start or gamestatehandler enter.

    The GameServer triggers all attached features. Start and Stop is called within 
    the gamestatehandler to allow all features do stuff after everything is loaded 
    and initialized. Load and Save is invoked from the BaseGameFeature which allows
    beginning a new game, load or save a game.
    
    @copyright
    (C) 2007 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "core/singleton.h"
#include "game/featureunit.h"
#include "debug/debugtimer.h"
#include "ids/idgenerationpool.h"
#include "basegamefeature/properties/owner.h"
#include "api.h"
#include "entitypool.h"

//------------------------------------------------------------------------------
namespace Game
{

//------------------------------------------------------------------------------
/**
*/
class GameServer : public Core::RefCounted
{
    __DeclareClass(GameServer)
    __DeclareSingleton(GameServer)
public:
    /// constructor
    GameServer();
    /// destructor
    virtual ~GameServer();

    // initialize all features
    virtual bool Open();
     // start the game; starts running all processors
    virtual bool Start();
    // stop the game. Stops running the processors
    virtual void Stop(); 
    // shuts down all features
    virtual void Close(); 
    
    /// has the game world already started
    bool HasStarted() const;

    /// trigger actions before rendering the game world
    virtual void OnBeginFrame();
    /// trigger the game world
    virtual void OnFrame();
    /// trigger actions after rendering the game world
    virtual void OnEndFrame();

    /// call OnBeforeLoad on all game features
    virtual void NotifyBeforeLoad();
    /// call OnBeforeCleanup on all game features
    virtual void NotifyBeforeCleanup();
   
    /// call OnLoad on all game features
    virtual void NotifyGameLoad();
    /// call OnSave on all game features
    virtual void NotifyGameSave();
        
    /// add game feature
    void AttachGameFeature(const Ptr<FeatureUnit>& feature);
    /// remove game feature
    void RemoveGameFeature(const Ptr<FeatureUnit>& feature);
    /// is feature attached
    bool IsFeatureAttached(const Util::String& stringName) const;
    /// access to all attached features units
    Util::Array<Ptr<FeatureUnit>> const& GetGameFeatures() const;

    /// create a processor
    ProcessorHandle CreateProcessor(ProcessorCreateInfo const& info);

    /// set command line args
    void SetCmdLineArgs(const Util::CommandLineArgs& a);
    /// get command line args
    const Util::CommandLineArgs& GetCmdLineArgs() const;

    /// setup an empty game world
    virtual void SetupEmptyWorld();
    /// cleanup the game world
    virtual void CleanupWorld();

    /// contains internal state and world management
    struct State
    {
        /// world entity management
        World world;
        /// Contains all templates
        Ptr<MemDb::Database> templateDatabase;
        /// quick access to the Owner property id
        PropertyId ownerId;
    } state;

protected:
    bool isOpen;
    bool isStarted;
    Util::CommandLineArgs args;
    Util::Array<Ptr<FeatureUnit> > gameFeatures;
    friend class World;
    
    struct CallbackInfo
    {
        ProcessorHandle handle;
        Filter filter;
        ProcessorFrameCallback func;
        /// cached tables that we've filtered out.
        Util::Array<MemDb::TableId> cache;
    };

    struct ProcessorInfo
    {
        Util::StringAtom name;
        /// set if this processor should run as a job
        bool async = false;
        /// called when removed from game server
        void(*OnDeactivate)() = nullptr;
    };

    /// add the table to any callback-caches that accepts it
    void AddTableToCaches(MemDb::TableId tid, MemDb::TableSignature signature);

    Util::Array<CallbackInfo> onBeginFrameCallbacks;
    Util::Array<CallbackInfo> onFrameCallbacks;
    Util::Array<CallbackInfo> onEndFrameCallbacks;
    Util::Array<CallbackInfo> onLoadCallbacks;
    Util::Array<CallbackInfo> onSaveCallbacks;
    Util::Array<ProcessorInfo> processors;
    Ids::IdGenerationPool processorHandlePool;

#if NEBULA_ENABLE_PROFILING
    _declare_timer(GameServerOnBeginFrame);
    _declare_timer(GameServerOnFrame);
    _declare_timer(GameServerOnEndFrame);
    Util::Array<Ptr<Debug::DebugTimer>> onBeginFrameTimers;
    Util::Array<Ptr<Debug::DebugTimer>> onFrameTimers;
    Util::Array<Ptr<Debug::DebugTimer>> onEndFrameTimers;
#endif
};

//------------------------------------------------------------------------------
/**
*/
inline void
GameServer::SetCmdLineArgs(const Util::CommandLineArgs& a)
{
    this->args = a;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::CommandLineArgs&
GameServer::GetCmdLineArgs() const
{
    return this->args;
}

/// call this if you need to defragment the category instance table
void DefragmentCategoryInstances(Category const& cat);

}; // namespace Game
//------------------------------------------------------------------------------


