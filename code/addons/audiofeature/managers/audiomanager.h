#pragma once
//------------------------------------------------------------------------------
/**
    @class  AudioFeature::AudioManager

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"
#include "audiofeature/components/audiofeature.h"

namespace AudioFeature
{

class AudioManager : public Game::Manager
{
    __DeclareClass(AudioManager)
    __DeclareSingleton(AudioManager)
public:
    AudioManager();
    virtual ~AudioManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnDecay() override;
    void OnCleanup(Game::World* world) override;

    static void InitAudioEmitter(Game::World*, Game::Entity, AudioEmitter*);
};

} // namespace AudioFeature
