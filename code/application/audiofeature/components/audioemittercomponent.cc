//------------------------------------------------------------------------------
//  audioemittercomponent.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "audioemittercomponent.h"
#include "basegamefeature/messages/basegameprotocol.h"
#include "basegamefeature/components/transformcomponent.h"
#include "basegamefeature/managers/componentmanager.h"
#include "audiodevice.h"
#include "audiofeature/messages/audioprotocol.h"

namespace AudioFeature
{

static Game::Component<AudioEmitter, AudioResource, Autoplay, Loop, Spatialize> component;

__ImplementComponent(AudioFeature::AudioEmitterComponent, component)

using namespace Audio;

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::Create()
{
	component.DestroyAll();

	__SetupDefaultComponentBundle(component);
	component.functions.OnActivate = OnActivate;
	component.functions.OnDeactivate = OnDeactivate;
	__RegisterComponent(&component, "AudioEmitterComponent"_atm, 'audc');

	SetupAcceptedMessages();
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::Discard()
{
	
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::SetupAcceptedMessages()
{
	__RegisterMsg(Msg::UpdateTransform, UpdateTransform);
	__RegisterMsg(Msg::SetAudioResource, SetAudioResource);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::OnActivate(Game::InstanceId instance)
{
	Resources::ResourceName resource = component.Get<AudioResource>(instance);
	SetAudioResource(instance, resource);
	auto const& emitter = component.Get<AudioEmitter>(instance);
	AudioDevice::Instance()->SetSpatialize(emitter, component.Get<Spatialize>(instance));

	if (component.Get<Autoplay>(instance))
	{
		AudioDevice::Instance()->Play(emitter, component.Get<Loop>(instance));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::OnDeactivate(Game::InstanceId instance)
{
	AudioEmitterId emitter = { component.Get<AudioEmitter>(instance) };
	AudioDevice::Instance()->DestroyAudioEmitter(emitter);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::UpdateTransform(Game::Entity entity, const Math::matrix44 & transform)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		AudioEmitterId emitter = { component.Get<AudioEmitter>(instance) };
		AudioDevice::Instance()->SetPosition(emitter, transform.get_position());
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::SetAudioResource(Game::Entity entity, Util::String const & resource)
{
	auto instance = component.GetInstance(entity);
	if (instance != InvalidIndex)
	{
		Resources::ResourceName resourceName = resource;
		SetAudioResource(instance, resourceName);
		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::SetAudioResource(Game::InstanceId instance, Resources::ResourceName const & resource)
{
	AudioEmitterId audioEmitter = component.Get<AudioEmitter>(instance);
	if (audioEmitter != AudioEmitterId::Invalid())
	{
		AudioDevice::Instance()->DestroyAudioEmitter(audioEmitter);
	}

	component.Get<AudioResource>(instance) = resource.Value();
	audioEmitter = AudioDevice::Instance()->CreateAudioEmitter(resource);
	component.Get<AudioEmitter>(instance) = audioEmitter.id;

	if (audioEmitter != AudioEmitterId::Invalid())
	{
		auto transform = Game::TransformComponent::GetWorldTransform(component.GetOwner(instance));
		AudioDevice::Instance()->SetPosition(audioEmitter, transform.get_position());
	}
}

} // namespace AudioFeature
