#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimKeyBuffer
    
    A simple buffer of vec4 animation keys.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimKeyBuffer : public Core::RefCounted
{
    __DeclareClass(AnimKeyBuffer);
public:
    struct Interval
    {
        Timing::Tick start, end;
        uint key0, key1;
    };

    /// constructor
    AnimKeyBuffer();
    /// destructor
    virtual ~AnimKeyBuffer();
    /// setup the buffer
    void Setup(SizeT numIntervals, SizeT numKeys, void* intervalPtr, void* keyPtr);
    /// discard the buffer
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    /// get number of keys in buffer
    SizeT GetNumKeys() const;
    /// get buffer size in bytes
    SizeT GetByteSize() const;
    /// Get direct pointer to keys
    const float* GetKeyBufferPointer() const;
    /// get direct pointer to interval buffer
    const AnimKeyBuffer::Interval* GetIntervalBufferPointer() const;

private:
    SizeT numKeys;
    SizeT numIntervals;
    uint mapCount;
    float* keyBuffer;
    AnimKeyBuffer::Interval* intervalBuffer;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimKeyBuffer::IsValid() const
{
    return (0 != this->intervalBuffer);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimKeyBuffer::GetNumKeys() const
{
    return this->numKeys;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimKeyBuffer::GetByteSize() const
{
    return this->numKeys * sizeof(float);
}

//------------------------------------------------------------------------------
/**
*/
inline const float*
AnimKeyBuffer::GetKeyBufferPointer() const
{
    return this->keyBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline const AnimKeyBuffer::Interval*
AnimKeyBuffer::GetIntervalBufferPointer() const
{
    return this->intervalBuffer;
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------

