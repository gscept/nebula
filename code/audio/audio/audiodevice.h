#pragma once
//------------------------------------------------------------------------------
/**
    @class Audio::AudioDevice

    Central class of the core audio subsystem.
    Currently implements Soloud as the audio engine.

    Uses which ever audio backend that it is compiled with
    and that initializes properly.

    Audio clips/resources are loaded and shared between audio emitters until
    their reference count is 0, upon which they are unloaded.

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "ids/idallocator.h"
#include "audioclip.h"
#include "debug/debugtimer.h"
#include "debug/debugcounter.h"

namespace SoLoud
{
class Wav;
}

namespace Audio
{

ID_32_TYPE(AudioEmitterId);

class AudioDevice : public Core::RefCounted
{
    __DeclareClass(AudioDevice);
    __DeclareSingleton(AudioDevice);

public:
    AudioDevice();
    ~AudioDevice();

    /// Initialize the audio engine
    bool Open();
    /// Shutdown the audio engine
    bool Close();

    /// Called per frame to update spatial positions
    void OnFrame();

    /// Load a soundfile into memory
    ClipId LoadClip(Resources::ResourceName const& name);
    /// Unload a soundfile
    void UnloadClip(ClipId const id);

    /// Play non-spatial audio clip. Returns the playing clip instance.
    ClipInstanceId Play(ClipId clip, float volume, float pan, bool loop = false, float clock = 0.0f);

    /// Play spatial audio clip that accounts for spatial position when playing. Returns the playing clip instance.
    ClipInstanceId PlaySpatial(
        ClipId clip,
        float volume,
        Math::vec3 const& position,
        Math::vec3 const& velocity,
        float minDistance,
        float maxDistance,
        bool loop = false,
        float clock = 0.0f
    );

    /// Stop an instance of a clip
    void StopInstance(ClipInstanceId id);
    /// Stop all instances of a clip
    void Stop(ClipId id);
    /// Check if an instance is valid
    bool IsValid(ClipInstanceId id);

    /// Update the spatial position of a sound instance in world space.
    void UpdatePosition(ClipInstanceId id, Math::point const& pos);
    /// Update the spatial velocity of a sound instance
    void UpdateVelocity(ClipInstanceId id, Math::vector const& vel);

    /// Set the listener transform in world space
    void SetListenerTransform(Math::mat4 const& transform);
    /// Set the listeners velocity
    void SetListenerVelocity(Math::vector const& velocity);
    /// Reset listener
    void ResetListener();

    /// Set the speed of sound constant. (default assumes 1 unit is 1 meter)
    void SetSpeedOfSound(float value = 343.0f);


    /// Load a soundfile into memory
    AudioEmitterId CreateAudioEmitter(Resources::ResourceName const& name);
    /// Destroy an audio emitter
    void DestroyAudioEmitter(AudioEmitterId const id);
    /// Play clip. Returns the playing clip instance.
    ClipInstanceId Play(AudioEmitterId id, bool loop = false);
    /// Set whether or not to account for spatial position when playing a sound.
    void SetSpatialize(AudioEmitterId id, bool value);
    /// Set spatial position of a sound in world space
    void SetPosition(AudioEmitterId id, Math::point const& pos);
    /// Set spatial velocity of a sound
    void SetVelocity(AudioEmitterId id, Math::vector const& vel);
    /// Set the max audible distance of a sound
    void SetMaxDistance(AudioEmitterId id, float value);
    /// Set the min audible distance of a sound
    void SetMinDistance(AudioEmitterId id, float value);
    /// Set min delay between subsequent instances of an emitted sound (value of 0 disables)
    void SetClock(AudioEmitterId id, float value);
    /// Set 2D pan for non spatialized sounds. (L = -1.0, R = 1.0)
    void SetPan(AudioEmitterId id, float value);

private:
    enum EmitterSlot
    {
        CLIPID,      // Clip that this emitter will be playing
        POSITION,    // spatial 3D position in world space coordinates
        VELOCITY,    // velocity used for doppler effect
        VOLUME,      // audio max volume
        MINDISTANCE, // Min distance before audio attenuation starts
        MAXDISTANCE, // Max audible distance
        PAN,         // 2D pan for non spatialized sounds. (L = -1.0, R = 1.0)
        SPATIALIZE,  // Set true if spatial position and velocity should be taken into account when playing sound
        CLOCK, // Set this to > 0 if you need to delay the start of sounds so that rapidly launched sounds don't all get clumped to the start of the next outgoing sound buffer.
    };
    Ids::IdAllocator<ClipId, Math::point, Math::vector, float, float, float, float, bool, float> emitterAllocator;

    enum WavAllocator
    {
        WAV,
        REFCOUNT
    };
    /**
        Contains all wavs that are currently loaded.
        refcount will automatically unload wav if it
        is no longer in use by any emitters
    */
    Ids::IdAllocator<SoLoud::Wav, uint> wavs;

    /// resource -> clipid table
    Util::Dictionary<Resources::ResourceName, ClipId> clipMap;

    struct Listener
    {
        float forward[3] = {0, 0, 1};
        float up[3] = {0, 1, 0};
        float position[3] = {0, 0, 0};
        float velocity[3] = {0, 0, 0};
    } listener;

    _declare_timer(AudioOnFrameTime) _declare_counter(AudioNumberOfSoundsPlaying)
};

} // namespace Audio
