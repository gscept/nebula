#pragma once
//------------------------------------------------------------------------------
/**
	AudioEmitterComponent

	Adds a sound resource to the entity and updates the spatial position and 
	velocity of the sound upon transform updates.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "resources/resourceid.h"

namespace AudioFeature
{

class AudioEmitterComponent
{
	__DeclareComponent(AudioEmitterComponent)
public:
	static void SetupAcceptedMessages();
	
	static void OnActivate(Game::InstanceId instance);
	static void OnDeactivate(Game::InstanceId instance);

	/// transform updated callback
	static void UpdateTransform(Game::Entity entity, const Math::matrix44 & transform);
	/// set the current audio resource of an entity
	static void SetAudioResource(Game::Entity entity, Util::String const& resource);
	/// set the current audio resource of an instance
	static void SetAudioResource(Game::InstanceId instance, Resources::ResourceName const& resource);

private:

};

} // namespace AudioFeature
