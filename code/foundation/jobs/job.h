#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::Job
    
    A Job in the Nebula3 job system.  
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/ 
#include "core/config.h"
#if __PS3__
#include "jobs/ps3/ps3job.h"
namespace Jobs
{
class Job : public PS3::PS3Job
{
    __DeclareClass(Job);
};
} // namespace Jobs
#elif (NEBULA3_USE_SERIAL_JOBSYSTEM || __WII__)
#include "jobs/serial/serialjob.h"
namespace Jobs
{
class Job : public SerialJob
{
    __DeclareClass(Job);
};
} // namespace Jobs
#elif (__WIN32__ || __XBOX360__ || __linux__)
#include "jobs/tp/tpjob.h"
namespace Jobs
{
class Job : public TPJob
{ 
    __DeclareClass(Job);
};
} // namespace Jobs
#else
#error "Jobs::Job not implemented on this platform!"
#endif
