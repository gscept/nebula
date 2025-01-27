//------------------------------------------------------------------------------
//  raytracetest.fxh
//  (C) 2023 gscept
//------------------------------------------------------------------------------
#include <lib/raytracing.fxh>
#include <lib/mie-rayleigh.fxh>
#include <lib/pbr.fxh>

group(SYSTEM_GROUP) write rgba16f image2D RaytracingOutput;

//------------------------------------------------------------------------------
/**
*/
shader void 
Raygen(
    [ray_payload] out LightResponsePayload Result
)
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = InvView * vec4(0, 0, 0, 1);
    vec4 target = InvProjection * vec4(d.x, d.y, 1, 1);
    vec4 direction = InvView * vec4(normalize(target.xyz), 0);

    Result.bits = 0x0;
    Result.albedo = vec3(0);
    Result.material = vec4(0);
    Result.radiance = vec3(0);
    Result.alpha = 0.0f;
    Result.normal = vec3(0);
    Result.depth = 0;
    // Ray trace against BVH
    traceRayEXT(TLAS, gl_RayFlagsCullBackFacingTrianglesEXT, 0xFF, 0, 0, 0, origin.xyz, 0.01f, direction.xyz, 10000.0f, 0);

    vec3 light = vec3(0);
    if ((Result.bits & RAY_MISS_BIT) != 0)
    {
        vec3 dir = normalize(direction.xyz);
        light += CalculateAtmosphericScattering(dir, GlobalLightDirWorldspace.xyz) * GlobalLightColor.rgb;
    }
    else
    {
    
        //light += (Result.normal + 1.0f) * 0.5f;
        light += Result.radiance;
    }

    imageStore(RaytracingOutput, ivec2(gl_LaunchIDEXT.xy), vec4(light, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in HitResult Result
)
{
    Result.bits |= RAY_MISS_BIT;
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "test";]
{
    RayGenerationShader = Raygen();
    RayMissShader = Miss();
};
