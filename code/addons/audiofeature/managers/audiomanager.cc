//------------------------------------------------------------------------------
//  audiomanager.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "audiomanager.h"
#include "game/gameserver.h"
#include "audiofeature/components/audio.h"
#include "audio/audiodevice.h"

namespace AudioFeature
{

__ImplementSingleton(AudioManager)

//------------------------------------------------------------------------------
/**
*/
AudioManager::AudioManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AudioManager::~AudioManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnDecay()
{
    //Game::ComponentDecayBuffer const decayBuffer = Game::GetDecayBuffer(Singleton->pids.physicsActor);
    //Physics::ActorId* data = (Physics::ActorId*)decayBuffer.buffer;
    //for (int i = 0; i < decayBuffer.size; i++)
    //{
    //    Physics::DestroyActorInstance(data[i]);
    //}
}

//------------------------------------------------------------------------------
/**
*/
void
ActivateAudioEmitter(Game::World* world, Game::Entity& entity, AudioEmitter& emitter)
{
    //emitter.emitterId = Audio::AudioDevice::Instance()->CreateAudioEmitter(emitter.audioResource).id;
}

//------------------------------------------------------------------------------
/**
*/
void
DeactivateAudioEmitter(Game::World* world, Game::Entity& entity, AudioEmitter& emitter)
{
    Audio::AudioDevice::Instance()->DestroyAudioEmitter(emitter.emitterId);
    emitter.emitterId = -1;
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
AudioManager::Create()
{
    using namespace Game;
    using namespace Audio;
    n_assert(!AudioManager::HasInstance());
    AudioManager::Singleton = n_new(AudioManager);

    //ProcessorBuilder("AudioManager.ActivateAudioEmitter")
    //    .Func(ActivateAudioEmitter)
    //    .OnActivate<AudioEmitter>()
    //    .Build();
    //
    //ProcessorBuilder("AudioManager.DeactivateAudioEmitter")
    //    .Func(DeactivateAudioEmitter)
    //    .OnDeactivate<AudioEmitter>()
    //    .Build();
    //
    //ProcessorBuilder("AudioManager.UpdateAudio")
    //    .Func(
    //        [](World* world, AudioEmitter const& emitter)
    //        {
    //
    //        })
    //    .Build();

    Game::ManagerAPI api;
    api.OnCleanup    = &OnCleanup;
    api.OnDeactivate = &Destroy;
    api.OnDecay      = &OnDecay;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::Destroy()
{
    n_delete(AudioManager::Singleton);
    AudioManager::Singleton = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnCleanup(Game::World* world)
{
    n_assert(AudioManager::HasInstance());
}

} // namespace AudioFeature
