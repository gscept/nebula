//------------------------------------------------------------------------------
//  audiofeature/audiofeatureunit.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "audiofeatureunit.h"
#include "basegamefeature/managers/timemanager.h"
#include "game/api.h"
#include "audiofeature/managers/audiomanager.h"
#include "game/world.h"
#include "components/audiofeature.h"

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
AudioFeatureUnit::OnAttach()
{
    Game::RegisterType<AudioEmitter>({ .decay = true, .OnInit = &AudioManager::InitAudioEmitter });
    Game::RegisterType<SpatialAudioEmission>();
    Game::RegisterType<AudioListener>();
    Game::RegisterType<ClipInstance>();
    Game::RegisterType<PlayAudioEvent>();
}

//------------------------------------------------------------------------------
/**
*/
void
AudioFeatureUnit::OnActivate()
{
    FeatureUnit::OnActivate();

    Game::TimeSourceCreateInfo timeSourceInfo;
    timeSourceInfo.hash = TIMESOURCE_AUDIO;
    Game::TimeManager::CreateTimeSource(timeSourceInfo);

    this->audioServer = Audio::AudioServer::Create();
    if (!this->audioServer->Open())
    {
        n_error("Could not open audio server!\n");
        return;
    }

    this->AttachManager(AudioManager::Create());
}

//------------------------------------------------------------------------------
/**
*/
void
AudioFeatureUnit::OnDeactivate()
{
    if (this->audioServer && this->audioServer->IsOpen())
    {
        this->audioServer->Close();
    }
    this->audioServer = nullptr;

    FeatureUnit::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void 
AudioFeatureUnit::OnBeginFrame()
{
    Game::TimeSource* const time = Game::TimeManager::GetTimeSource(TIMESOURCE_AUDIO);
    this->audioServer->OnFrame();
}

//------------------------------------------------------------------------------
/**
*/
void 
AudioFeatureUnit::OnRenderDebug()
{

}

} // namespace Game
