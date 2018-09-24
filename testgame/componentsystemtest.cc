//------------------------------------------------------------------------------
//  componentsystemtest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "componentsystemtest.h"
#include "appgame/gameapplication.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"
#include "util/random.h"

using namespace Game;

__ImplementClass(Test::ComponentSystemTest, 'CpTs', Test::TestCase)

namespace Test
{

//------------------------------------------------------------------------------
/**
*/
void
ComponentSystemTest::Run()
{
	Ptr<EntityManager> eMgr = EntityManager::Instance();
	Ptr<ComponentManager> cMgr = ComponentManager::Instance();

	Ptr<TransformComponent> tComp = cMgr->GetComponent<TransformComponent>();

	Util::Array<Entity> entities;

	auto entity = eMgr->NewEntity();
	entities.Append(entity);
	
	tComp->RegisterEntity(entity);
	for (SizeT i = 0; i < 32768; i++)
	{
		if (i % 5 == 0)
		{
			tComp->DeregisterEntity(entity);
		}
		else if (i % 50 == 0)
		{
			int k = 0;
			while (k < 20)
			{
				SizeT size = entities.Size();
				uint index = Util::FastRandom() % size;
				Entity e = entities[index];
				entities.EraseIndexSwap(index);
				eMgr->DeleteEntity(e);
				k++;
			}
		}
		
		entity = eMgr->NewEntity();
		entities.Append(entity);
		tComp->RegisterEntity(entity);

		this->gameApp->StepFrame();
	}

	for (SizeT i = 0; i < entities.Size(); i++)
	{
		eMgr->DeleteEntity(entities[i]);
	}

	tComp->DestroyAll();
	tComp->CleanData();

	// Went through with no bugs, hurray!!
	this->Verify(true);
}

} // namespace Test
