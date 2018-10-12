#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimSampleBuffer
  
    Stores the result of an animation sampling operation, stores 
    samples key values and sample-counts which keep track of the number 
    of samples which contributed to a mixing result (this is necessary
    for correct mixing of partial animations).
     
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "streamanimationpool.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimSampleBuffer : public Core::RefCounted
{
    __DeclareClass(AnimSampleBuffer);
public:
    /// constructor
    AnimSampleBuffer();
    /// destructor
    virtual ~AnimSampleBuffer();
    
    /// setup the object from an animation resource
    void Setup(const AnimResourceId& animResource);
    /// discard the object
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    
    /// get the number of samples in the buffer
    SizeT GetNumSamples() const;
    /// (obsolete) gain read/write access to sample buffer
    Math::float4* MapSamples();
    /// (obsolete) give up access to sample buffer
    void UnmapSamples();
    /// (obsolete) gain read/write access to sample counts
    uchar* MapSampleCounts();
    /// (obsolete) give up access to sample counts
    void UnmapSampleCounts();
    
    /// get direct pointer to samples
    Math::float4* GetSamplesPointer() const;
    /// get direct pointer to sample counts
    uchar* GetSampleCountsPointer() const;

private:
    AnimResourceId animResource;
    SizeT numSamples;
    Math::float4* samples;
    uchar* sampleCounts;
    bool samplesMapped;
    bool sampleCountsMapped;
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
inline Math::float4*
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
