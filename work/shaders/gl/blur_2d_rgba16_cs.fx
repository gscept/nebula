//------------------------------------------------------------------------------
//  blur_2d_rgba16f_cs.fxh
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#define IMAGE_IS_RGBA16 1
#define KERNEL_RADIUS 9
#define KERNEL_RADIUS_FLOAT 9.0f
#define BLUR_SHARPNESS 1.0f
#include "lib/blur_2d_cs.fxh"

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