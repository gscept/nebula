//------------------------------------------------------------------------------
//  animsamplebuffer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animsamplebuffer.h"
namespace CoreAnimation
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimSampleBuffer::AnimSampleBuffer() :
    numSamples(0),
    samples(nullptr),
    sampleCounts(nullptr)
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
AnimSampleBuffer::AnimSampleBuffer(AnimSampleBuffer&& rhs) :
    numSamples(rhs.numSamples),
    animResource(rhs.animResource)
{
    // erase any current arrays
    if (this->samples)
        Memory::Free(Memory::ResourceHeap, this->samples);
    if (this->sampleCounts)
        Memory::Free(Memory::ResourceHeap, this->sampleCounts);

    this->samples = rhs.samples;
    this->sampleCounts = rhs.sampleCounts;

    rhs.numSamples = 0;
    rhs.samples = nullptr;
    rhs.sampleCounts = nullptr;
    rhs.animResource = InvalidAnimationId;
}

//------------------------------------------------------------------------------
/**
*/
AnimSampleBuffer::AnimSampleBuffer(const AnimSampleBuffer& rhs) :
    numSamples(rhs.numSamples),
    animResource(rhs.animResource)
{
    if (this->samples)
        Memory::Free(Memory::ResourceHeap, this->samples);
    if (this->sampleCounts)
        Memory::Free(Memory::ResourceHeap, this->sampleCounts);

    if (this->numSamples > 0)
    {
        this->samples = (float*)Memory::Alloc(Memory::ResourceHeap, this->numSamples * sizeof(float));
        memcpy(this->samples, rhs.samples, this->numSamples * sizeof(float));

        this->sampleCounts = (uchar*)Memory::Alloc(Memory::ResourceHeap, (this->numSamples * sizeof(uchar)) + 16);
        memcpy(this->sampleCounts, rhs.sampleCounts, (this->numSamples * sizeof(uchar)) + 16);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSampleBuffer::operator=(AnimSampleBuffer&& rhs)
{
    this->numSamples = rhs.numSamples;
    this->animResource = rhs.animResource;

    // erase any current arrays
    if (this->samples)
        Memory::Free(Memory::ResourceHeap, this->samples);
    if (this->sampleCounts)
        Memory::Free(Memory::ResourceHeap, this->sampleCounts);

    this->samples = rhs.samples;
    this->sampleCounts = rhs.sampleCounts;

    rhs.numSamples = 0;
    rhs.samples = nullptr;
    rhs.sampleCounts = nullptr;
    rhs.animResource = InvalidAnimationId;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::operator=(const AnimSampleBuffer& rhs)
{
    this->numSamples = rhs.numSamples;
    this->animResource = rhs.animResource;
    if (this->samples)
        Memory::Free(Memory::ResourceHeap, this->samples);
    if (this->sampleCounts)
        Memory::Free(Memory::ResourceHeap, this->sampleCounts);

    if (this->numSamples > 0)
    {
        this->samples = (float*)Memory::Alloc(Memory::ResourceHeap, this->numSamples * sizeof(float));
        memcpy(this->samples, rhs.samples, this->numSamples * sizeof(float));

        this->sampleCounts = (uchar*)Memory::Alloc(Memory::ResourceHeap, (this->numSamples * sizeof(uchar)) + 16);
        memcpy(this->sampleCounts, rhs.sampleCounts, (this->numSamples * sizeof(uchar)) + 16);
    }   
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleBuffer::Setup(const AnimationId& animRes)
{
    n_assert(!this->IsValid());
    n_assert(0 == this->samples);
    n_assert(0 == this->sampleCounts);

    this->animResource = animRes;
    const Util::FixedArray<AnimClip>& clips = AnimGetClips(this->animResource);
    SizeT maxCurveCount = 0;
    for (const auto& clip : clips)
        maxCurveCount = Math::max(maxCurveCount, clip.numCurves);

    // The amount of samples we need is going to be the amount of curves, which is pos/rot/scale / 3 * 10
    this->numSamples = (maxCurveCount / 3) * 10;
    this->samples      = (float*) Memory::Alloc(Memory::ResourceHeap, this->numSamples * sizeof(float));

    // NOTE: sample count size must be aligned to 16 bytes, this allocate some more bytes in the buffer
    this->sampleCounts = (uchar*)  Memory::Alloc(Memory::ResourceHeap, (maxCurveCount * sizeof(uchar)) + 16);
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

    this->animResource = InvalidAnimationId;
    Memory::Free(Memory::ResourceHeap, this->samples);
    Memory::Free(Memory::ResourceHeap, this->sampleCounts);
    this->samples = nullptr;
    this->sampleCounts = nullptr;
}

} // namespace CoreAnimation
