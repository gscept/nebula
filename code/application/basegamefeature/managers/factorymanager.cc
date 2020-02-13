//------------------------------------------------------------------------------
//  factorymanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "factorymanager.h"
#include "entitymanager.h"
#include "categorymanager.h"

namespace Game
{

__ImplementClass(Game::FactoryManager, 'MFAM', Game::Manager);
__ImplementSingleton(FactoryManager)

//------------------------------------------------------------------------------
/**
*/
FactoryManager::FactoryManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
FactoryManager::~FactoryManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
FactoryManager::CreateEntityByCategory(Util::StringAtom const categoryName) const
{
	Entity entity = EntityManager::Instance()->CreateEntity();
	Ptr<CategoryManager> cm = CategoryManager::Instance();
	CategoryId cid = cm->GetCategoryId(categoryName);

	cm->AllocateInstance(entity, cid);

	return Game::Entity();
}

} // namespace Game


