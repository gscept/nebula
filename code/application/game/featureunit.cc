//------------------------------------------------------------------------------
//  featureunit.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
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
    while (this->managers.Size() > 0)
    {
        if (this->managers.Get<1>(0).OnDeactivate != nullptr)
            this->managers.Get<1>(0).OnDeactivate();
        this->managers.EraseIndex(0);
    }

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
        if (this->managers.Get<1>(managerIndex).OnLoad != nullptr)
            this->managers.Get<1>(managerIndex).OnLoad(world);
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
        if (this->managers.Get<1>(i).OnStart != nullptr)
            this->managers.Get<1>(i).OnStart(world);
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
        if (this->managers.Get<1>(managerIndex).OnSave != nullptr)
            this->managers.Get<1>(managerIndex).OnSave(world);
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
        if (this->managers.Get<1>(i).OnBeginFrame != nullptr)
            this->managers.Get<1>(i).OnBeginFrame();
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
        if (this->managers.Get<1>(i).OnFrame != nullptr)
            this->managers.Get<1>(i).OnFrame();
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
        if (this->managers.Get<1>(i).OnEndFrame != nullptr)
            this->managers.Get<1>(i).OnEndFrame();
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
        if (this->managers.Get<1>(i).OnDecay != nullptr)
            this->managers.Get<1>(i).OnDecay();
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
        if (this->managers.Get<1>(i).OnRenderDebug != nullptr)
            this->managers.Get<1>(i).OnRenderDebug();
    }
}

//------------------------------------------------------------------------------
/**
    Attach a manager to the game world. The manager's OnActivate()
    method will be called once right away, and then its OnFrame() method
    once per frame.
*/
ManagerHandle
FeatureUnit::AttachManager(ManagerAPI api)
{
    if (api.OnActivate != nullptr)
        api.OnActivate();

    Ids::Id32 handle;
    this->managerPool.Allocate(handle);

    uint32_t index = this->managers.Alloc();
    this->managers.Get<0>(index) = handle;
    this->managers.Get<1>(index) = std::move(api);

    return handle;
}

//------------------------------------------------------------------------------
/**
    Remove a manager from the game world. The manager's OnDeactivate()
    method will be called.
*/
void
FeatureUnit::RemoveManager(ManagerHandle handle)
{
    n_assert(this->managerPool.IsValid(handle.id));
    for (IndexT i = 0; i < this->managers.Size(); ++i)
    {
        if (this->managers.Get<0>(i) == handle)
        {
            if (this->managers.Get<1>(i).OnDeactivate != nullptr)
                this->managers.Get<1>(i).OnDeactivate();

            this->managers.EraseIndex(i);
            break;
        }
    }
    this->managerPool.Deallocate(handle.id);
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
    for (IndexT i = 0; i < this->managers.Size(); ++i)
    {
        if (this->managers.Get<1>(i).OnCleanup != nullptr)
            this->managers.Get<1>(i).OnCleanup(world);
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
