//------------------------------------------------------------------------------
//  animkeybuffer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animkeybuffer.h"
#include "math/float4.h"

namespace CoreAnimation
{
__ImplementClass(CoreAnimation::AnimKeyBuffer, 'ANKB', Core::RefCounted);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimKeyBuffer::AnimKeyBuffer() :
    numKeys(0),
    mapCount(0),
    keyBuffer(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimKeyBuffer::~AnimKeyBuffer()
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
AnimKeyBuffer::Setup(SizeT numKeys_)
{
    n_assert(!this->IsValid());
    n_assert(!this->IsMapped());
    this->numKeys = numKeys_;
    this->mapCount = 0;
    this->keyBuffer = Memory::Alloc(Memory::ResourceHeap, this->GetByteSize());
}

//------------------------------------------------------------------------------
/**
*/
void
AnimKeyBuffer::Discard()
{
    n_assert(this->IsValid());
    n_assert(!this->IsMapped());
    Memory::Free(Memory::ResourceHeap, this->keyBuffer);
    this->keyBuffer = 0;
    this->numKeys = 0;
}

//------------------------------------------------------------------------------
/**
*/
void*
AnimKeyBuffer::Map()
{
    n_assert(this->IsValid());
    this->mapCount++;
    return this->keyBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimKeyBuffer::Unmap()
{
    n_assert(this->IsValid());
    n_assert(this->IsMapped());
    this->mapCount--;
}

} // namespace CoreAnimation
