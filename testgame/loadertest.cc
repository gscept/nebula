//------------------------------------------------------------------------------
//  loadertest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "loadertest.h"

#include "basegamefeature/loader/userprofile.h"
#include "basegamefeature/loader/levelloader.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/components/transformcomponent.h"

using namespace BaseGameFeature;
using namespace Game;

namespace Test
{
__ImplementClass(Test::LoaderTest, 'LDTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
LoaderTest::Run()
{
	Ptr<Game::ComponentManager> componentManager = Game::ComponentManager::Instance();

	Ptr<Game::EntityManager> entityManager = Game::EntityManager::Instance();

	Ptr<Game::TransformComponent> tComp = componentManager->GetComponent<Game::TransformComponent>();

	Util::Array<Entity> entities;

	Game::Entity entity;
	
	tComp->DestroyAll();

	// fill scene
	for (SizeT i = 0; i < 100000; i++)
	{
		entity = entityManager->NewEntity();
		entities.Append(entity);

		auto instance = tComp->RegisterEntity(entity);
		tComp->SetAttributeValue(instance, Attr::Parent, uint(i));
	}

	LevelLoader::Save("bin:test.scnb");

	tComp->DestroyAll();

	// Delete all entities.
	for (SizeT i = 0; i < entities.Size(); i++)
		entityManager->DeleteEntity(entities[i]);

	VERIFY(tComp->NumRegistered() == 0);
	VERIFY(entityManager->GetNumEntities() == 0);

	LevelLoader::Load("bin:test.scnb");

	VERIFY(tComp->NumRegistered() != 0);
	VERIFY(entityManager->GetNumEntities() != 0);

	tComp->DestroyAll();
	entities.Clear();
}

}