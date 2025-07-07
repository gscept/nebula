//------------------------------------------------------------------------------
//  visibilitydependencyjob.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "jobs/jobs.h"
#include "visibilitycontext.h"
#include "profiling/profiling.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void
VisibilityDependencyJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(VisibilityDependencyJob, Visibility);
    DependencyMode mode = *(DependencyMode*)ctx.uniforms[0];
    uint32_t numResults = ctx.inputSizes[0] / sizeof(Math::ClipStatus::Type);
    IndexT bIndexInA = *(uint32_t*)ctx.uniforms[1];
    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    {
        Math::ClipStatus::Type* aResults = (Math::ClipStatus::Type*)N_JOB_INPUT(ctx, sliceIdx, 0);
        Math::ClipStatus::Type* bResults = (Math::ClipStatus::Type*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

        if (mode == DependencyMode_Masked)
        {
            for (uint32_t i = 0; i < numResults; i++)
            {
                if (aResults[i] == Math::ClipStatus::Inside)
                    bResults[i] = Math::ClipStatus::Outside;
            }
        }
        else if (mode == DependencyMode_Total)
        {
            // if B is not visible from A, make sure B produces nothing
            if (!aResults[bIndexInA])
            {
                for (uint32_t i = 0; i < numResults; i++)
                {
                    bResults[i] = Math::ClipStatus::Outside;
                }
            }
        }
        
    }
}

} // namespace Visibility
