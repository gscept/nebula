#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::JobBase
  
    Job objects are asynchronous data crunchers. 
    
    A Job is setup from the following components:

    - an input data buffer
    - an output data buffer
    - an uniform data buffer
    - the job function

    The job function will perform parallel processing of input data into
    the output buffer. The input data will be split into independent slices
    and slices will be processed in parallel. The uniform data is identical
    for every slice and can be used to pass additional arguments to the 
    job function.

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "jobs/jobuniformdesc.h"
#include "jobs/jobdatadesc.h"
#include "jobs/jobfuncdesc.h"

//------------------------------------------------------------------------------
namespace Base
{
class JobBase : public Core::RefCounted
{
    __DeclareClass(JobBase);
public:
    /// max size of a data slice is 16 kByte - 1 byte
    static const SizeT MaxSliceSize = JobMaxSliceSize;

    /// constructor
    JobBase();
    /// destructor
    virtual ~JobBase();

    /// allocate a single memory buffer associated with the job, may only be called once, and before Setup()!
    void* AllocPrivateBuffer(Memory::HeapType heapType, SizeT size);
    /// get pointer to job's optional private buffer
    void* GetPrivateBuffer() const;
    /// get size of job's optional private buffer
    SizeT GetPrivateBufferSize() const;

    /// setup the job
    void Setup(const Jobs::JobUniformDesc& uniformDesc, const Jobs::JobDataDesc& inputDesc, const Jobs::JobDataDesc& outputDesc, const Jobs::JobFuncDesc& funcDesc);
    /// discard the job
    void Discard();
    /// return true if job has been setup
    bool IsValid() const;

    /// patch input pointers after job has been setup
    void PatchInputDesc(const Jobs::JobDataDesc& inputDesc);
    /// patch output pointers after job has been setup
    void PatchOutputDesc(const Jobs::JobDataDesc& outputDesc);
    /// patch uniform pointer after job has been setup
    void PatchUniformDesc(const Jobs::JobUniformDesc& uniformDesc);

    /// get uniform data descriptor
    const Jobs::JobUniformDesc& GetUniformDesc() const;
    /// get input data descriptor
    const Jobs::JobDataDesc& GetInputDesc() const;
    /// get output data descriptor
    const Jobs::JobDataDesc& GetOutputDesc() const;
    /// get function descriptor
    const Jobs::JobFuncDesc& GetFuncDesc() const;

protected:
    Jobs::JobUniformDesc uniformDesc;
    Jobs::JobDataDesc inputDesc;
    Jobs::JobDataDesc outputDesc;
    Jobs::JobFuncDesc funcDesc;
    Memory::HeapType privateBufferHeapType;
    SizeT privateBufferSize;
    void* privateBuffer;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
JobBase::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline const Jobs::JobUniformDesc&
JobBase::GetUniformDesc() const
{
    return this->uniformDesc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Jobs::JobDataDesc&
JobBase::GetInputDesc() const
{
    return this->inputDesc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Jobs::JobDataDesc&
JobBase::GetOutputDesc() const
{
    return this->outputDesc;
}

//------------------------------------------------------------------------------
/**
*/
inline const Jobs::JobFuncDesc&
JobBase::GetFuncDesc() const
{
    return this->funcDesc;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
JobBase::GetPrivateBuffer() const
{
    return this->privateBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
JobBase::GetPrivateBufferSize() const
{
    return this->privateBufferSize;
}

} // namespace Jobs
//------------------------------------------------------------------------------

