#pragma once
//------------------------------------------------------------------------------
/**
	Portal system

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "system.h"
#include "jobs/jobs.h"
namespace Visibility
{
	
extern void PortalSystemJobFunc(const Jobs::JobFuncContext& ctx);

class PortalSystem : public System
{
public:
private:
	friend class ObserverContext;

	/// setup from load info
	void Setup(const PortalSystemLoadInfo& info);
};
} // namespace Visibility
