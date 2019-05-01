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
#include "audiofeature/components/audioemitterallocator.h"

namespace AudioFeature
{

static AudioEmitterComponentAllocator* component;

__ImplementComponent(AudioFeature::AudioEmitterComponent, component)

using namespace Audio;

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::Create()
{
	component->DestroyAll();

	__SetupDefaultComponentBundle(component);
	component->functions.OnActivate = OnActivate;
	component->functions.OnDeactivate = OnDeactivate;
	__RegisterComponent(component, "AudioEmitterComponent"_atm);

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
	Resources::ResourceName resource = component->Get<Attr::AudioResource>(instance);
	SetAudioResource(instance, resource);
	auto const& emitter = component->Get<Attr::AudioEmitter>(instance);
	AudioDevice::Instance()->SetSpatialize(emitter, component->Get<Attr::Spatialize>(instance));

	if (component->Get<Attr::Autoplay>(instance))
	{
		AudioDevice::Instance()->Play(emitter, component->Get<Attr::Loop>(instance));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::OnDeactivate(Game::InstanceId instance)
{
	AudioEmitterId emitter = { component->Get<Attr::AudioEmitter>(instance) };
	AudioDevice::Instance()->DestroyAudioEmitter(emitter);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::UpdateTransform(Game::Entity entity, const Math::matrix44 & transform)
{
	auto instance = component->GetInstance(entity);
	if (instance != InvalidIndex)
	{
		AudioEmitterId emitter = { component->Get<Attr::AudioEmitter>(instance) };
		AudioDevice::Instance()->SetPosition(emitter, transform.get_position());
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AudioEmitterComponent::SetAudioResource(Game::Entity entity, Util::String const & resource)
{
	auto instance = component->GetInstance(entity);
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
	AudioEmitterId audioEmitter = component->Get<Attr::AudioEmitter>(instance);
	if (audioEmitter != AudioEmitterId::Invalid())
	{
		AudioDevice::Instance()->DestroyAudioEmitter(audioEmitter);
	}

	component->Get<Attr::AudioResource>(instance) = resource.Value();
	audioEmitter = AudioDevice::Instance()->CreateAudioEmitter(resource);
	component->Get<Attr::AudioEmitter>(instance) = audioEmitter.id;

	if (audioEmitter != AudioEmitterId::Invalid())
	{
		auto transform = Game::TransformComponent::GetWorldTransform(component->GetOwner(instance));
		AudioDevice::Instance()->SetPosition(audioEmitter, transform.get_position());
	}
}

} // namespace AudioFeature
