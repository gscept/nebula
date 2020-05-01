//------------------------------------------------------------------------------
//  visibilitysortjob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "visibilitycontext.h"
#include "models/modelcontext.h"
#include "models/nodes/shaderstatenode.h"
#include "profiling/profiling.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilitySortJob(const Jobs::JobFuncContext& ctx)
{
	N_SCOPE(VisibilitySortJob, Visibility);
	Memory::ArenaAllocator<1024>* packetAllocator = (Memory::ArenaAllocator<1024>*)ctx.uniforms[0];
	packetAllocator->Release();

	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{ 
		Math::ClipStatus::Type* results = (Math::ClipStatus::Type*)N_JOB_INPUT(ctx, sliceIdx, 0);
		Models::ModelNode::Instance** const nodes = (Models::ModelNode::Instance**)N_JOB_INPUT(ctx, sliceIdx, 1);
		ObserverContext::VisibilityDrawList* buckets = (ObserverContext::VisibilityDrawList*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

		// begin adding buckets
		buckets->BeginBulkAdd();

		// calculate amount of models
		uint32 numNodes = ctx.inputSizes[0] / sizeof(Math::ClipStatus::Type);
		uint32 i;
		for (i = 0; i < numNodes; i++)
		{
			Models::ModelNode::Instance* const inst = nodes[i];
			Models::NodeBits bits = inst->node->GetBits();

			// only treat renderable nodes
			if ((bits & Models::HasStateBit) == Models::HasStateBit)
			{
				if (results[i] == Math::ClipStatus::Outside)
					continue;

				Models::ShaderStateNode::Instance* const shdNodeInst = reinterpret_cast<Models::ShaderStateNode::Instance*>(inst);
				if (!shdNodeInst->active)
					continue;

				Models::ShaderStateNode* const shdNode = reinterpret_cast<Models::ShaderStateNode*>(inst->node);
				n_assert(bits == shdNode->GetBits());

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
}

} // namespace Visibility
