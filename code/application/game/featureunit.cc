//------------------------------------------------------------------------------
//  featureunit.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "game/featureunit.h"
#include "game/gameserver.h"
#include "gameserver.h"

namespace Game
{
__ImplementClass(FeatureUnit, 'GAFE', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
FeatureUnit::FeatureUnit() :
    active(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
FeatureUnit::~FeatureUnit()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnAttach()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnRemove()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method is called by Game::GameServer::AttachGameFeature().
    Use this method for one-time initializations of the FeatureUnit.
*/
void
FeatureUnit::OnActivate()
{
    n_assert(!this->IsActive());
    this->active = true;
}

//------------------------------------------------------------------------------
/**
    This method is called by Game::GameServer::RemoveGameFeature(). Use this
    method to cleanup stuff which has been initialized in OnActivate().
*/
void
FeatureUnit::OnDeactivate()
{
    n_assert(this->IsActive());

    // remove all managers
    for (SizeT i = 0; i < this->managers.Size(); i++)
    {
        this->managers[i]->OnDeactivate();
    }

    this->managers.Free();
    this->active = false;
}

//------------------------------------------------------------------------------
/**
    This method is called from within Game::GameServer::Load().
*/
void
FeatureUnit::OnLoad(World* world)
{
    // now call the OnLoad() method on all managers
    int managerIndex;
    int numManagers = this->managers.Size();
    for (managerIndex = 0; managerIndex < numManagers; managerIndex++)
    {
        // invoke OnLoad() on manager
        this->managers[managerIndex]->OnLoad(world);
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from within Game::GameServer::OnStart().
    Its called after all game features are activated and have initialized their subsystems.
*/
void
FeatureUnit::OnStart(World* world)
{
    // call the OnStart method on all managers
    int i;
    int num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnStart(world);
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from within Game::GameServer::NotifyGameSave().
*/
void
FeatureUnit::OnSave(World* world)
{
    int managerIndex;
    int numManagers = this->managers.Size();
    for (managerIndex = 0; managerIndex < numManagers; managerIndex++)
    {
        this->managers[managerIndex]->OnSave(world);
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from Game::GameServer::OnBeginFrame() on all
    game features attached to an GameServer in the order of attachment. Override this
    method if your FeatureUnit has to do any work at the beginning of the frame.
*/
void
FeatureUnit::OnBeginFrame()
{
    // call OnBeginFrame() on managers
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnBeginFrame();
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from Game::GameServer::OnBeforeViews() on all
    game features attached to an GameServer in the order of attachment. Override this
    method if your FeatureUnit has to do any work at the beginning of the frame.
*/
void
FeatureUnit::OnBeforeViews()
{
    // call OnBeginFrame() on managers
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnBeforeViews();
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from Game::GameServer::OnMoveBefore() on all
    game features attached to an GameServer in the order of attachment. Override this
    method if your FeatureUnit has any work to do before the physics subsystem
    is triggered.
*/
void
FeatureUnit::OnFrame()
{
    // call OnFrame() on managers   
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnFrame();
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from Game::GameServer::OnRender() on all
    game features attached to an GameServer in the order of attachment. Override
    this method if your FeatureUnit has any work to do before rendering happens.
*/
void
FeatureUnit::OnEndFrame()
{
    // call OnEndFrame() on managers
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnEndFrame();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnDecay()
{
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnDecay();
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from Game::GameServer::OnRenderDebug() on all
    game features attached to an GameServer in the order of attachment. It's meant for debug
    issues. It will be called when debug mode is enabled.
*/
void
FeatureUnit::OnRenderDebug()
{
    // call OnEndFrame() on managers
    IndexT i;
    SizeT num = this->managers.Size();
    for (i = 0; i < num; i++)
    {
        this->managers[i]->OnRenderDebug();
    }
}

//------------------------------------------------------------------------------
/**
    Attach a manager to the game world. The manager's OnActivate()
    method will be called once right away, and then its OnFrame() method
    once per frame.
*/
void
FeatureUnit::AttachManager(Ptr<Manager> manager)
{
    manager->OnActivate();
    this->managers.Append(manager);
}

//------------------------------------------------------------------------------
/**
    Remove a manager from the game world. The manager's OnDeactivate()
    method will be called.
*/
void
FeatureUnit::RemoveManager(Ptr<Manager> manager)
{
    n_assert(manager.isvalid());
    for (IndexT i = 0; i < this->managers.Size(); ++i)
    {
        if (this->managers[i] == manager)
        {
            this->managers[i]->OnDeactivate();
            this->managers.EraseIndex(i);
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnBeforeLoad(World* world)
{
    // override in subclass if needed
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnBeforeCleanup(World* world)
{
    // TODO: We should call destroy on all entities, thus automatically allowing
    //       managers to clean up components by just doing their decay routine.

    for (IndexT i = 0; i < this->managers.Size(); ++i)
    {
        this->managers[i]->OnCleanup(world);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
FeatureUnit::OnStop(World* world)
{
    
}

}; // namespace Game
