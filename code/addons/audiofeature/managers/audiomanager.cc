//------------------------------------------------------------------------------
//  audiomanager.cc
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "audiomanager.h"
#include "game/gameserver.h"
#include "audiofeature/components/audiofeature.h"
#include "audio/audiodevice.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"

namespace AudioFeature
{

__ImplementClass(AudioFeature::AudioManager, 'AuMa', Game::Manager);
__ImplementSingleton(AudioManager)

//------------------------------------------------------------------------------
/**
*/
AudioManager::AudioManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
AudioManager::~AudioManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnDecay()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    {
        Game::ComponentDecayBuffer const decayBuffer = world->GetDecayBuffer(Game::GetComponentId<AudioEmitter>());
        AudioEmitter* data = (AudioEmitter*)decayBuffer.buffer;
        Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
        for (int i = 0; i < decayBuffer.size; i++)
        {
            audioDevice->UnloadClip(data[i].clipId);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::InitAudioEmitter(Game::World* world, Game::Entity entity, AudioEmitter* emitter)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    emitter->clipId = audioDevice->LoadClip(emitter->clipResource).id;
    if (emitter->autoplay)
    {
        if (world->HasComponent<SpatialAudioEmission>(entity))
        {
            SpatialAudioEmission const spatial = world->GetComponent<SpatialAudioEmission>(entity);
            Game::Position position = world->GetComponent<Game::Position>(entity);
            // TODO: We need a velocity component that we can read from
            Math::vec3 velocity = Math::vec3(0);
            Audio::ClipInstanceId clipInstanceId = audioDevice->PlaySpatial(
                emitter->clipId,
                emitter->volume,
                position,
                velocity,
                spatial.minDistance,
                spatial.maxDistance,
                emitter->loop,
                emitter->clock
            );
            ClipInstance* instance = world->AddComponent<ClipInstance>(entity);
            instance->id = clipInstanceId.id;
        }
        else
        {
            Audio::ClipInstanceId clipInstanceId =
                audioDevice->Play(emitter->clipId, emitter->volume, emitter->pan, emitter->loop, emitter->clock);
            ClipInstance* instance = world->AddComponent<ClipInstance>(entity);
            instance->id = clipInstanceId.id;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ValidateClipInstances(
    Game::World* world,
    Game::Entity const& entity,
    ClipInstance& clipInstance
)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    if (!audioDevice->IsValid(clipInstance.id))
    {
        world->RemoveComponent<ClipInstance>(entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateSpatialAudio(
    Game::World* world,
    Game::Entity const& entity,
    SpatialAudioEmission const&,
    ClipInstance& clipInstance,
    Game::Position const& position
)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    if (!audioDevice->IsValid(clipInstance.id))
    {
        return;
    }

    audioDevice->UpdatePosition(clipInstance.id, position);
    // TODO: We need a velocity component that we can read from
    Math::vec3 velocity = Math::vec3(0);
    audioDevice->UpdateVelocity(clipInstance.id, velocity);
}

//------------------------------------------------------------------------------
/**
*/
void
HandlePlayAudioEvent(Game::World* world, Game::Entity const& entity, AudioEmitter const& emitter)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    Audio::ClipInstanceId clipInstanceId =
        audioDevice->Play(emitter.clipId, emitter.volume, emitter.pan, emitter.loop, emitter.clock);
    ClipInstance* instance = world->AddComponent<ClipInstance>(entity);
    instance->id = clipInstanceId.id;
    world->RemoveComponent<PlayAudioEvent>(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
HandlePlaySpatialAudioEvent(
    Game::World* world,
    Game::Entity const& entity,
    Game::Position const& position,
    AudioEmitter const& emitter,
    SpatialAudioEmission const& spatial
)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    Math::vec3 velocity = Math::vec3(0);
    Audio::ClipInstanceId clipInstanceId = audioDevice->PlaySpatial(
        emitter.clipId, emitter.volume, position, velocity, spatial.minDistance, spatial.maxDistance, emitter.loop, emitter.clock
    );
    world->RemoveComponent<PlayAudioEvent>(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnActivate()
{
    using namespace Game;
    using namespace Audio;
    Game::Manager::OnActivate();

    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    ProcessorBuilder(world, "AudioManager.ValidateClipInstances")
        .Func(ValidateClipInstances)
        .On("OnFrame")
        .Order(50)
        .Build();

    ProcessorBuilder(world, "AudioManager.HandlePlayAudioEvent")
        .Excluding<SpatialAudioEmission, ClipInstance>()
        .Func(HandlePlayAudioEvent)
        .Including<PlayAudioEvent>()
        .On("OnFrame")
        .Order(51)
        .Build();

    ProcessorBuilder(world, "AudioManager.HandlePlaySpatialAudioEvent")
        .Excluding<ClipInstance>()
        .Func(HandlePlaySpatialAudioEvent)
        .Including<PlayAudioEvent>()
        .On("OnFrame")
        .Order(52)
        .Build();

    ProcessorBuilder(world, "AudioManager.UpdateSpatialAudio")
        .Excluding<Game::Static>()
        .Func(UpdateSpatialAudio)
        .On("OnFrame")
        .Order(53)
        .RunInEditor()
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnDeactivate()
{
    Game::Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::OnCleanup(Game::World* world)
{
    n_assert(AudioManager::HasInstance());
    // TODO: Implement me!
}

} // namespace AudioFeature
