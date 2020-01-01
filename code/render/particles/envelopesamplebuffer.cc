//------------------------------------------------------------------------------
//  envelopesamplebuffer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "particles/envelopesamplebuffer.h"

namespace Particles
{

//------------------------------------------------------------------------------
/**
*/
EnvelopeSampleBuffer::EnvelopeSampleBuffer() :
    numSamples(0),
    buffer(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
EnvelopeSampleBuffer::~EnvelopeSampleBuffer()
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
EnvelopeSampleBuffer::Setup(const EmitterAttrs& emitterAttrs, SizeT numSamp)
{
    n_assert(!this->IsValid());
    this->numSamples = numSamp;

    // allocate the sample buffer
    SizeT size = this->numSamples * EmitterAttrs::NumEnvelopeAttrs * sizeof(float);   
    this->buffer = (float*) Memory::Alloc(Memory::ResourceHeap, size);

    // pre-sample envelope curves into sample buffer
    IndexT i;
    for (i = 0; i < EmitterAttrs::NumEnvelopeAttrs; i++)
    {
        float* samplePtr = this->buffer + i;
        emitterAttrs.GetEnvelope((EmitterAttrs::EnvelopeAttr)i).PreSample(samplePtr, this->numSamples, EmitterAttrs::NumEnvelopeAttrs);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EnvelopeSampleBuffer::Discard()
{
    n_assert(this->IsValid());
    n_assert(0 != this->buffer);

    Memory::Free(Memory::ResourceHeap, this->buffer);
    this->buffer = 0;
    this->numSamples = 0;
}

} // namespace Particles