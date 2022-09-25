//------------------------------------------------------------------------------
//  msaaresolvedepth4.fx
//  (C) 2022 Gustav Sterbrant
//------------------------------------------------------------------------------

#define IMAGE_TYPE_R32F
#define METHOD_SAMPLE_ZERO
#define SAMPLES 64
#include "msaaresolve.fxh"

//------------------------------------------------------------------------------
/**
*/
program Resolve[string Mask = "Resolve"; ]
{
    ComputeShader = main();
};
