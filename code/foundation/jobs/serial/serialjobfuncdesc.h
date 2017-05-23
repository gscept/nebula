#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::SerialJobFuncDesc
  
    A function pointer descriptor for the dummy job system. This is compatibel
    with the JobFuncDesc from the TP (thread-pool) job system.
    
    (C) 2009 Radon Labs GmbH
*/
#include "jobs/base/jobfuncdescbase.h"
#include "jobs/jobfunccontext.h"

//------------------------------------------------------------------------------
namespace Jobs
{
class SerialJobFuncDesc : public Base::JobFuncDescBase
{
public:
    /// callback function typedef
    typedef void (*FuncPtr)(const JobFuncContext& ctx);

    /// default constructor
    SerialJobFuncDesc();
    /// constructor
    SerialJobFuncDesc(FuncPtr funcPtr);
    /// get function pointer
    FuncPtr GetFunctionPointer() const;

private:
    FuncPtr funcPtr;
};

//------------------------------------------------------------------------------
/**
*/
inline
SerialJobFuncDesc::SerialJobFuncDesc() :
    funcPtr(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
SerialJobFuncDesc::SerialJobFuncDesc(FuncPtr funcPtr_) :
    funcPtr(funcPtr_)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline SerialJobFuncDesc::FuncPtr
SerialJobFuncDesc::GetFunctionPointer() const
{
    return this->funcPtr;
}

} // namespace Jobs
//------------------------------------------------------------------------------
