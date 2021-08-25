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
        const ModelContext::NodeInstanceRange* transformRange = (const ModelContext::NodeInstanceRange*)N_JOB_INPUT(ctx, i, 0);
        bool* pending = (const bool*)N_JOB_INPUT(ctx, i, 1);
        const Math::mat4* pendingTransforms = (Math::mat4*)N_JOB_INPUT(ctx, i, 2);

        if (pending[i])
        {
            // The pending transform is the root of the model
            const Math::mat4 transform = pendingTransforms[i];
            pending[i] = false;

            // Set root transform
            nodeTransforms->nodeTransforms[transformRange->offset] = transform;

            SizeT j;
            for (j = 0; j < transformRange->size; j++)
            {
                uint32_t parent = nodeTransforms->nodeParents[transformRange->offset + j];
                Math::mat4 parentTransform = nodeTransforms->nodeTransforms[parent];
                Math::mat4 orig = nodeTransforms->origTransforms[transformRange->offset + j];
                nodeTransforms->nodeTransforms[transformRange->offset + j] = orig * parentTransform;
            }
        }

    }
}

} // namespace Models
