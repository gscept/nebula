//------------------------------------------------------------------------------
//  @file probeupdate.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include "ddgi.fxh"

group(BATCH_GROUP) accelerationStructure TLAS;
group(BATCH_GROUP) write rgba8 image2D RadianceOutput;
group(BATCH_GROUP) write rgba8 image2D NormalOutput;
group(BATCH_GROUP) write r32f image2D DepthOutput;

struct Probe
{
    vec3 position;
    mat3 rotation;
};

group(BATCH_GROUP) rw_buffer ProbeBuffer
{
    Probe Probes[];
};

group(BATCH_GROUP) constant SampleDirections
{
    vec3 Directions[24];
};

//------------------------------------------------------------------------------
/**
*/
shader void
RayGen(
    [ray_payload] out HitResult payload
)
{
    payload.radiance = vec3(0);
    payload.normal = vec3(0, 1, 0);

    Probe probe = Probes[gl_LaunchIDEXT.x];
    vec3 direction = Directions[gl_LaunchIDEXT.y] * probe.rotation;

    const float MaxDistance = 10000.0f;
    const uint NumColorSamples = 16;
    const uint NumDepthSamples = 8;

    traceRayEXT(TLAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, probe.position.xyz, 0.01f, direction, MaxDistance, 0);

    // If launch ID is below 16, it's a color sample
    if (gl_LaunchIDEXT.y < NumColorSamples)
    {
        uint row = gl_LaunchIDEXT.y / NumColorSamples;
        uint column = (gl_LaunchIDEXT.y % NumColorSamples) * NumColorSamples + gl_LaunchIDEXT.x;
        imageStore(RadianceOutput, ivec2(row, column), vec4(payload.radiance, 0));
        imageStore(NormalOutput, ivec2(row, column), vec4(payload.normal, 0));
    }
    else
    {
        uint row = gl_LaunchIDEXT.y / NumDepthSamples;
        uint column = (gl_LaunchIDEXT.y % NumDepthSamples) * NumDepthSamples + gl_LaunchIDEXT.x;
        imageStore(DepthOutput, ivec2(row, column), vec4(payload.depth / MaxDistance));
    }

    // TODO use octahedral mapping to output the probe pixels here
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in HitResult payload
)
{
    vec3 lightDir = normalize(GlobalLightDirWorldspace.xyz);
    vec3 dir = normalize(gl_WorldRayDirectionEXT);
    vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;

    payload.radiance = atmo;
    payload.normal = -gl_WorldRayDirectionEXT;
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "ProbeRayGen"; ]
{
    RayGenerationShader = RayGen();
    RayMissShader = Miss();
};
