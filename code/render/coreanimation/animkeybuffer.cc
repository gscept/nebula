//------------------------------------------------------------------------------
//  animkeybuffer.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animkeybuffer.h"

namespace CoreAnimation
{
__ImplementClass(CoreAnimation::AnimKeyBuffer, 'ANKB', Core::RefCounted);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimKeyBuffer::AnimKeyBuffer()
    : numKeys(0)
    , numIntervals(0)
    , mapCount(0)
    , keyBuffer(nullptr)
    , intervalBuffer(nullptr)
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
AnimKeyBuffer::Setup(SizeT numIntervals, SizeT numKeys, void* intervalPtr, void* keyPtr)
{
    n_assert(!this->IsValid());
    this->numIntervals = numIntervals;
    this->numKeys = numKeys;
    this->mapCount = 0;
    this->keyBuffer = (float*)Memory::Alloc(Memory::ResourceHeap, sizeof(float) * this->numKeys);
    Memory::Copy(keyPtr, this->keyBuffer, this->GetByteSize());
    this->intervalBuffer = (AnimKeyBuffer::Interval*)Memory::Alloc(Memory::ResourceHeap, sizeof(AnimKeyBuffer::Interval) * this->numIntervals);
    Memory::Copy(intervalPtr, this->intervalBuffer, sizeof(AnimKeyBuffer::Interval) * this->numIntervals);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimKeyBuffer::Discard()
{
    n_assert(this->IsValid());
    Memory::Free(Memory::ResourceHeap, this->keyBuffer);
    this->keyBuffer = 0;
    Memory::Free(Memory::ResourceHeap, this->intervalBuffer);
    this->intervalBuffer = 0;
    this->numKeys = 0;
}

} // namespace CoreAnimation
