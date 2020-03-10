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
		ObserverContext::VisibilityDrawList* buckets = (ObserverContext::VisibilityDrawList*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

		// clear draw lists
		auto it1 = buckets->Begin();
		while (it1 != buckets->End())
		{
			auto it2 = it1.val->Begin();
			while (it2 != it1.val->End())
			{
				
				for (IndexT i = 0; i < it2.val->Size(); i++)
					(*it2.val)[i]->node->Update();
				it2++;
			}
			it1++;
		}
	}
}

} // namespace Visibility
