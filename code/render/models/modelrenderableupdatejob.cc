//------------------------------------------------------------------------------
//  @file modelboundingboxupdatejob.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/jobs.h"
#include "profiling/profiling.h"
#include "models/modelcontext.h"
#include "coregraphics/graphicsdevice.h"

#include "objects_shared.h"

namespace Models
{

//------------------------------------------------------------------------------
/**
*/
void
ModelRenderableUpdateJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(ModelRenderableUpdateJob, Models);
    ModelContext::ModelInstance::Renderable* nodeStates = (ModelContext::ModelInstance::Renderable*)ctx.uniforms[0];
    ModelContext::ModelInstance::Transformable* nodeTransforms = (ModelContext::ModelInstance::Transformable*)ctx.uniforms[1];
    const Math::mat4 cameraTransform = *(Math::mat4*)ctx.uniforms[2];
    SizeT i;
    for (i = 0; i < ctx.numSlices; i++)
    {
        const Models::NodeInstanceRange* stateRange = (const Models::NodeInstanceRange*)N_JOB_INPUT(ctx, i, 0);
        SizeT j;
        for (j = stateRange->begin; j < stateRange->end; j++)
        {

            Math::mat4 transform = nodeTransforms->nodeTransforms[nodeStates->nodeTransformIndex[j]];
            Math::bbox box = nodeStates->origBoundingBoxes[j];
            box.affine_transform(transform);
            nodeStates->nodeBoundingBoxes[j] = box;

            // calculate view vector to calculate LOD
            Math::vec4 viewVector = cameraTransform.position - transform.position;
            float viewDistance = length(viewVector);
            float textureLod = viewDistance - 38.5f;

            Models::NodeInstanceFlags& nodeFlag = nodeStates->nodeFlags[j];

            // Calculate if object should be culled due to LOD
            const Util::Tuple<float, float>& lodDistances = nodeStates->nodeLodDistances[j];
            if (viewDistance >= Util::Get<0>(lodDistances) && viewDistance < Util::Get<1>(lodDistances))
                nodeFlag = SetBits(nodeFlag, Models::NodeInstance_LodActive);
            else
                nodeFlag = UnsetBits(nodeFlag, Models::NodeInstance_LodActive);

            // Set LOD factor
            float lodFactor = (viewDistance - (Util::Get<0>(lodDistances) + 1.5f)) / (Util::Get<1>(lodDistances) - (Util::Get<0>(lodDistances) + 1.5f));
            nodeStates->nodeLods[j] = lodFactor;

            // Notify materials system this LOD might be used
            Materials::surfacePool->SetMaxLOD(nodeStates->nodeSurfaceResources[j], textureLod);

            // Allocate object constants
            ObjectsShared::ObjectBlock block;
            transform.store(block.Model);
            inverse(transform).store(block.InvModel);
            block.DitherFactor = lodFactor;
            block.ObjectId = j;

            uint offset = CoreGraphics::SetGraphicsConstants(CoreGraphics::GlobalConstantBufferType::VisibilityThreadConstantBuffer, block);
            nodeStates->nodeStates[j].resourceTableOffsets[nodeStates->nodeStates[j].objectConstantsIndex] = offset;
        }
    }
}

} // namespace Models
