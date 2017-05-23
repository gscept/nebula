#pragma once
//------------------------------------------------------------------------------
/**
    @class Jobs::JobContext
    
    JobContext structure handed to job processing functions. Note that
    this header must be self-contained, since it will be included
    on the SPU side on PS3.
   
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/

#define JOBFUNCCONTEXT_MAXINPUTS 4
#define JOBFUNCCONTEXT_MAXUNIFORMS 4
#define JOBFUNCCONTEXT_MAXOUTPUTS 4

struct JobFuncContext
{
    unsigned char* scratch;
    
    unsigned int numUniforms;
    unsigned char* uniforms[JOBFUNCCONTEXT_MAXUNIFORMS];
    unsigned int uniformSizes[JOBFUNCCONTEXT_MAXUNIFORMS];

    unsigned int numInputs;
    unsigned char* inputs[JOBFUNCCONTEXT_MAXINPUTS];
    unsigned int inputSizes[JOBFUNCCONTEXT_MAXINPUTS];

    unsigned int numOutputs;
    unsigned char* outputs[JOBFUNCCONTEXT_MAXOUTPUTS];
    unsigned int outputSizes[JOBFUNCCONTEXT_MAXOUTPUTS];
};
