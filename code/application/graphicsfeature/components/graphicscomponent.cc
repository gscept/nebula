//------------------------------------------------------------------------------
//  graphicscomponent.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicscomponent.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "graphicsfeature/messages/graphicsprotocol.h"
#include "models/modelcontext.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "graphicsdata.h"
#include "game/component/componentserialization.h"

namespace GraphicsFeature
{

static GraphicsComponentData component;

__ImplementComponent(GraphicsFeature::GraphicsComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Create()
{
	component = GraphicsComponentData();

	__SetupDefaultComponentBundle(component);
	component.functions.OnActivate = OnActivate;
	component.functions.OnDeactivate = OnDeactivate;
	__RegisterComponent(&component);

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Discard()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::SetupAcceptedMessages()
{
	__RegisterMsg(Msg::UpdateTransform, UpdateTransform);
	__RegisterMsg(Msg::SetModel, SetModel);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnActivate(const uint32_t& instance)
{
	auto gfxEntity = Graphics::CreateEntity();
	component.data.Get<GraphicsComponentData::GRAPHICSENTITY>(instance) = gfxEntity.id;
	Models::ModelContext::RegisterEntity(gfxEntity);
	Models::ModelContext::Setup(gfxEntity, "mdl:Buildings/castle_tower.n3", "NONE");
	auto transform = Game::TransformComponent::GetWorldTransform(component.GetOwner(instance));
	Models::ModelContext::SetTransform(gfxEntity, transform);
	Visibility::ObservableContext::RegisterEntity(gfxEntity);
	Visibility::ObservableContext::Setup(gfxEntity, Visibility::VisibilityEntityType::Model);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnDeactivate(const uint32_t& instance)
{
	Graphics::GraphicsEntityId gfxEntity = { component.data.Get<GraphicsComponentData::GRAPHICSENTITY>(instance) };
	Models::ModelContext::DeregisterEntity(gfxEntity);
	Visibility::ObservableContext::DeregisterEntity(gfxEntity);
	Graphics::DestroyEntity(gfxEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::UpdateTransform(const Game::Entity & entity, const Math::matrix44 & transform)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Graphics::GraphicsEntityId gfxEntity = { component.data.Get<GraphicsComponentData::GRAPHICSENTITY>(instance) };
		Models::ModelContext::SetTransform(gfxEntity, transform);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::SetModel(const Game::Entity & entity, const Util::String & path)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Graphics::GraphicsEntityId gfxEntity = { component.data.Get<GraphicsComponentData::GRAPHICSENTITY>(instance) };
		Models::ModelContext::ChangeModel(gfxEntity, path, "NONE");
	}
}

} // namespace GraphicsFeature
