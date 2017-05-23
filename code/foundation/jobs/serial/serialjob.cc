//------------------------------------------------------------------------------
//  serialjob.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/serial/serialjob.h"

namespace Jobs
{
__ImplementClass(Jobs::SerialJob, 'SLJB', Base::JobBase);

//------------------------------------------------------------------------------
/**
*/
SerialJob::SerialJob()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
SerialJob::~SerialJob()
{
    // empty
}    

} // namespace Jobs