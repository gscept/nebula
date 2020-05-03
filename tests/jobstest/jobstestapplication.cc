//------------------------------------------------------------------------------
//  jobstestapplication.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "jobstestapplication.h"
#include "math/matrix44.h"
#include "math/float4.h"
#include "timing/timer.h"
#include "core/debug.h"
#include "particles/particle.h"
#include "testdata.h"

using namespace Jobs;
using namespace Math;
using namespace Timing;

namespace Particles
{

#if __PS3__
extern "C" {
    extern const char _binary_jqjob_render_particlejob_ps3_bin_start[];
    extern const char _binary_jqjob_render_particlejob_ps3_bin_size[];
}
#else
extern void ParticleJobFunc(const JobFuncContext& ctx);
#endif


//------------------------------------------------------------------------------
/**
*/
JobsTestApplication::JobsTestApplication()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
JobsTestApplication::~JobsTestApplication()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
bool
JobsTestApplication::Open()
{
    if (ConsoleApplication::Open())
    {
        // setup jobs subsystem
        this->jobSystem = JobSystem::Create();
        this->jobSystem->Setup();

        this->gameContentServer = IO::GameContentServer::Create();
        this->gameContentServer->SetTitle("RL Test Title");
        this->gameContentServer->SetTitleId("RLTITLEID");
        this->gameContentServer->SetVersion("1.00");
        this->gameContentServer->Setup();

        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
JobsTestApplication::Close()
{
    this->gameContentServer->Discard();
    this->gameContentServer = 0;
    this->jobSystem->Discard();
    this->jobSystem = 0;
    ConsoleApplication::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
JobsTestApplication::Run()
{
    // uniform
#if __PS3__
    static const bool generateVertexList = false;
    n_assert(sizeof(JobUniformData) == uniform_size_bytes);   
    
#endif
    JobUniformData *un = n_new(JobUniformData);
    Memory::Copy(uniform, un, sizeof(JobUniformData));
#if __PS3__
    un->generateVertexList = generateVertexList;
#endif
    JobUniformDesc uniformDesc(un, sizeof(JobUniformData), 0);

    const int input_size_bytes = sizeof(Particle) * particleCount; 
    
#if __PS3__
    const SizeT inputSliceSize = generateVertexList ? PARTICLE_JOB_INPUT_SLICE_SIZE__VSTREAM_ON :
                                                      PARTICLE_JOB_INPUT_SLICE_SIZE__VSTREAM_OFF;
#else
    const SizeT inputSliceSize = PARTICLE_JOB_INPUT_SLICE_SIZE__VSTREAM_OFF;
#endif
    JobDataDesc inputDesc(particlesInput, input_size_bytes, inputSliceSize);


    // output
    Jobs::JobDataDesc outputDesc;
    Particle *particlesOutput = n_new_array(Particle, particleCount);
    n_assert(particlesOutput);
    const int inputBufferSize = input_size_bytes;
    int sliceCount = (inputBufferSize +(inputSliceSize-1)) / inputSliceSize;
    n_assert(sliceCount > 0);
    
    // and resize the slice-outputbuffer if necessary
    JobSliceOutputData *sliceOutput = n_new_array(JobSliceOutputData, sliceCount);
    n_assert(sliceOutput);
    n_assert(!((int)sliceOutput & 0xF));

#if __PS3__
    unsigned char *vertexCache = NULL;
    if(generateVertexList)
    {
        const SizeT vertexBufferSize = sliceCount * PARTICLE_JOB_PS3_OUTPUT_VBUFFER_SLICE_SIZE;
        vertexCache = n_new_array(unsigned char, vertexBufferSize);
        n_assert(vertexCache);
        n_assert(vertexBufferSize > 0);
        outputDesc = Jobs::JobDataDesc(particlesOutput, inputBufferSize, inputSliceSize,
                                       sliceOutput, sizeof(JobSliceOutputData) * sliceCount, sizeof(JobSliceOutputData),
                                       vertexCache, vertexBufferSize, PARTICLE_JOB_PS3_OUTPUT_VBUFFER_SLICE_SIZE);
    }
    else
#endif
    {
        outputDesc = Jobs::JobDataDesc(particlesOutput, inputBufferSize, inputSliceSize,
                                       sliceOutput, sizeof(JobSliceOutputData) * sliceCount, sizeof(JobSliceOutputData));
    }


    ///////////////////////
    // setup job and run it
    #if __PS3__
    Jobs::JobFuncDesc jobFunc(_binary_jqjob_render_particlejob_ps3_bin_start, _binary_jqjob_render_particlejob_ps3_bin_size);
    #else
    Jobs::JobFuncDesc jobFunc(ParticleJobFunc);
    #endif
    this->job = Jobs::Job::Create();
    job->Setup(uniformDesc, inputDesc, outputDesc, jobFunc);

    this->jobPort = Jobs::JobPort::Create();
    jobPort->Setup();

    // 1st job takes very long
    jobPort->PushFlush();
    jobPort->PushJob(job);
    jobPort->WaitDone();


    Timer parallelTimer;
    parallelTimer.Start();
    IndexT runIndex;
    SizeT numRuns = 10000;
    for (runIndex = 0; runIndex < numRuns; runIndex++)
    {
        this->jobPort->PushFlush();
        this->jobPort->PushJob(this->job);
        this->jobPort->WaitDone();
    }
    parallelTimer.Stop();
    n_printf("(runs %d): %fs\n", runIndex, parallelTimer.GetTime());
    job->Discard();
    job = 0;
    jobPort->Discard();
    jobPort = 0;

    n_printf("result:\n");
    for(int i = 0; i < sliceCount; ++i)
    {
        n_printf("  slice %d numLivingParticles: %d\n", i, sliceOutput[i].numLivingParticles);
    }
    n_delete_array(sliceOutput);
    sliceOutput = NULL;
#if __PS3__
    if(vertexCache)
    {
        n_delete_array(vertexCache);
        vertexCache = NULL;
    }
#endif
}

} // namespace Test
