#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::EnvelopeSampleBuffer
    
    A lookup table for pre-sampled envelope curves.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "particles/envelopecurve.h"
#include "particles/emitterattrs.h"

//------------------------------------------------------------------------------
namespace Particles
{
class EnvelopeSampleBuffer
{
public:
    /// constructor
    EnvelopeSampleBuffer();
    /// destructor
    ~EnvelopeSampleBuffer();
    
    /// setup the sample buffer
    void Setup(const EmitterAttrs& emitterAttrs, SizeT numSamples);
    /// discard the sample buffer
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;

    /// convert t-value (0.0 to 1.0) into a lookup index
    IndexT AsSampleIndex(float t) const;
    /// get pointer to samples, index into array by EmitterAttrs::EnvelopeAttr
    float* LookupSamples(IndexT sampleIndex) const;
    /// get the number of samples per attribute
    SizeT GetNumSamples() const;
    /// get the sample buffer
    const float* GetSampleBuffer() const;

private:
    SizeT numSamples;
    float* buffer;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
EnvelopeSampleBuffer::IsValid() const
{
    return (0 != this->buffer);
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
EnvelopeSampleBuffer::AsSampleIndex(float t) const
{
    IndexT index = IndexT(t / this->numSamples);

    // make sure index is in valid range
    index = Math::n_iclamp(index, 0, this->numSamples - 1);
    return index;
}

//------------------------------------------------------------------------------
/**
*/
inline float*
EnvelopeSampleBuffer::LookupSamples(IndexT sampleIndex) const
{
    #if NEBULA_BOUNDSCHECKS
    n_assert((sampleIndex >= 0) && (sampleIndex < this->numSamples));
    n_assert(0 != buffer);
    #endif
    return this->buffer + (sampleIndex * EmitterAttrs::NumEnvelopeAttrs);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT 
EnvelopeSampleBuffer::GetNumSamples() const 
{ 
    return this->numSamples; 
}

//------------------------------------------------------------------------------
/**
*/
inline const float* 
EnvelopeSampleBuffer::GetSampleBuffer() const 
{ 
    return this->buffer; 
}

} // namespace Particles
//------------------------------------------------------------------------------
    