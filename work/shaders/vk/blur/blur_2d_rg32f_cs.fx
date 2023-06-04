//------------------------------------------------------------------------------
//  blur_2d_rg32f_cs.fxh
//
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_IS_RG32F 1
#include "blur_cs.fxh"

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