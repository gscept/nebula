//------------------------------------------------------------------------------
//  boxsystemjob.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
#include "models/nodes/shaderstatenode.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
	This is essentially a single-slice job, because the bucket will be unique per observer
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
	ObserverContext::VisibilityDrawList* buckets = (ObserverContext::VisibilityDrawList*)ctx.outputs[0];

	bool* results = (bool*)ctx.inputs[0];
	Graphics::ContextEntityId* entities = (Graphics::ContextEntityId*)ctx.inputs[1];

	// calculate amount of models
	uint32 numModels = ctx.inputSizes[0] / sizeof(bool);
	uint32 i;
	for (i = 0; i < numModels; i++)
	{	
		// get model instance
		const Models::ModelId model = Models::ModelContext::GetModel(entities[i]);
		const Util::Array<Models::ModelNode::Instance*>& nodes = Models::ModelContext::GetModelNodeInstances(entities[i]);
		const Util::Array<Models::NodeType>& types = Models::ModelContext::GetModelNodeTypes(entities[i]);
		bool result = results[i];

		IndexT j;
		if (result) for (j = 0; j < nodes.Size(); j++)
		{
			Models::ModelNode::Instance* const inst = nodes[j];

			// only treat renderable nodes
			if (types[j] >= Models::NodeHasShaderState)
			{
				Models::ShaderStateNode::Instance* const shdNodeInst = static_cast<Models::ShaderStateNode::Instance*>(inst);
				Models::ShaderStateNode* const shdNode = static_cast<Models::ShaderStateNode*>(inst->node);
				auto& bucket = buckets->AddUnique(shdNode->materialType);
				auto& draw = bucket.AddUnique(inst->node);
				draw.Append(inst);
			}
		}
	}
}

} // namespace Visibility
