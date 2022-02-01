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

namespace AudioFeature
{

class AudioManager
{
    __DeclareSingleton(AudioManager);
public:
    /// retrieve the api
    static Game::ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

private:
    /// constructor
    AudioManager();
    /// destructor
    ~AudioManager();

    static void OnDecay();
    static void OnCleanup(Game::World* world);
};

} // namespace AudioFeature
