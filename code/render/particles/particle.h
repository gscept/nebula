#pragma once
//------------------------------------------------------------------------------
/**
    @class Particles::Particle
    
    The particle structure holds the current state of a single particle and
    common data for particle-job and nebula3 particle system

    !! NOTE: this header is also included from job particlejob.cc, so only 
    !! job-compliant headers can be included here

    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/bbox.h"
#include "particles/emitterattrs.h"
#include "threading/interlocked.h"
#include "threading/event.h"

//------------------------------------------------------------------------------
namespace Particles
{
    /// number of samples in envelope curves
    static const SizeT ParticleSystemNumEnvelopeSamples = 192;
    static const SizeT MaxNumRenderedParticles = 65535;

    struct Particle
    {
        Math::vec4 position;
        Math::vec4 startPosition;
        Math::vec4 stretchPosition;
        Math::vec4 velocity;
        Math::vec4 uvMinMax;
        Math::vec4 color;
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
    struct ParticleJobUniformData
    {
        ParticleJobUniformData()
            : gravity(0.0f, 0.0f, 0.0f)
            , windVector(0.0f, 0.0f, 0.0f)
            , stretchToStart(false)
            , stretchTime(0)
            , sampleBuffer(nullptr)
        {};
        Math::vector gravity;
        Math::vector windVector;
        bool stretchToStart;
        float stretchTime;

        // static sample buffer
        const float* sampleBuffer;
    };

    // each job-slice generates this output
    struct ParticleJobSliceOutputData
    {
        Math::bbox bbox;
        unsigned int numLivingParticles;
    };

    struct ParticleJobContext
    {
        const Particle* inputParticles;
        Particle* outputParticles;
        const ParticleJobUniformData* uniformData;
        float stepTime;
        uint numParticles;
        ParticleJobSliceOutputData* output;
    };

    static const SizeT ParticleJobInputElementSize = sizeof(Particle);

    static const SizeT ParticleJobInputMaxElementsPerSlice = JobMaxSliceSize / ParticleJobInputElementSize;
    static const SizeT ParticleJobInputSliceSize = ParticleJobInputMaxElementsPerSlice * ParticleJobInputElementSize;

} // namespace Particles
//------------------------------------------------------------------------------
