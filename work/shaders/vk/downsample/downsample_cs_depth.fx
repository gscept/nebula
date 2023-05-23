//------------------------------------------------------------------------------
//  downsample_cs_depth.fx
//  Downsample compute shader for light buffer
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define FORMAT r32f
#define IMAGE_DATA_TYPE float
#define IMAGE_DATA_SWIZZLE x
#define IMAGE_DATA_EXPAND xxxx
#include "downsample_cs.fxh"