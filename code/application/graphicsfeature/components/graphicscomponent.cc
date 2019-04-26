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
#include "game/component/componentserialization.h"

namespace GraphicsFeature
{

static Game::Component<GraphicsEntity, ModelResource> component;

__ImplementComponent_woSerialization(GraphicsFeature::GraphicsComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Create()
{
	component.DestroyAll();

	__SetupDefaultComponentBundle(component);
	component.functions.OnActivate = OnActivate;
	component.functions.OnDeactivate = OnDeactivate;
	__RegisterComponent(&component, "GraphicsComponent"_atm, GetFourCC());

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
GraphicsComponent::OnActivate(Game::InstanceId instance)
{
	auto gfxEntity = Graphics::CreateEntity();
	component.Get<GraphicsEntity>(instance) = gfxEntity.id;
	Models::ModelContext::RegisterEntity(gfxEntity);
	Models::ModelContext::Setup(gfxEntity, component.Get<ModelResource>(instance), "NONE");
	auto transform = Game::TransformComponent::GetWorldTransform(component.GetOwner(instance));
	Models::ModelContext::SetTransform(gfxEntity, transform);
	Visibility::ObservableContext::RegisterEntity(gfxEntity);
	Visibility::ObservableContext::Setup(gfxEntity, Visibility::VisibilityEntityType::Model);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnDeactivate(Game::InstanceId instance)
{
	Graphics::GraphicsEntityId gfxEntity = { component.Get<GraphicsEntity>(instance) };
	Models::ModelContext::DeregisterEntity(gfxEntity);
	Visibility::ObservableContext::DeregisterEntity(gfxEntity);
	Graphics::DestroyEntity(gfxEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::UpdateTransform(Game::Entity entity, const Math::matrix44 & transform)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Graphics::GraphicsEntityId gfxEntity = { component.Get<GraphicsEntity>(instance) };
		Models::ModelContext::SetTransform(gfxEntity, transform);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::SetModel(Game::Entity entity, const Util::String & path)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Graphics::GraphicsEntityId gfxEntity = { component.Get<GraphicsEntity>(instance) };
		Models::ModelContext::ChangeModel(gfxEntity, path, "NONE");
		component.Get<ModelResource>(instance) = path;
		auto transform = Game::TransformComponent::GetWorldTransform(component.GetOwner(instance));
		Models::ModelContext::SetTransform(gfxEntity, transform);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Serialize(const Ptr<IO::BinaryWriter>& writer)
{
	Game::Serialize(writer, component.data.GetArray<component.GetAttributeIndex<ModelResource>()>());
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	Game::Deserialize(reader, component.data.GetArray<component.GetAttributeIndex<ModelResource>()>(), offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
GraphicsComponent::GetFourCC()
{
	return 'grpc';
}

} // namespace GraphicsFeature
