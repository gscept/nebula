//------------------------------------------------------------------------------
//  jobbase.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/base/jobbase.h"

namespace Base
{
__ImplementClass(Base::JobBase, 'JBBS', Core::RefCounted);
    
using namespace Jobs;

//------------------------------------------------------------------------------
/**
*/
JobBase::JobBase() :
    privateBufferHeapType(Memory::InvalidHeapType),
    privateBufferSize(0),
    privateBuffer(0),
    isValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JobBase::~JobBase()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    if (0 != this->privateBuffer)
    {
        Memory::Free(this->privateBufferHeapType, this->privateBuffer);
        this->privateBuffer = 0;
    }
}

//------------------------------------------------------------------------------
/**
    This method can be used to allocate a single memory buffer, owned by
    the job object. The method must be called before Setup(), and will
    remain valid until the destructor of the job object is call
    (so it will survive a Discard()!).
*/
void*
JobBase::AllocPrivateBuffer(Memory::HeapType heapType, SizeT size)
{
    n_assert(0 == this->privateBuffer);
    this->privateBuffer = Memory::Alloc(heapType, size);
    this->privateBufferHeapType = heapType;
    this->privateBufferSize = size;
    return this->privateBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void
JobBase::Setup(const JobUniformDesc& uniform, const JobDataDesc& input, const JobDataDesc& output, const Jobs::JobFuncDesc& func)
{
    n_assert(!this->IsValid());
    this->uniformDesc = uniform;
    this->inputDesc = input;
    this->outputDesc = output;
    this->funcDesc = func;
    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
*/
void
JobBase::PatchInputDesc(const JobDataDesc& input)
{
    this->inputDesc = input;
}

//------------------------------------------------------------------------------
/**
*/
void
JobBase::PatchOutputDesc(const JobDataDesc& output)
{
    this->outputDesc = output;
}

//------------------------------------------------------------------------------
/**
*/
void
JobBase::PatchUniformDesc(const JobUniformDesc& uniform)
{
    this->uniformDesc = uniform;
}

//------------------------------------------------------------------------------
/**
*/
void
JobBase::Discard()
{
    n_assert(this->IsValid());
    this->isValid = false;
}

} // namespace Base