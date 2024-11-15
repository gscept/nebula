#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::FeatureUnit

    A FeatureUnit is an encapsulated feature which can be
    added to an application.
    E.g. game features can be core features like Render or Network,
    or it can be some of the addons like db or physics.

    To add a new feature, derive from this class and add it to
    the Game::GameServer on application or statehandler startup.

    The Game::GameServer will start, load, save, trigger and close your feature.

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "game/manager.h"
#include "core/singleton.h"
#include "util/commandlineargs.h"
#include "io/jsonwriter.h"
#include "ids/idgenerationpool.h"
#include "util/arrayallocator.h"
#include "game/component.h"

//------------------------------------------------------------------------------
namespace Game
{

class FeatureUnit : public Core::RefCounted
{
    __DeclareClass(FeatureUnit)
public:
    /// constructor
    FeatureUnit();
    /// destructor
    virtual ~FeatureUnit();

    /// Register a component type
    template <typename COMPONENT_TYPE>
    ComponentId RegisterComponentType(ComponentRegisterInfo<COMPONENT_TYPE> info = {});

    /// called from GameServer::AttachGameFeature()
    virtual void OnAttach();
    /// called from GameServer::RemoveGameFeature()
    virtual void OnRemove();

    /// called after GameServer::AttachGameFeature()
    virtual void OnActivate();
    /// called before GameServer::RemoveGameFeature()
    virtual void OnDeactivate();

    /// return true if featureunit is currently active
    bool IsActive() const;

    /// called from within GameServer::Load()
    virtual void OnLoad(World* world);
    /// called from within GameServer::OnStart() after OnLoad
    virtual void OnStart(World* world);
    /// called from within GameServer::Save()
    virtual void OnSave(World* world);

    /// called from within GameServer::NotifyBeforeLoad()
    virtual void OnBeforeLoad(World* world);
    /// called from within GameServer::NotifyBeforeCleanup()
    virtual void OnBeforeCleanup(World* world);
    /// called from withing GameServer::Stop()
    virtual void OnStop(World* world);


    /// called on begin of frame
    virtual void OnBeginFrame();
    /// Called between beginning of frame and before the views are iterated
    virtual void OnBeforeViews();
    /// called in the middle of the feature trigger cycle
    virtual void OnFrame();
    /// called at the end of the feature trigger cycle
    virtual void OnEndFrame();
    /// called after entities has been destroyed and before releasing their memory
    virtual void OnDecay();

    /// called when game debug visualization is on
    virtual void OnRenderDebug();

    /// attach a manager to the feature unit
    virtual void AttachManager(Ptr<Manager> manager);
    /// remove a manager from the feature unit
    virtual void RemoveManager(Ptr<Manager> manager);

    /// set command line args
    void SetCmdLineArgs(const Util::CommandLineArgs& a);
    /// get command line args
    const Util::CommandLineArgs& GetCmdLineArgs() const;

protected:
    Util::Array<Ptr<Manager>> managers; 
    bool active;

    /// cmdline args for configuration from cmdline
    Util::CommandLineArgs args;
};

//------------------------------------------------------------------------------
/**
    @todo We should register the component ids to the feature unit and 
    deregister them when the feature is detached or destroyed.
*/
template <typename COMPONENT_TYPE>
ComponentId
FeatureUnit::RegisterComponentType(ComponentRegisterInfo<COMPONENT_TYPE> info)
{
    uint32_t componentFlags = 0;
    componentFlags |= (uint32_t)COMPONENTFLAG_DECAY * (uint32_t)info.decay;

    ComponentInterface* cInterface = new ComponentInterface(COMPONENT_TYPE::Traits::name, COMPONENT_TYPE(), componentFlags);
    cInterface->Init = reinterpret_cast<ComponentInterface::ComponentInitFunc>(info.OnInit);
    Game::ComponentId const cid = MemDb::AttributeRegistry::Register<COMPONENT_TYPE>(cInterface);
    Game::ComponentSerialization::Register<COMPONENT_TYPE>(cid);
    Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<COMPONENT_TYPE>);

    return cid;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
FeatureUnit::IsActive() const
{
    return this->active;
}

//------------------------------------------------------------------------------
/**
*/
inline void
FeatureUnit::SetCmdLineArgs(const Util::CommandLineArgs& a)
{
    this->args = a;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::CommandLineArgs&
FeatureUnit::GetCmdLineArgs() const
{
    return this->args;
}

} // namespace FeatureUnit
//------------------------------------------------------------------------------
