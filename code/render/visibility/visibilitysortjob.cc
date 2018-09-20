//------------------------------------------------------------------------------
//  boxsystemjob.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
	This is essentially a single-slice job, because the bucket will be unique per observer
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
	ObserverContext::VisibilityBuckets* buckets = (ObserverContext::VisibilityBuckets*)ctx.outputs[0];

	bool* results = (bool*)ctx.inputs[0];
	Graphics::ContextEntityId* entities = (Graphics::ContextEntityId*)ctx.inputs[1];

	// calculate amount of models
	uint32 numModels = ctx.inputSizes[0] / sizeof(bool);
	uint32 i;
	for (i = 0; i < numModels; i++)
	{	
		// get model instance
		const Models::ModelInstanceId model = Models::ModelContext::GetModel(entities[i]);
		const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(entities[i]);
		bool result = results[i];
	}
}

} // namespace Visibility
