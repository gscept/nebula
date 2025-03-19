//------------------------------------------------------------------------------
//  downsample_cs_light.fx
//  Downsample compute shader for light buffer
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define FORMAT r11g11b10f
#define IMAGE_DATA_TYPE vec3
#define IMAGE_DATA_SWIZZLE xyz
#define IMAGE_DATA_EXPAND xyzx
#include "downsample_cs.fxh" 