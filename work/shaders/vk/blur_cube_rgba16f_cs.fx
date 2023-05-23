//------------------------------------------------------------------------------
//  blur_cube_rgba16f_cs.fxh
//
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define IMAGE_IS_RGBA16F 1
#define IMAGE_IS_ARRAY 1
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