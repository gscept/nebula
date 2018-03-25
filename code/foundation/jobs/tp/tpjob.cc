//------------------------------------------------------------------------------
//  tpjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/tp/tpjob.h"
#include "jobs/tp/tpjobthreadpool.h"
#include "jobs/jobsystem.h"
#include "math/scalar.h"
#include "threading/interlocked.h"

namespace Jobs
{
__ImplementClass(Jobs::TPJob, 'TJOB', Base::JobBase);

using namespace Base;
using namespace Threading;

//------------------------------------------------------------------------------
/**
*/
TPJob::TPJob() :
    completionCounter(0),
    completionEvent(true)  // configure as manual reset event
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TPJob::~TPJob()
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
TPJob::Setup(const JobUniformDesc& uniform, const JobDataDesc& input, const JobDataDesc& output, const JobFuncDesc& func)
{
    n_assert(!this->IsValid());
    JobBase::Setup(uniform, input, output, func);
    this->completionCounter = 0;

    // setup job slice array
    n_assert(input.GetNumBuffers() > 0);
    n_assert(output.GetNumBuffers() > 0);
    SizeT numInputSlices = (input.GetBufferSize(0) + (input.GetSliceSize(0) - 1)) / input.GetSliceSize(0);
    SizeT numOutputSlices = (output.GetBufferSize(0) + (output.GetSliceSize(0) - 1)) / output.GetSliceSize(0);
    n_assert(numInputSlices == numOutputSlices);
    this->jobSlices.SetSize(numInputSlices);
    IndexT i;
    for (i = 0; i < numInputSlices; i++)
    {
        this->jobSlices[i].Setup(this, i);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TPJob::Discard()
{
    n_assert(this->IsValid());
    this->jobSlices.SetSize(0);
    JobBase::Discard();
}

} // namespace Jobs