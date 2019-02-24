#pragma once
//------------------------------------------------------------------------------
/**
	AudioEmitterComponent

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

	static void UpdateTransform(Game::Entity entity, const Math::matrix44 & transform);
	static void SetAudioResource(Game::Entity entity, Util::String const& resource);
	static void SetAudioResource(Game::InstanceId instance, Resources::ResourceName const& resource);
private:

};

} // namespace AudioFeature
