//------------------------------------------------------------------------------
//  messagetest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "messagetest.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/entitymanager.h"
#include "game/messaging/message.h"
#include "util/delegate.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "basegamefeature/managers/componentmanager.h"

#include <chrono>
#include <ctime>

using namespace Game;

namespace Test
{
__ImplementClass(Test::MessageTest, 'msTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
MessageTest::Run()
{
	Ptr<ComponentManager> cMgr = ComponentManager::Instance();
	
	auto a = Math::mat4();
	auto entity = Game::EntityManager::Instance()->NewEntity();
	TransformComponent::RegisterEntity(entity);

	auto instance = TransformComponent::GetInstance(entity);

	auto tstart = std::chrono::system_clock::now();
	for (SizeT i = 0; i < 100000; i++)
	{
		Msg::SetLocalTransform::Send(instance, a);
	}
	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Send time: %f\n", elapsed_seconds.count());
	
	tstart = std::chrono::system_clock::now();
	for (SizeT i = 0; i < 100000; i++)
	{
		TransformComponent::SetLocalTransform(instance, a);
	}
	tend = std::chrono::system_clock::now();
	elapsed_seconds = tend - tstart;
	n_printf("Direct call time: %f\n", elapsed_seconds.count());

	TransformComponent::DestroyAll();
	Game::EntityManager::Instance()->InvalidateAllEntities();
}

}