//------------------------------------------------------------------------------
//  @file probefinalize.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include "ddgi.fxh"


groupshared vec3 Radiance[1024];
groupshared float Distance[1024];

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = NUM_IRRADIANCE_TEXELS_PER_PROBE
[local_size_y] = NUM_IRRADIANCE_TEXELS_PER_PROBE
[local_size_z] = 1
shader void
ProbeFinalizeRadiance()
{
}

//------------------------------------------------------------------------------
/**
*/
[local_size_x] = NUM_DISTANCE_TEXELS_PER_PROBE
[local_size_y] = NUM_DISTANCE_TEXELS_PER_PROBE
[local_size_z] = 1
shader void
ProbeFinalizeDistance()
{
}

//------------------------------------------------------------------------------
/**
*/
program RadianceFinalize[string Mask = "ProbeFinalizeRadiance"; ]
{
    ComputeShader = ProbeFinalizeRadiance();
};

//------------------------------------------------------------------------------
/**
*/
program DistanceFinalize[string Mask = "ProbeFinalizeDistance"; ]
{
    ComputeShader = ProbeFinalizeDistance();
};
