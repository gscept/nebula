#pragma once
//------------------------------------------------------------------------------
/**
    @class AudioFeature::AudioFeatureUnit

    Sets up and interfaces with the audio subsystem.
    
    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "audio/audioserver.h"

#define TIMESOURCE_AUDIO 'AUTS'

//------------------------------------------------------------------------------
namespace AudioFeature
{

class AudioFeatureUnit : public Game::FeatureUnit
{
    __DeclareClass(AudioFeatureUnit)
    __DeclareSingleton(AudioFeatureUnit)

public:

    /// constructor
    AudioFeatureUnit();
    /// destructor
    ~AudioFeatureUnit();

    void OnAttach();

    void OnActivate();
    void OnDeactivate();
    virtual void OnBeginFrame();
    virtual void OnRenderDebug();

private:
    Ptr<Audio::AudioServer> audioServer;
};

} // namespace AudioFeature
//------------------------------------------------------------------------------
