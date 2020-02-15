//------------------------------------------------------------------------------
//  transformmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "transformmanager.h"
#include "entitymanager.h"

namespace Game
{

__ImplementClass(Game::TransformManager, 'MTRN', Game::Manager);
__ImplementSingleton(TransformManager)

//------------------------------------------------------------------------------
/**
*/
TransformManager::TransformManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TransformManager::~TransformManager()
{
	__DestructSingleton;
}

} // namespace Game


