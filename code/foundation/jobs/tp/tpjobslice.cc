//------------------------------------------------------------------------------
//  tpjobslice.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/tp/tpjobslice.h"
#include "jobs/tp/tpjob.h"

namespace Jobs
{

//------------------------------------------------------------------------------
/**
*/
TPJobSlice::TPJobSlice() :
    job(0),
    sliceIndex(InvalidIndex)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TPJobSlice::~TPJobSlice()
{
    this->job = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
TPJobSlice::Setup(TPJob* job_, IndexT sliceIndex_)
{
    n_assert(0 == this->job);
    n_assert(0 != job_);
    this->job = job_;
    this->sliceIndex = sliceIndex_;
}

} // namespace Jobs
