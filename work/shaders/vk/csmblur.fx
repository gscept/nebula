//------------------------------------------------------------------------------
//  csmblur.fx
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_IS_RG32F 1
#define BLUR_KERNEL_8 1
#define IMAGE_IS_ARRAY 1
#include "lib/blur_cs.fxh"

//------------------------------------------------------------------------------
/**
*/
program BlurX[string Mask = "Alt0";]
{
    ComputeShader = csMainX();
};

program BlurY[string Mask = "Alt1";]
{
    ComputeShader = csMainY();
}; 