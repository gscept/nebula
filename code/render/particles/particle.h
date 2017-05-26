#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::Particle
    
    The particle structure holds the current state of a single particle and
    common data for particle-job and nebula3 particle system

    !! NOTE: this header is also included from job particlejob.cc, so only 
    !! job-compliant headers can be included here

    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "math/float4.h"
#include "math/vector.h"
#include "math/matrix44.h"
#include "math/bbox.h"
#include "particles/emitterattrs.h"

//------------------------------------------------------------------------------
namespace Particles
{
    /// number of samples in envelope curves
    static const SizeT ParticleSystemNumEnvelopeSamples = 192;
    static const SizeT MaxNumRenderedParticles = 65535;
#if __PS3__
    static const SizeT VerticesPerParticle = 4;
#endif

    struct Particle
    {
        Math::float4 position;
        Math::float4 startPosition;
        Math::float4 stretchPosition;
        Math::float4 velocity;
        Math::float4 uvMinMax;
        Math::float4 color;
        float rotation;      
        float rotationVariation;
        float size;
        float sizeVariation;
        float oneDivLifeTime;
        float relAge;                       // between 0 and 1, particle is dead if age > 1.0
        float age;                          // absolute age
        float particleId;                   // id for differing particles in vertex shader
    };

    typedef unsigned int JOB_ID;

    // uniform data for particle system instances, used for job-uniform data as well,
    // so we dont have to copy that much
	struct JobUniformData
	{
        JobUniformData() : windVector(0.0f, 0.0f, 0.0f) {}
		Math::float4 gravity;
		Math::vector windVector;
		float stepTime;
        bool stretchToStart;
        float stretchTime;
        // static sample buffer
        float sampleBuffer[ParticleSystemNumEnvelopeSamples * EmitterAttrs::NumEnvelopeAttrs];
#if __PS3__
        Math::float4 du;
        // 1 / numAnimPhases, splatted into all vector elements
        Math::float4 numAnimPhasesReciprocal;
        bool generateVertexList;
        struct ParticleRaw
        {
            // the 4 untransformed particle corners
            Math::float4 corners[4];
        } particleRaw;
        // inverse rotational part of the view matrix
        Math::matrix44 invViewRot;
        // inverse particle system transform
        Math::matrix44 invTransform;
        // the transformed normal for this job
        Math::float4 normal;
#endif
	};

    // each job-slice generates this output
    struct JobSliceOutputData
    {
		Math::bbox bbox;
        unsigned int numLivingParticles;
    };

#if __PS3__
    // only the ps3 generates the render vertex stream in the job, other
    // platforms do it in the renderer
    struct ParticleQuadRenderVertex
    {
        Math::float4 pos;
        Math::float4 normal;
        Math::float4 tex;
        Math::float4 color;
    };
#endif

    static const SizeT PARTICLE_JOB_INPUT_ELEMENT_SIZE = sizeof(Particle);

#if __PS3__
    // vertex output buffer is twice as big as input buffer, thats why we divide by 2, if
    // for vertex stream generation jiobs
    static const SizeT PARTICLE_JOB_INPUT_MAX_ELEMENTS_PER_SLICE__VSTREAM_ON = (JobMaxSliceSize / PARTICLE_JOB_INPUT_ELEMENT_SIZE) / 2;
    static const SizeT PARTICLE_JOB_INPUT_SLICE_SIZE__VSTREAM_ON = PARTICLE_JOB_INPUT_MAX_ELEMENTS_PER_SLICE__VSTREAM_ON * PARTICLE_JOB_INPUT_ELEMENT_SIZE;
#endif
    static const SizeT PARTICLE_JOB_INPUT_MAX_ELEMENTS_PER_SLICE__VSTREAM_OFF = JobMaxSliceSize / PARTICLE_JOB_INPUT_ELEMENT_SIZE;
    static const SizeT PARTICLE_JOB_INPUT_SLICE_SIZE__VSTREAM_OFF = PARTICLE_JOB_INPUT_MAX_ELEMENTS_PER_SLICE__VSTREAM_OFF * PARTICLE_JOB_INPUT_ELEMENT_SIZE;

#if __PS3__
    static const SizeT PARTICLE_JOB_PS3_RENDER_QUAD_SIZE = 4 * sizeof(Particles::ParticleQuadRenderVertex);
    static const SizeT PARTICLE_JOB_PS3_OUTPUT_VBUFFER_SLICE_SIZE = PARTICLE_JOB_INPUT_MAX_ELEMENTS_PER_SLICE__VSTREAM_ON * PARTICLE_JOB_PS3_RENDER_QUAD_SIZE;
#endif

} // namespace Particles
//------------------------------------------------------------------------------
