#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::JobFuncDescBase
    
    Platform-specific description of a Job function. This can be a simple
    pointer to a function if the job system works with simple CPU threads,
    or anything else.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Base
{
class JobFuncDescBase
{
    // empty, must be overriden in subclass
};    

};
//------------------------------------------------------------------------------
    