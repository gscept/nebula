//------------------------------------------------------------------------------
//  skeletonevaljob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "characters/skeletonjoint.h"
#include "profiling/profiling.h"

using namespace Math;
namespace Characters
{

//------------------------------------------------------------------------------
/**
*/
void
SkeletonEvalJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE_ACCUM(SkeletonEvaluateAndTransform, Character);
    mat4* invPoseMatrixBase = (mat4*)ctx.uniforms[0];
    mat4* mixPoseMatrixBase = (mat4*)ctx.uniforms[1];
    mat4* unscaledMatrixBase = (mat4*)ctx.scratch;

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    {
        vec4 finalScale, parentFinalScale, variationTranslation;
        vec4 translate(0.0f, 0.0f, 0.0f, 1.0f);
        vec4 scale(1.0f, 1.0f, 1.0f, 0.0f);
        vec4 parentScale(1.0f, 1.0f, 1.0f, 0.0f);
        vec4 parentTranslate(0.0f, 0.0f, 0.0f, 1.0f);
        quat rotate;
        vec4 vec1111(1.0f, 1.0f, 1.0f, 1.0f);

        // load pointers from context
        // NOTE: the samplesBase pointer may be NULL if no valid animation
        // data exists, in this case the skeleton should simply be set
        // to its jesus pose
        SkeletonJobJoint* compsBase = (SkeletonJobJoint*)(ctx.inputs[0] + sliceIdx * ctx.inputSizes[0]);
        n_assert(0 != compsBase);
        const vec4* samplesBase = (const vec4*)(ctx.inputs[1] + sliceIdx * ctx.inputSizes[1]);
        const vec4* samplesPtr = samplesBase;

        mat4* scaledMatrixBase = (mat4*)(ctx.outputs[0] + sliceIdx * ctx.outputSizes[0]);
        mat4* skinMatrixBase = (mat4*)(ctx.outputs[1] + sliceIdx * ctx.outputSizes[1]);

        // input samples may optionally include velocity samples which we need to skip...
        uint numElements = ctx.inputSizes[0] / sizeof(SkeletonJobJoint);
        uint sampleWidth = (ctx.inputSizes[1] / numElements) / sizeof(vec4);

        // compute number of joints
        int jointIndex;
        int numJoints = ctx.inputSizes[0] / sizeof(SkeletonJobJoint);
        for (jointIndex = 0; jointIndex < numJoints; jointIndex++)
        {
            // load joint translate/rotate/scale
            translate.load((scalar*)&(samplesPtr[0]));
            rotate.load((scalar*)&(samplesPtr[1]));
            scale.load((scalar*)&(samplesPtr[2]));
            samplesPtr += sampleWidth;

            const SkeletonJobJoint& comps = compsBase[jointIndex];

            // load variation scale
            finalScale.load(&comps.varScaleX);
            finalScale = scale * finalScale;
            finalScale = permute(finalScale, vec1111, 0, 1, 2, 7);

            mat4& unscaledMatrix = unscaledMatrixBase[jointIndex];
            mat4& scaledMatrix = scaledMatrixBase[jointIndex];

            // update unscaled matrix
            // animation rotation
            unscaledMatrix = rotationquat(rotate);

            // load variation translation
            variationTranslation.load(&comps.varTranslationX);
            unscaledMatrix.translate(xyz(translate + variationTranslation));

            // add mix pose if the pointer is set
            if (mixPoseMatrixBase)
                unscaledMatrix = unscaledMatrix * mixPoseMatrixBase[jointIndex];

            // update scaled matrix
            // scale after rotation
            scaledMatrix.x_axis = unscaledMatrix.x_axis * splat_x(finalScale);
            scaledMatrix.y_axis = unscaledMatrix.y_axis * splat_y(finalScale);
            scaledMatrix.z_axis = unscaledMatrix.z_axis * splat_z(finalScale);

            if (InvalidIndex == comps.parentJointIndex)
            {
                // no parent directly set translation
                scaledMatrix.position = unscaledMatrix.position;
            }
            else
            {
                // load parent animation scale
                parentScale.load((scalar*)(samplesBase + sampleWidth * jointIndex + 2));
                const SkeletonJobJoint& parentComps = compsBase[comps.parentJointIndex];

                // combine both scaling types
                parentFinalScale.load(&parentComps.varScaleX);
                parentFinalScale = parentScale * parentFinalScale;
                parentFinalScale = permute(parentFinalScale, vec1111, 0, 1, 2, 7);

                // transform our unscaled position with parent scaling
                unscaledMatrix.position = unscaledMatrix.position * parentFinalScale;
                scaledMatrix.position = unscaledMatrix.position;

                // apply rotation and relative animation translation of parent 
                const mat4& parentUnscaledMatrix = unscaledMatrixBase[comps.parentJointIndex];
                unscaledMatrix = unscaledMatrix * parentUnscaledMatrix;
                scaledMatrix = scaledMatrix * parentUnscaledMatrix;
            }
            skinMatrixBase[jointIndex] = invPoseMatrixBase[jointIndex] * scaledMatrix;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
SkeletonEvalJobWithVariation(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE_ACCUM(SkeletonEvaluateAndTransformWithVariation, Character);
    mat4* invPoseMatrixBase = (mat4*)ctx.uniforms[0];
    mat4* mixPoseMatrixBase = (mat4*)ctx.uniforms[1];
    mat4* unscaledMatrixBase = (mat4*)ctx.scratch;

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    {
        vec4 finalScale, parentFinalScale, variationTranslation;
        vec4 translate(0.0f, 0.0f, 0.0f, 1.0f);
        vec4 scale(1.0f, 1.0f, 1.0f, 0.0f);
        vec4 parentScale(1.0f, 1.0f, 1.0f, 0.0f);
        vec4 parentTranslate(0.0f, 0.0f, 0.0f, 1.0f);
        quat rotate;
        vec4 vec1111(1.0f, 1.0f, 1.0f, 1.0f);

        // load pointers from context
        // NOTE: the samplesBase pointer may be NULL if no valid animation
        // data exists, in this case the skeleton should simply be set
        // to its jesus pose
        SkeletonJobJoint* compsBase = (SkeletonJobJoint*)N_JOB_INPUT(ctx, sliceIdx, 0);
        n_assert(0 != compsBase);
        const vec4* samplesBase = (const vec4*)N_JOB_INPUT(ctx, sliceIdx, 1);
        const vec4* samplesPtr = samplesBase;

        mat4* scaledMatrixBase = (mat4*)N_JOB_OUTPUT(ctx, sliceIdx, 0);
        mat4* skinMatrixBase = (mat4*)N_JOB_OUTPUT(ctx, sliceIdx, 1);

        // input samples may optionally include velocity samples which we need to skip...
        uint numElements = ctx.inputSizes[0] / sizeof(SkeletonJobJoint);
        uint sampleWidth = (ctx.inputSizes[1] / numElements) / sizeof(vec4);

        // compute number of joints
        int jointIndex;
        int numJoints = ctx.inputSizes[0] / sizeof(SkeletonJobJoint);
        for (jointIndex = 0; jointIndex < numJoints; jointIndex++)
        {
            // load joint translate/rotate/scale
            translate.load((scalar*)&(samplesPtr[0]));
            rotate.load((scalar*)&(samplesPtr[1]));
            scale.load((scalar*)&(samplesPtr[2]));
            samplesPtr += sampleWidth;

            const SkeletonJobJoint& comps = compsBase[jointIndex];

            // load variation scale
            finalScale.load(&comps.varScaleX);
            finalScale = scale * finalScale;
            finalScale = permute(finalScale, vec1111, 0, 1, 2, 7);

            mat4& unscaledMatrix = unscaledMatrixBase[jointIndex];
            mat4& scaledMatrix = scaledMatrixBase[jointIndex];

            // update unscaled matrix
            // animation rotation
            unscaledMatrix = rotationquat(rotate);

            // load variation translation
            variationTranslation.load(&comps.varTranslationX);
            unscaledMatrix.translate(xyz(translate + variationTranslation));

            // add mix pose if the pointer is set
            if (mixPoseMatrixBase)  
                unscaledMatrix = unscaledMatrix * mixPoseMatrixBase[jointIndex];

            // update scaled matrix
            // scale after rotation
            scaledMatrix.x_axis = unscaledMatrix.x_axis * splat_x(finalScale);
            scaledMatrix.y_axis = unscaledMatrix.y_axis * splat_y(finalScale);
            scaledMatrix.z_axis = unscaledMatrix.z_axis * splat_z(finalScale);

            if (InvalidIndex == comps.parentJointIndex)
            {
                // no parent directly set translation
                scaledMatrix.position = unscaledMatrix.position;
            }
            else
            {
                // load parent animation scale
                parentScale.load((scalar*)(samplesBase + sampleWidth * jointIndex + 2));
                const SkeletonJobJoint& parentComps = compsBase[comps.parentJointIndex];

                // load parent variation scale
                parentFinalScale.load(&parentComps.varScaleX);

                // combine both scaling types
                parentFinalScale = parentScale * parentFinalScale;
                parentFinalScale = permute(parentFinalScale, vec1111, 0, 1, 2, 7);

                // transform our unscaled position with parent scaling
                unscaledMatrix.position = unscaledMatrix.position * parentFinalScale;
                scaledMatrix.position = unscaledMatrix.position;

                // apply rotation and relative animation translation of parent 
                const mat4& parentUnscaledMatrix = unscaledMatrixBase[comps.parentJointIndex];
                unscaledMatrix = unscaledMatrix * parentUnscaledMatrix;
                scaledMatrix = scaledMatrix * parentUnscaledMatrix;
            }
            skinMatrixBase[jointIndex] = invPoseMatrixBase[jointIndex] * scaledMatrix;
        }
    }
}

} // namespace Characters
