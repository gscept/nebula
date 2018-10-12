//------------------------------------------------------------------------------
//  animsamplebuffer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animsamplebuffer.h"
#include "animresource.h"
namespace CoreAnimation
{
__ImplementClass(CoreAnimation::AnimSampleBuffer, 'ASBF', Core::RefCounted);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimSampleBuffer::AnimSampleBuffer() :
	numSamples(0),
	samples(0),
	sampleCounts(0),
	samplesMapped(false),
	sampleCountsMapped(false)
{
	// empty
}    

//------------------------------------------------------------------------------
/**
*/
AnimSampleBuffer::~AnimSampleBuffer()
{
	if (this->IsValid())
	{
		this->Discard();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::Setup(const AnimResourceId& animRes)
{
	n_assert(!this->IsValid());
	n_assert(0 == this->samples);
	n_assert(0 == this->sampleCounts);
	n_assert(!this->samplesMapped);
	n_assert(!this->sampleCountsMapped);

	this->animResource = animRes;
	const Util::FixedArray<AnimClip>& clips = AnimGetClips(this->animResource);
	if (clips.Size() > 0) this->numSamples = clips[0].GetNumCurves();
	this->samples      = (float4*) Memory::Alloc(Memory::ResourceHeap, this->numSamples * sizeof(float4));

	// NOTE: sample count size must be aligned to 16 bytes, this allocate some more bytes in the buffer
	this->sampleCounts = (uchar*)  Memory::Alloc(Memory::ResourceHeap, (this->numSamples * sizeof(uchar)) + 16);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::Discard()
{
	n_assert(this->IsValid());
	n_assert(0 != this->samples);
	n_assert(0 != this->sampleCounts);
	n_assert(!this->samplesMapped);
	n_assert(!this->sampleCountsMapped);

	this->animResource = AnimResourceId::Invalid();
	Memory::Free(Memory::ResourceHeap, this->samples);
	Memory::Free(Memory::ResourceHeap, this->sampleCounts);
	this->samples = 0;
	this->sampleCounts = 0;
}

//------------------------------------------------------------------------------
/**
*/
Math::float4*
AnimSampleBuffer::MapSamples()
{
	n_assert(this->IsValid());
	n_assert(!this->samplesMapped);
	this->samplesMapped = true;
	return this->samples;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::UnmapSamples()
{
	n_assert(this->IsValid());
	n_assert(this->samplesMapped);
	this->samplesMapped = false;
}

//------------------------------------------------------------------------------
/**
*/
uchar*
AnimSampleBuffer::MapSampleCounts()
{
	n_assert(this->IsValid());
	n_assert(!this->sampleCountsMapped);
	this->sampleCountsMapped = true;
	return this->sampleCounts;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::UnmapSampleCounts()
{
	n_assert(this->IsValid());
	n_assert(this->sampleCountsMapped);
	this->sampleCountsMapped = false;
}

} // namespace CoreAnimation
