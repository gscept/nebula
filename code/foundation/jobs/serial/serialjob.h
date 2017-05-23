#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::SerialJob
  
    A job in the serial job system (dummy job system which doesn't parallelize
    jobs).
        
    (C) 2009 Radon Labs GmbH
*/    
#include "jobs/base/jobbase.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class SerialJob : public Base::JobBase
{
    __DeclareClass(SerialJob);
public:
    /// constructor
    SerialJob();
    /// destructor
    virtual ~SerialJob();
};

} // namespace Jobs
//------------------------------------------------------------------------------

