//------------------------------------------------------------------------------
//  particlejob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "math/vec4.h"
#include "particles/particle.h"

#if __NEBULA_NO_ASSERT__
#define ASSERT_POINT(p)
#define ASSERT_VECTOR(v)
#else
#define ASSERT_POINT(p)     { n_assert(p.w > 0.995f);  n_assert(p.w < 1.005f) }
#define ASSERT_VECTOR(v)    { n_assert(v.w > -0.005f); n_assert(v.w < 0.005f) }
#endif

namespace Particles
{

using namespace Math;

//------------------------------------------------------------------------------
/**

    NOTE: JobStepAndVlist does the same as JobStep, but it also generates the vertex 
          list. It could be in one function, but then we would always have to check a 
          boolean value for each particle. Thats bad for the SPU, since it doesnt
          have any branch prediction, and it would have to throw away commands 
          and fetch new commands from the program in each iteration
*/


/// lookup samples at index "sampleIndex" in sample-table
const float* LookupEnvelopeSamples(const float sampleBuffer[ParticleSystemNumEnvelopeSamples*EmitterAttrs::NumEnvelopeAttrs], IndexT sampleIndex);
/// update bounding box (min, max) with vector v
void UpdateBbox(const vec4& v, vec4& min, vec4& max);
/// update the age of one particle
void ParticleUpdateAge(const float stepTime, const Particle& in, Particle& out);
/// integrate the particle state with a given time-step
void ParticleStep(const ParticleJobUniformData* perSystemUniforms, const float stepTime, const Particle& in, Particle& out, ParticleJobSliceOutputData* sliceOutput);
/// update particle system step
void JobStep(const ParticleJobUniformData* perSystemUniforms, const float stepTime, unsigned int numParticles, const Particle* particles_input, Particle* particles_output, ParticleJobSliceOutputData* sliceOutput);

//------------------------------------------------------------------------------
/**
*/
__forceinline 
const float*
LookupEnvelopeSamples(const float sampleBuffer[ParticleSystemNumEnvelopeSamples*EmitterAttrs::NumEnvelopeAttrs], IndexT sampleIndex)
{
    n_assert(sampleIndex >= 0);
    n_assert(sampleIndex < ParticleSystemNumEnvelopeSamples);
    return sampleBuffer + (sampleIndex * EmitterAttrs::NumEnvelopeAttrs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline 
void
UpdateBbox(const vec4& v, vec4& min, vec4& max)
{
    min = minimize(min, v);
    max = maximize(max, v);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
void ParticleUpdateAge(const float stepTime, const Particle& in, Particle& out)
{
    // update particle's age
    out.oneDivLifeTime = in.oneDivLifeTime;
    out.age = in.age + stepTime;
    out.relAge = in.relAge + stepTime * in.oneDivLifeTime;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
void ParticleStep(const ParticleJobUniformData* perSystemUniforms, const float stepTime, const Particle& in, Particle& out, ParticleJobSliceOutputData* sliceOutput)
{
    ++sliceOutput->numLivingParticles;

    // copy the *const* members
    out.startPosition = in.startPosition;
    out.uvMinMax = in.uvMinMax;
    out.rotationVariation = in.rotationVariation;
    out.sizeVariation = in.sizeVariation;

    const IndexT sampleIndex = IndexT(out.relAge * (float)(ParticleSystemNumEnvelopeSamples-1));
    n_assert(sampleIndex >= 0);            
    n_assert(sampleIndex < ParticleSystemNumEnvelopeSamples);

    const float* samples = LookupEnvelopeSamples(perSystemUniforms->sampleBuffer, sampleIndex);
    
    // compute current particle acceleration
    Math::vec4 acceleration = perSystemUniforms->windVector * samples[EmitterAttrs::AirResistance];
    acceleration += perSystemUniforms->gravity;
    acceleration *= samples[EmitterAttrs::Mass];

    float curStretchTime = 0.0f;

    // fix stretch time (if particle stretch is enabled
    if (perSystemUniforms->stretchTime > 0.0f)
    {
        curStretchTime = (perSystemUniforms->stretchTime > out.age) ? out.age : perSystemUniforms->stretchTime;
    }

    // update position, velocity, rotation
    ASSERT_POINT(in.position);
    ASSERT_VECTOR(in.velocity);
    out.position = in.position + in.velocity * samples[EmitterAttrs::VelocityFactor] * stepTime;
    ASSERT_POINT(out.position);
    sliceOutput->bbox.extend(Math::bbox(out.position, Math::vector(samples[EmitterAttrs::Size])));
    out.velocity = in.velocity + acceleration * stepTime;
    ASSERT_VECTOR(out.velocity);
    if (perSystemUniforms->stretchToStart)
    {
        // NOTE: don't support particle rotation in stretch modes
        out.stretchPosition = in.startPosition;
        out.rotation = in.rotation;
    }
    else if (curStretchTime > 0.0f)
    {
        // NOTE: don't support particle rotation in stretch modes
        // ???
        out.stretchPosition = out.position -
                              (out.velocity - acceleration * curStretchTime * 0.5f) *
                              (perSystemUniforms->stretchTime * samples[EmitterAttrs::VelocityFactor]);
        out.rotation = in.rotation;
    }                
    else
    {
        out.stretchPosition = out.position;
        out.rotation = in.rotation + in.rotationVariation * samples[EmitterAttrs::RotationVelocity] * stepTime;
    }
    out.color.loadu(&(samples[EmitterAttrs::Red]));
    out.color.w = Math::clamp(out.color.w, 0.0f, 1.0f);
    out.size = samples[EmitterAttrs::Size] * in.sizeVariation;
}

//------------------------------------------------------------------------------
/**
*/
void
JobStep(const ParticleJobUniformData* perSystemUniforms, const float stepTime, unsigned int numParticles,
             const Particle* particles_input, Particle* particles_output, 
             ParticleJobSliceOutputData* sliceOutput)
{
    unsigned int i;
    sliceOutput->bbox.begin_extend();
    for (i = 0; i < numParticles; i++)
    {
        const Particle &in = particles_input[i];
        Particle &out = particles_output[i];
        ParticleUpdateAge(stepTime, in, out);
        if (out.relAge < 1.0f)
        {
            ParticleStep(perSystemUniforms, stepTime, in, out, sliceOutput);
        }
    }
    sliceOutput->bbox.end_extend();
}

} // namespace Particles
