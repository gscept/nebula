#pragma once
//------------------------------------------------------------------------------
/**
	Audio::AudioServer

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"

namespace Audio
{

class AudioDevice;

class AudioServer : public Core::RefCounted
{
	__DeclareClass(AudioServer);
	__DeclareSingleton(AudioServer);
public:
	AudioServer();
	~AudioServer();

	/// Initialize the audio subsystem
	bool Open();
	/// Shutdown the audio subsystem
	bool Close();
	/// return true if the audio subsystem is open
	bool IsOpen() const;
	/// called per-frame 
	void OnFrame();
	

private:
	bool isOpen;
	Ptr<AudioDevice> device;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AudioServer::IsOpen() const
{
	return this->isOpen;
}

inline void AudioServer::OnFrame()
{
}

} // namespace Audio
