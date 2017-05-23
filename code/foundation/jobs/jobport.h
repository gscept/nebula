#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobPort
    
    A job port of the N3 job system (see JobPortBase for details).
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __PS3__
#include "jobs/ps3/ps3jobport.h"
namespace Jobs
{
class JobPort : public PS3::PS3JobPort
{
    __DeclareClass(JobPort);
};
} // namespace Jobs
#elif (NEBULA3_USE_SERIAL_JOBSYSTEM || __WII__)
#include "jobs/serial/serialjobport.h"
namespace Jobs
{
class JobPort : public SerialJobPort
{
    __DeclareClass(JobPort);
};
} // namespace Jobs
#elif (__WIN32__ || __XBOX360__ || linux)
#include "jobs/tp/tpjobport.h"
namespace Jobs
{
class JobPort : public TPJobPort
{
    __DeclareClass(JobPort);
};
} // namespace Jobs
#else
#error "Jobs::JobPort not implemented on this platform!"
#endif
