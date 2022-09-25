//------------------------------------------------------------------------------
//  msaaresolvedepth8.fx
//  (C) 2022 Gustav Sterbrant
//------------------------------------------------------------------------------

#define IMAGE_TYPE_R32F
#define METHOD METHOD_ZERO_SAMPLE
#define SAMPLES 8
#include "msaaresolve.fxh"

//------------------------------------------------------------------------------
/**
*/
program Resolve[string Mask = "Resolve"; ]
{
    ComputeShader = main();
};
