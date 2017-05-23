#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::TPJobFuncDesc
  
    A function descriptor for the thread-pool job system.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "jobs/base/jobfuncdescbase.h"
#include "jobs/jobfunccontext.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class TPJobFuncDesc : public Base::JobFuncDescBase
{
public:
    /// callback function typedef
    typedef void (*FuncPtr)(const JobFuncContext& ctx);

    /// default constructor
    TPJobFuncDesc();
    /// constructor
    TPJobFuncDesc(FuncPtr funcPtr);
    /// get function pointer
    FuncPtr GetFunctionPointer() const;

private:
    FuncPtr funcPtr;
};

//------------------------------------------------------------------------------
/**
*/
inline
TPJobFuncDesc::TPJobFuncDesc() :
    funcPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
TPJobFuncDesc::TPJobFuncDesc(FuncPtr funcPtr_) :
    funcPtr(funcPtr_)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline TPJobFuncDesc::FuncPtr
TPJobFuncDesc::GetFunctionPointer() const
{
    return this->funcPtr;
}

} // namespace Jobs
//------------------------------------------------------------------------------
