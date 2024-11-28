//------------------------------------------------------------------------------
//  @file probeupdate.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include "ddgi.fxh"

group(SYSTEM_GROUP) write rgb10a2 image2D RadianceOutput;
group(SYSTEM_GROUP) write rgba8 image2D NormalOutput;
group(SYSTEM_GROUP) write rg32f image2D DepthOutput;

const uint NumColorSamples = 16;
const uint NumDepthSamples = 8;

struct Probe
{
    vec3 position;
    mat3 rotation;
};

group(SYSTEM_GROUP) rw_buffer ProbeBuffer
{
    vec3 scale;
    vec3 offset;
    Probe Probes[];
};

group(SYSTEM_GROUP) constant SampleDirections
{
    vec3 Directions[24];
};

//------------------------------------------------------------------------------
/**
*/
ivec2
GetOutputPixel(uvec2 rayIndex, uint numSamples)
{
    uint row = rayIndex.y / numSamples;
    uint column = (rayIndex.y % numSamples) * numSamples + rayIndex.x;
    return ivec2(row, column); 
}

//------------------------------------------------------------------------------
/**
*/
shader void
RayGen(
    [ray_payload] out HitResult payload
)
{
    //payload.radiance = vec3(0);
    payload.normal = vec3(0, 1, 0);

    Probe probe = Probes[gl_LaunchIDEXT.x];
    vec3 direction = Directions[gl_LaunchIDEXT.y] * probe.rotation;

    const float MaxDistance = 10000.0f;

    traceRayEXT(TLAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, probe.position.xyz, 0.01f, direction, MaxDistance, 0);
    if (payload.miss == 0)
    {
        // If launch ID is below 16, it's a color sample
        if (gl_LaunchIDEXT.y < NumColorSamples)
        {
            vec3 WorldSpacePos = probe.position.xyz + direction.xyz * payload.depth;
            ivec2 pixel = GetOutputPixel(gl_LaunchIDEXT.xy, NumColorSamples);
            vec3 light = CalculateLightRT(WorldSpacePos, payload.depth / 10000.0f, payload.albedo.rgb, payload.material, payload.normal);
            imageStore(RadianceOutput, pixel, vec4(light, 0));
        }
        else
        {
            ivec2 pixel = GetOutputPixel(gl_LaunchIDEXT.xy, NumDepthSamples);
            imageStore(DepthOutput, pixel, vec4(payload.depth / MaxDistance));
        }
    }
    else
    {
        vec3 lightDir = normalize(GlobalLightDirWorldspace.xyz);
        vec3 dir = normalize(direction);
        vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
        ivec2 pixel = GetOutputPixel(gl_LaunchIDEXT.xy, NumColorSamples);
        imageStore(RadianceOutput, pixel, vec4(atmo, 0));
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
    payload.miss = 1;
}


//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "ProbeRayGen"; ]
{
    RayGenerationShader = RayGen();
    RayMissShader = Miss();
};
