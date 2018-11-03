//------------------------------------------------------------------------------
//  messagetest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "messagetest.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/entitymanager.h"
#include "game/messaging/message.h"
#include "util/delegate.h"

#include <chrono>
#include <ctime>

using namespace Game;

namespace Test
{
__ImplementClass(Test::MessageTest, 'msTS', Test::TestCase);

__DeclareMsg(TestMessage, 'TstM', uint32_t, Math::matrix44)

//------------------------------------------------------------------------------
/**
*/
void
MessageTest::Run()
{
	Ptr<ComponentManager> cMgr = ComponentManager::Instance();
	
	Ptr<TransformComponent> tComp = cMgr->GetComponent<TransformComponent>();

	auto listener = TestMessage::Register(TestMessage::Delegate::FromMethod<TransformComponent, &TransformComponent::SetLocalTransform>(tComp));

	auto a = Math::matrix44::identity();
	auto entity = Game::EntityManager::Instance()->NewEntity();
	tComp->RegisterEntity(entity);

	auto instance = tComp->GetInstance(entity);

	auto tstart = std::chrono::system_clock::now();
	for (SizeT i = 0; i < 100000; i++)
	{
		TestMessage::Send(instance, a);
	}
	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Send time: %f\n", elapsed_seconds.count());
	
	tstart = std::chrono::system_clock::now();
	for (SizeT i = 0; i < 100000; i++)
	{
		tComp->SetLocalTransform(instance, a);
	}
	tend = std::chrono::system_clock::now();
	elapsed_seconds = tend - tstart;
	n_printf("Direct call time: %f\n", elapsed_seconds.count());

	tComp->DestroyAll();
	Game::EntityManager::Instance()->InvalidateAllEntities();

	TestMessage::Deregister(listener);
}

}