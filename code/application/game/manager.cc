//------------------------------------------------------------------------------
//  game/manager.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/manager.h"

namespace Game
{
__ImplementClass(Game::Manager, 'GAMA', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Manager::Manager() :
    isActive(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Manager::~Manager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This method is called when the manager is attached to the game server.
    The manager base class will register its message port with the
    message server.
*/
void
Manager::OnActivate()
{
    n_assert(!this->isActive);
    this->isActive = true;
}

//------------------------------------------------------------------------------
/**
    This method is called when the manager is removed from the game server.
    It will unregister its message port from the message server at this point.
*/
void
Manager::OnDeactivate()
{
    n_assert(this->isActive);
    this->isActive = false;
}

//------------------------------------------------------------------------------
/**
    Called before frame, override in subclasses
*/
void
Manager::OnBeginFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Called after frame, override in subclasses
*/
void
Manager::OnEndFrame()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnLoad()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnRenderDebug()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnSave()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnStart()
{
    // empty
}

}; // namespace Game
