//------------------------------------------------------------------------------
//  raytracetest.fxh
//  (C) 2023 gscept
//------------------------------------------------------------------------------
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include <lib/pbr.fxh>


//------------------------------------------------------------------------------
/**
*/
shader void 
Raygen(
    [ray_payload] out HitResult Result
)
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = InvView * vec4(0, 0, 0, 1);
    vec4 target = InvProjection * vec4(d.x, d.y, 1, 1);
    vec4 direction = InvView * vec4(normalize(target.xyz), 0);

    Result.albedo = vec3(0, 0, 0);
    Result.alpha = 0.0f;
    Result.material = vec4(0, 0, 0, 0);
    Result.normal = vec3(0, 0, 0);
    Result.depth = 0;
    Result.miss = 0;

    // Ray trace against BVH
    traceRayEXT(TLAS, gl_RayFlagsNoneEXT, 0xFF, 0, 0, 0, origin.xyz, 0.01f, direction.xyz, 10000.0f, 0);

    vec3 F0 = CalculateF0(Result.albedo.rgb, Result.material[MAT_METALLIC], vec3(0.04));
    vec3 WorldSpacePos = origin.xyz + direction.xyz * Result.depth;
    vec3 light = vec3(0);
    if (Result.miss == 1)
    {
        vec3 dir = normalize(direction.xyz);
        light += CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
    }
    else
    {
        light += CalculateLightRT(WorldSpacePos, Result.depth / 10000.0f, Result.albedo.rgb, Result.material, Result.normal);
    }
    //light += CalculateGlobalLight(Result.albedo, Result.material, F0, -normalize(target.xyz), Result.normal, WorldSpacePos);
    //vec3 dir = normalize(Result.normal);
    //vec3 atmo = CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;

    imageStore(RaytracingOutput, ivec2(gl_LaunchIDEXT.xy), vec4(light / 10.0f, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in HitResult Result
)
{
    Result.miss = 1;
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "test";]
{
    RayGenerationShader = Raygen();
    RayMissShader = Miss();
};
