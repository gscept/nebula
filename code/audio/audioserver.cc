//------------------------------------------------------------------------------
//  audioserver.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "audioserver.h"
#include "audiodevice.h"

namespace Audio
{

__ImplementClass(Audio::AudioServer, 'AIOS', Core::RefCounted)
__ImplementSingleton(Audio::AudioServer)

//------------------------------------------------------------------------------
/**
*/
AudioServer::AudioServer()
{
	__ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
AudioServer::~AudioServer()
{
	__DestructSingleton
}

bool AudioServer::Open()
{
	n_assert(!this->IsOpen());
	this->device = AudioDevice::Create();
	this->device->Open();
	this->isOpen = true;
	return true;
}

bool AudioServer::Close()
{
	n_assert(this->IsOpen());
	this->device->Close();
	this->device = nullptr;
	this->isOpen = false;
	return true;
}

} // namespace Audio
