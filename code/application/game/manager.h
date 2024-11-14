#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Manager

    Managers are objects which care about some specific functionality
    of a feature.
    They should be subclassed by applications to implement aspects
    of the application (mainly game play related functionality).
    
    Managers are created and triggered by game features. The frame trigger
    functions are invoked when the gameserver triggers the game feature.
    
    @see NebulaApplication GameManagers

    @copyright
    (C) 2020-2024 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "ids/id.h"

//------------------------------------------------------------------------------
namespace Game
{

class World;

//------------------------------------------------------------------------------
/**
*/
class Manager : public Core::RefCounted
{
    __DeclareClass(Manager)
public:
    /// constructor
    Manager();
    /// destructor
    virtual ~Manager();

    /// return true if currently active
    bool IsActive() const;

    // TODO: RegisterTypes?

    virtual void OnActivate();
    /// called when removed from game server
    virtual void OnDeactivate();
    /// called before frame by the feature
    virtual void OnBeginFrame() {}
    /// Called between begin frame and before views
    virtual void OnBeforeViews() {}
    /// called per-frame by the feature
    virtual void OnFrame() {}
    /// called after frame by the feature
    virtual void OnEndFrame() {}
    /// called before cleaning up managed properties decay buffers
    virtual void OnDecay() {}
    /// called after loading game state
    virtual void OnLoad(World*) {}
    /// called before saving game state
    virtual void OnSave(World*) {}
    /// called before unloading game state
    virtual void OnCleanup(World*) {}
    /// called by Game::Server::Start()
    virtual void OnStart(World*) {}
    /// render a debug visualization
    virtual void OnRenderDebug() {}

private:
    bool isActive = false;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Manager::IsActive() const
{
    return this->isActive;
}

}; // namespace Game
//------------------------------------------------------------------------------
 
    
