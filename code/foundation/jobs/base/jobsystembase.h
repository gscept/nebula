#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobServerBase
    
    The JobSystem singleton is used to setup and shutdown the
    Jobs subsystem.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace Base
{
class JobSystemBase : public Core::RefCounted
{
    __DeclareClass(JobSystemBase);
public:
    /// constructor
    JobSystemBase();
    /// destructor
    virtual ~JobSystemBase();

    /// setup the job system
    void Setup();
    /// shutdown the job system
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;

protected:
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
JobSystemBase::IsValid() const
{
    return this->isValid;
}

} // namespace Jobs
//------------------------------------------------------------------------------

    