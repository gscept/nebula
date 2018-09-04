#pragma once
//------------------------------------------------------------------------------
/**
	Octree system

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "system.h"
#include "jobs/jobs.h"
namespace Visibility
{

extern void OctreeSystemJobFunc(const Jobs::JobFuncContext& ctx);

class OctreeSystem : public System
{
public:
private:
	friend class ObserverContext;

	/// setup from load info
	void Setup(const OctreeSystemLoadInfo& info);
};

} // namespace Visibility

