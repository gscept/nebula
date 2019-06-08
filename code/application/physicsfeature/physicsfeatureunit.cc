//------------------------------------------------------------------------------
//  physicsfeature/physicsfeatureunit.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "Physicsfeature/Physicsfeatureunit.h"
#include "physicsinterface.h"
#include "graphics/graphicsserver.h"

namespace PhysicsFeature
{
__ImplementClass(PhysicsFeature::PhysicsFeatureUnit, 'PXFU' , Game::FeatureUnit);
__ImplementSingleton(PhysicsFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
PhysicsFeatureUnit::PhysicsFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsFeatureUnit::~PhysicsFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();
    Physics::Setup();
    Physics::CreateScene();
	//FIXME
    IndexT dummyMaterial = Physics::CreateMaterial("dummy"_atm, 0.8, 0.6, 0.3, 1.0);
	ActorComponent::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsFeatureUnit::OnDeactivate()
{	
    FeatureUnit::OnDeactivate();    
    ActorComponent::Discard();
    Physics::ShutDown();
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnBeginFrame()
{
    //FIXME use own timer or add to basegamefeatureunit
    Physics::Update(Graphics::GraphicsServer::Instance()->GetFrameTime());
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysicsFeatureUnit::OnRenderDebug()
{
    Physics::RenderDebug();
}

} // namespace Game
