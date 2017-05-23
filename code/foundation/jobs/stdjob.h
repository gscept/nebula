#pragma once
//------------------------------------------------------------------------------
/**
    @file jobs/stdjob.h
    
    Standard header for job functions.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#define __NEBULA3_JOB__ (1)
#include "core/config.h"
#if __WIN32__
#include "core/win32/precompiled.h"
#include "jobs/jobfunccontext.h"
#elif __XBOX360__
#include "core/xbox360/precompiled.h"
#include "jobs/jobfunccontext.h"
#elif linux
#include "core/posix/precompiled.h"
#include "jobs/jobfunccontext.h"
#else
#error "Job functions not supported on this platform!"
#endif    

#if !__PS3__
#define __ImplementSpursJob(func)
#endif


