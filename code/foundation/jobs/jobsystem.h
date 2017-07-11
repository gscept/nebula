#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobSystem
    
    Initializes the N3 job system.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __PS3__
#include "jobs/ps3/ps3jobsystem.h"
namespace Jobs
{
class JobSystem : public PS3::PS3JobSystem
{
    __DeclareClass(JobSystem);
    __DeclareInterfaceSingleton(JobSystem);
public:
    /// constructor
    JobSystem();
    /// destructor
    virtual ~JobSystem();
};
} // namespace Jobs
#elif (NEBULA3_USE_SERIAL_JOBSYSTEM || __WII__)
#include "jobs/serial/serialjobsystem.h"
namespace Jobs
{
class JobSystem : public Jobs::SerialJobSystem
{
    __DeclareClass(JobSystem);
    __DeclareInterfaceSingleton(JobSystem);
public:
    /// constructor
    JobSystem();
    /// destructor
    virtual ~JobSystem();
};
} // namespace Jobs
#elif (__WIN32__ || __XBOX360__ || __WII__ || __linux__)
#include "jobs/tp/tpjobsystem.h"
namespace Jobs
{
class JobSystem : public Jobs::TPJobSystem
{
    __DeclareClass(JobSystem);
    __DeclareInterfaceSingleton(JobSystem);
public:
    /// constructor
    JobSystem();
    /// destructor
    virtual ~JobSystem();
};
} // namespace Jobs
#else
#error "Job::JobSystem not implemented on this platform!"
#endif    