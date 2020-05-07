//------------------------------------------------------------------------------
//  graphicscomponent.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicscomponent.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "graphicsfeature/messages/graphicsprotocol.h"
#include "models/modelcontext.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "game/component/componentserialization.h"
#include "graphicsfeature/components/graphicsdata.h"
#include "debug/debugtimer.h"

namespace GraphicsFeature
{

static GraphicsComponentAllocator* component;

_declare_static_timer(GraphicsComponentOnEndFrame);

__ImplementComponent_woSerialization(GraphicsFeature::GraphicsComponent, component)

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Create()
{
	if (component != nullptr)
	{
		component->DestroyAll();
	}
	else
	{
		component = new GraphicsComponentAllocator();
	}

	component->DestroyAll();

	__SetupDefaultComponentBundle(component);
	component->functions.OnActivate = OnActivate;
	component->functions.OnDeactivate = OnDeactivate;
	component->functions.OnEndFrame = OnEndFrame;
	__RegisterComponent(component, "GraphicsComponent"_atm);

	SetupAcceptedMessages();

    _setup_grouped_timer(GraphicsComponentOnEndFrame, "Components");
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Discard()
{
    _discard_timer(GraphicsComponentOnEndFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::SetupAcceptedMessages()
{
	__RegisterMsg(Msg::SetModel, SetModel);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnActivate(Game::InstanceId instance)
{
	auto gfxEntity = Graphics::CreateEntity();
	component->Get<Attr::GraphicsEntity>(instance) = gfxEntity.id;
	Models::ModelContext::RegisterEntity(gfxEntity);
	Visibility::ObservableContext::RegisterEntity(gfxEntity);
	Models::ModelContext::Setup(gfxEntity, component->Get<Attr::ModelResource>(instance), "NONE", [gfxEntity]()
		{
			Visibility::ObservableContext::Setup(gfxEntity, Visibility::VisibilityEntityType::Model);
		});
	auto transform = Game::TransformComponent::GetWorldTransform(component->GetOwner(instance));
	Models::ModelContext::SetTransform(gfxEntity, transform);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnDeactivate(Game::InstanceId instance)
{
	Graphics::GraphicsEntityId gfxEntity = { component->Get<Attr::GraphicsEntity>(instance) };
	Visibility::ObservableContext::DeregisterEntity(gfxEntity);
	Models::ModelContext::DeregisterEntity(gfxEntity);
	Graphics::DestroyEntity(gfxEntity);
    component->Get<Attr::GraphicsEntity>(instance) = Graphics::GraphicsEntityId::Invalid().id;
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::SetModel(Game::Entity entity, const Util::String & path)
{
	auto instance = component->GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Graphics::GraphicsEntityId gfxEntity = { component->Get<Attr::GraphicsEntity>(instance) };
		Models::ModelContext::ChangeModel(gfxEntity, path, "NONE", nullptr);
		component->Get<Attr::ModelResource>(instance) = path;
		auto transform = Game::TransformComponent::GetWorldTransform(component->GetOwner(instance));
		Models::ModelContext::SetTransform(gfxEntity, transform);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::OnEndFrame()
{
    _start_timer(GraphicsComponentOnEndFrame);

    for (int i = 0; i < component->NumRegistered(); ++i)
    {
        Graphics::GraphicsEntityId gfxEntity = { component->Get<Attr::GraphicsEntity>(i) };
        if (gfxEntity != Graphics::GraphicsEntityId::Invalid())
        {
            Models::ModelContext::SetTransform(gfxEntity, Game::TransformComponent::GetWorldTransform(component->GetOwner(i)));
        }
    }

    _stop_timer(GraphicsComponentOnEndFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Serialize(const Ptr<IO::BinaryWriter>& writer)
{
	Game::Serialize(writer, component->data.GetArray<GraphicsComponentAllocator::GetAttributeIndex<Attr::ModelResource>()>());
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsComponent::Deserialize(const Ptr<IO::BinaryReader>& reader, uint offset, uint numInstances)
{
	Game::Deserialize(reader, component->data.GetArray<GraphicsComponentAllocator::GetAttributeIndex<Attr::ModelResource>()>(), offset, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
Util::FourCC
GraphicsComponent::GetFourCC()
{
	return component->GetIdentifier();
}

} // namespace GraphicsFeature
