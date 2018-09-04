#pragma once
//------------------------------------------------------------------------------
/**
	Quadtree system

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "system.h"
#include "jobs/jobs.h"
namespace Visibility
{
	
extern void QuadtreeInjectFunction(const Jobs::JobFuncContext& ctx);
extern void QuadtreeResolveFunction(const Jobs::JobFuncContext& ctx);

class QuadtreeSystem : public System
{
public:
private:
	friend class ObserverContext;

	/// setup from load info
	void Setup(const QuadtreeSystemLoadInfo& info);
};

} // namespace Visibility
