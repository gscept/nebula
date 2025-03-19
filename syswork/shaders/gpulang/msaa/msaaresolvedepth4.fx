//------------------------------------------------------------------------------
//  msaaresolvedepth4.fx
//  (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_TYPE_R32F
#define METHOD_SAMPLE_ZERO
#define SAMPLES 4
#include "msaaresolve.fxh" 

//------------------------------------------------------------------------------
/**
*/
program Resolve[string Mask = "Resolve";]
{
    ComputeShader = main();
};
