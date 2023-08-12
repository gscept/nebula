//------------------------------------------------------------------------------
//  audiodevice.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "audiodevice.h"
#include "soloud.h"
#include "soloud_wav.h"

#include "io/uri.h"

namespace Audio
{

__ImplementClass(Audio::AudioDevice, 'AIOD', Core::RefCounted)
__ImplementSingleton(Audio::AudioDevice)

static SoLoud::Soloud* soloud;

//------------------------------------------------------------------------------
/**
*/
AudioDevice::AudioDevice()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
AudioDevice::~AudioDevice()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
*/
bool
AudioDevice::Open()
{
    soloud = new SoLoud::Soloud;
    soloud->init(SoLoud::Soloud::CLIP_ROUNDOFF);

    this->ResetListener();

    _setup_grouped_timer(AudioOnFrameTime, "Audio Subsystem");
    _setup_grouped_counter(AudioNumberOfSoundsPlaying, "Audio Subsystem");

    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AudioDevice::Close()
{
    soloud->deinit();
    delete soloud;
    
    _discard_timer(AudioOnFrameTime);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::OnFrame()
{
    _start_timer(AudioOnFrameTime);

    soloud->update3dAudio();

    _stop_timer(AudioOnFrameTime);
    //_begin_counter(AudioNumberOfSoundsPlaying);
    //_set_counter(AudioNumberOfSoundsPlaying, soloud->getActiveVoiceCount());
    //_end_counter(AudioNumberOfSoundsPlaying);
}

//------------------------------------------------------------------------------
/**
*/
AudioEmitterId
AudioDevice::CreateAudioEmitter(Resources::ResourceName const& name)
{
    if (!name.IsValid())
    {
        return AudioEmitterId::Invalid();
    }

    ClipId clip;
    
    // first, load resource if we haven't already.
    IndexT index = this->clipMap.FindIndex(name);
    if (index != InvalidIndex)
    {
        clip = this->clipMap.ValueAtIndex(index);
    }
    else
    {
        clip = this->wavs.Alloc();
        IO::URI uri(name.Value());
        auto result = this->wavs.Get<WavAllocator::WAV>(clip.id).load(uri.LocalPath().AsCharPtr());
        if (result != SoLoud::SOLOUD_ERRORS::SO_NO_ERROR)
        {
            this->wavs.Dealloc(clip.id);
            return AudioEmitterId::Invalid();
        }

        this->clipMap.Add(name, clip);
        this->wavs.Get<WavAllocator::REFCOUNT>(clip.id) = 0;
    }

    AudioEmitterId aeid = this->emitterAllocator.Alloc();
    this->wavs.Get<WavAllocator::REFCOUNT>(clip.id)++;
    this->wavs.Get<WavAllocator::WAV>(clip.id).set3dAttenuation(SoLoud::AudioSource::ATTENUATION_MODELS::LINEAR_DISTANCE, 1.0f);
    this->emitterAllocator.Get<EmitterSlot::CLIPID>(aeid.id) = clip;
    this->emitterAllocator.Get<EmitterSlot::VOLUME>(aeid.id) = 1.0f;
    this->emitterAllocator.Get<EmitterSlot::MINDISTANCE>(aeid.id) = 1.0f;
    this->emitterAllocator.Get<EmitterSlot::MAXDISTANCE>(aeid.id) = 1000000.0f;
    this->emitterAllocator.Get<EmitterSlot::PAN>(aeid.id) = 0.0f;
    this->emitterAllocator.Get<EmitterSlot::SPATIALIZE>(aeid.id) = true;
    this->emitterAllocator.Get<EmitterSlot::CLOCK>(aeid.id) = 0.0f;

    return aeid;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::DestroyAudioEmitter(AudioEmitterId const id)
{
    ClipId clip = this->emitterAllocator.Get<EmitterSlot::CLIPID>(id.id);
    uint& refCount = this->wavs.Get<WavAllocator::REFCOUNT>(clip.id);
    refCount--;
    if (refCount == 0)
    {
        this->wavs.Dealloc(clip.id);
    }
    
    this->emitterAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
ClipInstanceId
AudioDevice::Play(AudioEmitterId id, bool loop)
{
    auto& clip = this->emitterAllocator.Get<EmitterSlot::CLIPID>(id.id);
    auto& vol = this->emitterAllocator.Get<EmitterSlot::VOLUME>(id.id);
    auto& spatialize = this->emitterAllocator.Get<EmitterSlot::SPATIALIZE>(id.id);
    auto& clock = this->emitterAllocator.Get<EmitterSlot::CLOCK>(id.id);
    auto& wav = this->wavs.Get<WavAllocator::WAV>(clip.id);

    ClipInstanceId instance;
    if (spatialize)
    {
        auto& pos = this->emitterAllocator.Get<EmitterSlot::POSITION>(id.id);
        auto& vel = this->emitterAllocator.Get<EmitterSlot::VELOCITY>(id.id);
        auto& min = this->emitterAllocator.Get<EmitterSlot::MINDISTANCE>(id.id);
        auto& max = this->emitterAllocator.Get<EmitterSlot::MAXDISTANCE>(id.id);
        
        if (clock > 0)
        {
            instance.id = soloud->play3dClocked(clock, wav, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z, vol);
        }
        else
        {
            instance.id = soloud->play3d(wav, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z, vol);
        }
        soloud->set3dSourceMinMaxDistance(instance.id, min, max);
    }
    else
    {
        auto& pan = this->emitterAllocator.Get<EmitterSlot::PAN>(id.id);
        if (clock == 0.0f)
        {
            instance.id = soloud->play(wav, vol, pan);
        }
        else
        {
            instance.id = soloud->playClocked(clock, wav, vol, pan);
        }
    }
    
    soloud->setLooping(instance.id, loop);
    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::StopInstance(ClipInstanceId id)
{
    soloud->stop(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::Stop(ClipId id)
{
    soloud->stopAudioSource(this->wavs.Get<WavAllocator::WAV>(id.id));
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetSpatialize(AudioEmitterId id, bool value)
{
    this->emitterAllocator.Get<EmitterSlot::SPATIALIZE>(id.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetPosition(AudioEmitterId id, Math::point const & pos)
{
    this->emitterAllocator.Get<EmitterSlot::POSITION>(id.id) = pos;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetVelocity(AudioEmitterId id, Math::vector const & vel)
{
    this->emitterAllocator.Get<EmitterSlot::VELOCITY>(id.id) = vel;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetMaxDistance(AudioEmitterId id, float value)
{
    this->emitterAllocator.Get<EmitterSlot::MAXDISTANCE>(id.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetMinDistance(AudioEmitterId id, float value)
{
    this->emitterAllocator.Get<EmitterSlot::MINDISTANCE>(id.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetClock(AudioEmitterId id, float value)
{
    this->emitterAllocator.Get<EmitterSlot::CLOCK>(id.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetPan(AudioEmitterId id, float value)
{
    this->emitterAllocator.Get<EmitterSlot::PAN>(id.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::UpdatePosition(ClipInstanceId id, Math::point const & pos)
{
    soloud->set3dSourcePosition(id.id, pos.x, pos.y, pos.z);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::UpdateVelocity(ClipInstanceId id, Math::vector const & vel)
{
    soloud->set3dSourceVelocity(id.id, vel.x, vel.y, vel.z);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetListenerTransform(Math::mat4 const & transform)
{
    auto& pos = transform.position;
    auto& fw = transform.z_axis;
    auto& up = transform.y_axis;
    this->listener.position[0] = pos.x;
    this->listener.position[1] = pos.y;
    this->listener.position[2] = pos.z;
    this->listener.forward[0] = -fw.x;
    this->listener.forward[1] = -fw.y;
    this->listener.forward[2] = -fw.z;
    this->listener.up[0] = up.x;
    this->listener.up[1] = up.y;
    this->listener.up[2] = up.z;

    soloud->set3dListenerParameters(
        this->listener.position[0],
        this->listener.position[1],
        this->listener.position[2],
        this->listener.forward[0],
        this->listener.forward[1],
        this->listener.forward[2],
        this->listener.up[0],
        this->listener.up[1],
        this->listener.up[2],
        this->listener.velocity[0],
        this->listener.velocity[1],
        this->listener.velocity[2]
    );
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetListenerVelocity(Math::vector const & vel)
{
    this->listener.velocity[0] = vel.x;
    this->listener.velocity[0] = vel.y;
    this->listener.velocity[0] = vel.z;
    soloud->set3dListenerVelocity(vel.x, vel.y, vel.z);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::SetSpeedOfSound(float value)
{
    soloud->set3dSoundSpeed(value);
}

//------------------------------------------------------------------------------
/**
*/
void
AudioDevice::ResetListener()
{
    this->listener = Listener();
}

//------------------------------------------------------------------------------
/**
*/
bool
AudioDevice::IsValid(ClipInstanceId id)
{
    return soloud->isValidVoiceHandle(id.id);
}

} // namespace Audio
