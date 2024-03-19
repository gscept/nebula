//------------------------------------------------------------------------------
//  blur_2d_rgba16f_cs.fxh
//
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_IS_RGBA16F 1
#include "blur_cs.fxh"

//------------------------------------------------------------------------------
/**
*/
program BlurX [ string Mask = "BlurX"; ]
{
    ComputeShader = csMainX();
};

program BlurY [ string Mask = "BlurY"; ]
{
    ComputeShader = csMainY();
};