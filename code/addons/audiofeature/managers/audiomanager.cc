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
ActivateAudioEmitter(Game::World* world, Game::Entity const& entity, AudioEmitter& emitter)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    emitter.clipId = audioDevice->LoadClip(emitter.clipResource).id;
    if (emitter.autoplay)
    {
        Audio::ClipInstanceId clipInstanceId =
            audioDevice->Play(emitter.clipId, emitter.volume, emitter.pan, emitter.loop, emitter.clock);
        ClipInstance* instance = world->AddComponent<ClipInstance>(entity);
        instance->id = clipInstanceId.id;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ActivateSpatialAudioEmitter(
    Game::World* world,
    Game::Entity const& entity,
    AudioEmitter& emitter,
    SpatialAudioEmission const& spatial,
    Game::Position const& position
)
{
    Ptr<Audio::AudioDevice> audioDevice = Audio::AudioDevice::Instance();
    emitter.clipId = audioDevice->LoadClip(emitter.clipResource).id;
    if (emitter.autoplay)
    {
        // TODO: We need a velocity component that we can read from
        Math::vec3 velocity = Math::vec3(0);
        Audio::ClipInstanceId clipInstanceId = audioDevice->PlaySpatial(
            emitter.clipId,
            emitter.volume,
            position,
            velocity,
            spatial.minDistance,
            spatial.maxDistance,
            emitter.loop,
            emitter.clock
        );
        ClipInstance* instance = world->AddComponent<ClipInstance>(entity);
        instance->id = clipInstanceId.id;
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
Game::ManagerAPI
AudioManager::Create()
{
    using namespace Game;
    using namespace Audio;
    n_assert(!AudioManager::HasInstance());
    AudioManager::Singleton = new AudioManager;

    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    ProcessorBuilder(world, "AudioManager.ActivateAudioEmitter")
        .Excluding<SpatialAudioEmission>()
        .Func(ActivateAudioEmitter)
        .On("OnActivate")
        .Build();

    ProcessorBuilder(world, "AudioManager.ActivateSpatialAudioEmitter")
        .Func(ActivateSpatialAudioEmitter)
        .On("OnActivate")
        .Build();

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
        .Build();

    Game::ManagerAPI api;
    api.OnCleanup = &OnCleanup;
    api.OnDeactivate = &Destroy;
    api.OnDecay = &OnDecay;
    return api;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioManager::Destroy()
{
    delete AudioManager::Singleton;
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
