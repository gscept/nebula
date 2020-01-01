#pragma once
//------------------------------------------------------------------------------
/**
	Portal system

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "visibilitysystem.h"
#include "jobs/jobs.h"
namespace Visibility
{
	
extern void PortalSystemJobFunc(const Jobs::JobFuncContext& ctx);

class PortalSystem : public VisibilitySystem
{
public:
private:
	friend class ObserverContext;

	/// setup from load info
	void Setup(const PortalSystemLoadInfo& info);
};
} // namespace Visibility
