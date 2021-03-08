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
VisibilityDrawListUpdateJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(VisibilityDrawListUpdateJob, Visibility);

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    { 
        ObserverContext::VisibilityDrawList* drawList = (ObserverContext::VisibilityDrawList*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

        for (auto& packet : drawList->drawPackets)
        {
            Models::ModelNode::Instance* nodeInst = packet->ToNode<Models::ModelNode::Instance>();
            nodeInst->Update();
        }
    }
}

} // namespace Visibility
