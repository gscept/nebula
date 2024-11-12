//------------------------------------------------------------------------------
//  game/manager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "game/manager.h"

namespace Game
{

__ImplementClass(Game::Manager, 'GAMA', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Manager::Manager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Manager::~Manager()
{
    //empty
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnActivate()
{
    n_assert(!this->isActive);
    this->isActive = true;
}

//------------------------------------------------------------------------------
/**
*/
void
Manager::OnDeactivate()
{
    n_assert(this->isActive);
    this->isActive = false;
}

}; // namespace Game
