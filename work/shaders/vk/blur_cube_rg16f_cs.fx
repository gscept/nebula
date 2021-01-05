//------------------------------------------------------------------------------
//  blur_cube_rg16f_cs.fxh
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#define IMAGE_IS_RG16F 1
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