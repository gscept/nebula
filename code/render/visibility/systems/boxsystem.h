#pragma once
//------------------------------------------------------------------------------
/**
	Box system

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "visibilitysystem.h"
#include "jobs/jobs.h"
namespace Visibility
{

extern void BoxSystemJobFunc(const Jobs::JobFuncContext& ctx);

class BoxSystem : public VisibilitySystem
{
public:
private:
	friend class ObserverContext;

	/// setup from load info
	void Setup(const BoxSystemLoadInfo& info);
};

} // namespace Visibility
