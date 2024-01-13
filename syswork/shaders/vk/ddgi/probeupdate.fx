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
group(BATCH_GROUP) write rgba8 image2D AlbedoOutput;
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

//------------------------------------------------------------------------------
/**
*/
shader void
RayGen(
    [ray_payload] out HitResult payload
)
{
    payload.albedo = vec3(0);
    payload.normal = vec3(0, 1, 0);

    Probe probe = Probes[gl_LaunchIDEXT.x];

    vec3 direction = vec3(0, 0, 1) * probe.rotation;

    traceRayEXT(TLAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, probe.position.xyz, 0.01f, direction, 10000.0f, 0);

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

    payload.albedo = atmo;
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