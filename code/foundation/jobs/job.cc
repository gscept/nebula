//------------------------------------------------------------------------------
//  job.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobs/job.h"

namespace Jobs
{
#if __PS3__
__ImplementClass(Jobs::Job, 'JOB_', PS3::PS3Job);
#elif (NEBULA3_USE_SERIAL_JOBSYSTEM || __WII__)
__ImplementClass(Jobs::Job, 'JOB_', Jobs::SerialJob);
#elif (__WIN32__ || __XBOX360__ || linux)
__ImplementClass(Jobs::Job, 'JOB_', Jobs::TPJob);
#else
#error "Jobs::Job not implemented on this platform!"
#endif
}
