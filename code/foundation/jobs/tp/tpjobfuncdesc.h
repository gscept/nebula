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
#include <functional>

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
    /// constructor from function pointer
    TPJobFuncDesc(FuncPtr funcPtr);
	/// constructor from lambda function
	TPJobFuncDesc(const std::function<void(const JobFuncContext&)>& func);
    /// get function pointer
    const std::function<void(const JobFuncContext&)>& GetFunctionPointer() const;

private:
	std::function<void(const JobFuncContext&)> func;
};

//------------------------------------------------------------------------------
/**
*/
inline
TPJobFuncDesc::TPJobFuncDesc() :
	func(nullptr)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
TPJobFuncDesc::TPJobFuncDesc(FuncPtr funcPtr_) :
	func(funcPtr_)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline 
TPJobFuncDesc::TPJobFuncDesc(const std::function<void(const JobFuncContext&)>& func) :
	func(func)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline const std::function<void(const JobFuncContext&)>&
TPJobFuncDesc::GetFunctionPointer() const
{
    return this->func;
}

} // namespace Jobs
//------------------------------------------------------------------------------
