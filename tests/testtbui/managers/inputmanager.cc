//------------------------------------------------------------------------------
//  inputmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "inputmanager.h"
#include "input/inputserver.h"
#include "input/mouse.h"
#include "input/keyboard.h"
#include "game/api.h"
#include "memdb/table.h"
#include "memdb/filterset.h"
#include "memdb/database.h"
#include "game/processor.h"
#include "game/world.h"

#include "audiofeature/components/audiofeature.h"

namespace Tests
{

__ImplementClass(Tests::InputManager, 'DInM', Game::Manager);
__ImplementSingleton(InputManager)

//------------------------------------------------------------------------------
/**
*/
InputManager::InputManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
InputManager::~InputManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
void
InputManager::OnActivate()
{
}

//------------------------------------------------------------------------------
/**
*/
void
InputManager::OnDeactivate()
{
    Game::Manager::OnDeactivate();
}


} // namespace Game


