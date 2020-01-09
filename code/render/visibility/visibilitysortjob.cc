//------------------------------------------------------------------------------
//  visibilitysortjob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
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
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
	ObserverContext::VisibilityDrawList* buckets = (ObserverContext::VisibilityDrawList*)ctx.outputs[0];
	Memory::ArenaAllocator<1024>* packetAllocator = (Memory::ArenaAllocator<1024>*)ctx.uniforms[0];

	bool* results = (bool*)ctx.inputs[0];
	Graphics::ContextEntityId* entities = (Graphics::ContextEntityId*)ctx.inputs[1];

	// begin adding buckets
	buckets->BeginBulkAdd();

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
				Models::ShaderStateNode::Instance* const shdNodeInst = reinterpret_cast<Models::ShaderStateNode::Instance*>(inst);
				Models::ShaderStateNode* const shdNode = reinterpret_cast<Models::ShaderStateNode*>(inst->node);
				auto& bucket = buckets->AddUnique(shdNode->materialType);
				if (!bucket.IsBulkAdd())
					bucket.BeginBulkAdd();

				// add an array if non existant, or return reference to one if it exists
				auto& draw = bucket.AddUnique(inst->node);

				// allocate memory for draw packet
				void* mem = packetAllocator->Alloc(shdNodeInst->GetDrawPacketSize());

				// update packet and add to list
				Models::ModelNode::DrawPacket* packet = shdNodeInst->UpdateDrawPacket(mem);
				draw.Append(packet);
			}
		}
	}

	// end adding inner buckets
	auto it = buckets->Begin();
	while (it != buckets->End())
	{
		if (it.val->IsBulkAdd())
			it.val->EndBulkAdd();
		it++;
	}

	// end adding buckets
	buckets->EndBulkAdd();
}

} // namespace Visibility
