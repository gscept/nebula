#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimSampleBuffer
  
    Stores the result of an animation sampling operation, stores 
    samples key values and sample-counts which keep track of the number 
    of samples which contributed to a mixing result (this is necessary
    for correct mixing of partial animations).
     
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "animresource.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimSampleBuffer
{
public:
    /// constructor
    AnimSampleBuffer();
    /// destructor
    virtual ~AnimSampleBuffer();

    /// move operator
    AnimSampleBuffer(AnimSampleBuffer&& rhs);
    /// copy operator
    AnimSampleBuffer(const AnimSampleBuffer& rhs);
    /// assign move operator
    void operator=(AnimSampleBuffer&& rhs);
    /// assign move operator
    void operator=(const AnimSampleBuffer& rhs);
    
    /// setup the object from an animation resource
    void Setup(const AnimResourceId& animResource);
    /// discard the object
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    
    /// get the number of samples in the buffer
    SizeT GetNumSamples() const;
    
    /// get direct pointer to samples
    Math::vec4* GetSamplesPointer() const;
    /// get direct pointer to sample counts
    uchar* GetSampleCountsPointer() const;

private:
    AnimResourceId animResource;
    SizeT numSamples;
    Math::vec4* samples;
    uchar* sampleCounts;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimSampleBuffer::IsValid() const
{
    return this->animResource != AnimResourceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimSampleBuffer::GetNumSamples() const
{
    return this->numSamples;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4*
AnimSampleBuffer::GetSamplesPointer() const
{
    return this->samples;
}

//------------------------------------------------------------------------------
/**
*/
inline uchar*
AnimSampleBuffer::GetSampleCountsPointer() const
{
    return this->sampleCounts;
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------
