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
#include "basegamefeature/components/tagcomponent.h"

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
	Ptr<Game::TagComponent> tagComp = componentManager->GetComponent<Game::TagComponent>();

	Util::Array<Entity> entities;

	Game::Entity entity;
	
	tComp->DestroyAll();

	const auto numIter = 10000;

	// fill scene
	for (SizeT i = 0; i < numIter; i++)
	{
		entity = entityManager->NewEntity();
		entities.Append(entity);

		auto instance = tComp->RegisterEntity(entity);
		tComp->SetAttributeValue(instance, Attr::Parent, uint(i));
		tagComp->RegisterEntity(entity);
	}

	// Save level
	VERIFY(LevelLoader::Save("bin:test.scnb"));

	tComp->DestroyAll();
	tagComp->DestroyAll();

	// Delete all entities.
	for (SizeT i = 0; i < entities.Size(); i++)
		entityManager->DeleteEntity(entities[i]);

	VERIFY(tComp->NumRegistered() == 0);
	VERIFY(entityManager->GetNumEntities() == 0);

	// Load level without any entities in the scene
	VERIFY(LevelLoader::Load("bin:test.scnb"));

	VERIFY(tComp->NumRegistered() != 0);
	VERIFY(entityManager->GetNumEntities() != 0);

	// Load level with a number of entities already registered to the scene
	VERIFY(LevelLoader::Load("bin:test.scnb"));

	tagComp->DestroyAll();

	// Load with partial data destroyed.
	VERIFY(LevelLoader::Load("bin:test.scnb"));

	componentManager->DeregisterComponent(tagComp);
	
	// load level with a missing component
	VERIFY(LevelLoader::Load("bin:test.scnb"));

	componentManager->DeregisterComponent(tComp);

	// load level with no components at all
	VERIFY(LevelLoader::Load("bin:test.scnb"));

	entityManager->InvalidateAllEntities();
	entities.Clear();
}

}