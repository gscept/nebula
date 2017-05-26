#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimKeyBuffer
    
    A simple buffer of float4 animation keys.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimKeyBuffer : public Core::RefCounted
{
    __DeclareClass(AnimKeyBuffer);
public:
    /// constructor
    AnimKeyBuffer();
    /// destructor
    virtual ~AnimKeyBuffer();
    /// setup the buffer
    void Setup(SizeT numKeys);
    /// discard the buffer
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;
    /// get number of keys in buffer
    SizeT GetNumKeys() const;
    /// get buffer size in bytes
    SizeT GetByteSize() const;
    /// (obsolete) map key buffer for CPU access
    void* Map();
    /// (obsolete) unmap the resource
    void Unmap();
    /// return true if the key buffer is currently mapped
    bool IsMapped() const;
    /// get direct pointer to key buffer
    Math::float4* GetKeyBufferPointer() const;

private:
    SizeT numKeys;
    uint mapCount;
    void* keyBuffer;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimKeyBuffer::IsValid() const
{
    return (0 != this->keyBuffer);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimKeyBuffer::IsMapped() const
{
    return (0 != this->mapCount);
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
    return this->numKeys * sizeof(Math::float4);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::float4*
AnimKeyBuffer::GetKeyBufferPointer() const
{
    return (Math::float4*) this->keyBuffer;
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------

    