//------------------------------------------------------------------------------
//  @file modeltransformupdatejob.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/jobs.h"
#include "profiling/profiling.h"
#include "models/modelcontext.h"
namespace Models
{

//------------------------------------------------------------------------------
/**
*/
void
ModelTransformUpdateJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(ModelTransformUpdateJob, Models);
    ModelContext::ModelInstance::Transformable* nodeTransforms = (ModelContext::ModelInstance::Transformable*)ctx.uniforms[0];

    SizeT i;
    for (i = 0; i < ctx.numSlices; i++)
    {
        const Models::NodeInstanceRange* transformRange = (const Models::NodeInstanceRange*)N_JOB_INPUT(ctx, i, 0);
        bool* pending = (bool*)N_JOB_INPUT(ctx, i, 1);
        const Math::mat4* pendingTransforms = (Math::mat4*)N_JOB_INPUT(ctx, i, 2);

        if (pending[i])
        {
            // The pending transform is the root of the model
            const Math::mat4 transform = pendingTransforms[i];
            pending[i] = false;

            // Set root transform
            nodeTransforms->nodeTransforms[transformRange->begin] = transform;

            SizeT j;
            for (j = transformRange->begin; j < transformRange->end; j++)
            {
                uint32_t parent = nodeTransforms->nodeParents[j];
                Math::mat4 parentTransform = nodeTransforms->nodeTransforms[parent];
                Math::mat4 orig = nodeTransforms->origTransforms[j];
                nodeTransforms->nodeTransforms[j] = orig * parentTransform;
            }
        }

    }
}

} // namespace Models
