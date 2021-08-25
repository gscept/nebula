//------------------------------------------------------------------------------
//  @file modelboundingboxupdatejob.cc
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
ModelBoundingBoxUpdateJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(ModelBoundingBoxUpdateJob, Models);
    ModelContext::ModelInstance::Renderable* nodeStates = (ModelContext::ModelInstance::Renderable*)ctx.uniforms[0];
    ModelContext::ModelInstance::Transformable* nodeTransforms = (ModelContext::ModelInstance::Transformable*)ctx.uniforms[1];
    const Math::mat4 cameraTransform = *(Math::mat4*)ctx.uniforms[2];
    SizeT i;
    for (i = 0; i < ctx.numSlices; i++)
    {
        const ModelContext::NodeInstanceRange* stateRange = (const ModelContext::NodeInstanceRange*)N_JOB_INPUT(ctx, i, 0);
        SizeT j;
        for (j = 0; j < stateRange->size; j++)
        {

            Math::mat4 transform = nodeTransforms->nodeTransforms[nodeStates->nodeTransformIndex[stateRange->offset + j]];
            Math::bbox box = nodeStates->origBoundingBoxes[stateRange->offset + j];
            box.affine_transform(transform);
            nodeStates->nodeBoundingBoxes[stateRange->offset + j] = box;

            // calculate view vector to calculate LOD
            Math::vec4 viewVector = cameraTransform.position - transform.position;
            float viewDistance = length(viewVector);
            float textureLod = viewDistance - 38.5f;

            Models::NodeInstanceFlags& nodeFlag = nodeStates->nodeFlags[stateRange->offset + j];

            // Calculate if object should be culled due to LOD
            const Util::Tuple<float, float>& lodDistances = nodeStates->nodeLodDistances[stateRange->offset + j];
            if (viewDistance >= Util::Get<0>(lodDistances) && viewDistance < Util::Get<1>(lodDistances))
                nodeFlag = SetBits(nodeFlag, Models::NodeInstance_LodActive);
            else
                nodeFlag = UnsetBits(nodeFlag, Models::NodeInstance_LodActive);

            // Set LOD factor
            nodeStates->nodeLods[stateRange->offset + j] = (viewDistance - (Util::Get<0>(lodDistances) + 1.5f)) / (Util::Get<1>(lodDistances) - (Util::Get<0>(lodDistances) + 1.5f));

            // Notify materials system this LOD might be used
            Materials::surfacePool->SetMaxLOD(nodeStates->nodeSurfaceResources[stateRange->offset + j], textureLod);
        }
    }
}

} // namespace Models
