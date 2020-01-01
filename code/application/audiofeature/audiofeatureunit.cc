//------------------------------------------------------------------------------
//  audiofeature/audiofeatureunit.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "audiofeature/audiofeatureunit.h"
#include "audioserver.h"
#include "audiofeature/components/audioemittercomponent.h"

namespace AudioFeature
{
__ImplementClass(AudioFeature::AudioFeatureUnit, 'AUFU' , Game::FeatureUnit);
__ImplementSingleton(AudioFeatureUnit);

//------------------------------------------------------------------------------
/**
*/
AudioFeatureUnit::AudioFeatureUnit()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
AudioFeatureUnit::~AudioFeatureUnit()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioFeatureUnit::OnActivate()
{
	FeatureUnit::OnActivate();

	this->server = Audio::AudioServer::Create();
	this->server->Open();

	AudioEmitterComponent::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
AudioFeatureUnit::OnDeactivate()
{	
    FeatureUnit::OnDeactivate();

	this->server->Close();
	this->server = nullptr;

	AudioEmitterComponent::Discard();
}

} // namespace Game
