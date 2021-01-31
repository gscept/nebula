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

//------------------------------------------------------------------------------
namespace Game
{

class FeatureUnit : public Core::RefCounted
{
    __DeclareClass(FeatureUnit);
public:
    /// constructor
    FeatureUnit();
    /// destructor
    virtual ~FeatureUnit();

    /// called from GameServer::AttachGameFeature()
    virtual void OnActivate();
    /// called from GameServer::RemoveGameFeature()
    virtual void OnDeactivate();
    /// return true if featureunit is currently active
    bool IsActive() const;

    /// called from within GameServer::Load()
    virtual void OnLoad();
    /// called from within GameServer::OnStart() after OnLoad
    virtual void OnStart();
    /// called from within GameServer::Save()
    virtual void OnSave();

    /// called from within GameServer::NotifyBeforeLoad()
    virtual void OnBeforeLoad();
    /// called from within GameServer::NotifyBeforeCleanup()
    virtual void OnBeforeCleanup();
    /// called from withing GameServer::Stop()
    virtual void OnStop();


    /// called on begin of frame
    virtual void OnBeginFrame();
    /// called in the middle of the feature trigger cycle
    virtual void OnFrame();
    /// called at the end of the feature trigger cycle
    virtual void OnEndFrame();

    /// called when game debug visualization is on
    virtual void OnRenderDebug();

    /// attach a manager to the feature unit
    virtual ManagerHandle AttachManager(ManagerAPI api);
    /// remove a manager from the feature unit
    virtual void RemoveManager(ManagerHandle handle);

    /// set command line args
    void SetCmdLineArgs(const Util::CommandLineArgs& a);
    /// get command line args
    const Util::CommandLineArgs& GetCmdLineArgs() const;

protected:
    Util::ArrayAllocator<ManagerHandle, ManagerAPI> managers;
    Ids::IdGenerationPool managerPool;
    bool active;

    /// cmdline args for configuration from cmdline
    Util::CommandLineArgs args;
};

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
