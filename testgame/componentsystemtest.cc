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
#include "basegamefeature/components/tagcomponent.h"
#include "timing/timer.h"

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
	
	Util::Array<Entity> entities;

	auto entity = eMgr->NewEntity();
	entities.Append(entity);

	TransformComponent::RegisterEntity(entity);
	
	Timing::Timer timer;

	timer.Reset();

	timer.Start();
	for (SizeT i = 0; i < 32768; i++)
	{
		if (i % 5 == 0)
		{
			TransformComponent::DeregisterEntity(entity);
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
		TransformComponent::RegisterEntity(entity);

		TagComponent::RegisterEntity(entity);

		this->gameApp->StepFrame();
	}
	timer.Stop();
	n_printf("Average step time = %f\n", timer.GetTime() / 32768.0f);

	for (SizeT i = 0; i < entities.Size(); i++)
	{
		eMgr->DeleteEntity(entities[i]);
	}

	Game::ComponentManager::Instance()->ClearAll();

	// Went through with no bugs, hurray!!
	VERIFY(true);
}

} // namespace Test
