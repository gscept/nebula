//------------------------------------------------------------------------------
//  blur_bloom.fxh
//
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_IS_RGB16F 1
#define BLUR_KERNEL_64 1
#include "lib/blur_cs.fxh"

//------------------------------------------------------------------------------
/**
*/
program BlurX [ string Mask = "Alt0"; ]
{
    ComputeShader = csMainX();
};

program BlurY [ string Mask = "Alt1"; ]
{
    ComputeShader = csMainY();
};