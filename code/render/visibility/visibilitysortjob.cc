//------------------------------------------------------------------------------
//  boxsystemjob.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "visibilitycontext.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
	ObserverContext::VisibilityBuckets* buckets = (ObserverContext::VisibilityBuckets*)ctx.outputs[0];

	bool* results = (bool*)ctx.inputs[0];
	Graphics::ContextEntityId* entities = (Graphics::ContextEntityId*)ctx.inputs[1];
}

} // namespace Visibility
