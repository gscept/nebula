//------------------------------------------------------------------------------
//  raytracetest.fxh
//  (C) 2023 gscept
//------------------------------------------------------------------------------
#include <lib/shared.fxh>

group(BATCH_GROUP) accelerationStructure TLAS;
group(BATCH_GROUP) write rgba16f image2D Output;

//------------------------------------------------------------------------------
/**
*/
shader void 
Raygen(
    [ray_payload] out vec3 hitValue
)
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = InvView * vec4(0, 0, 0, 1);
    vec4 target = InvProjection * vec4(d.x, d.y, 1, 1);
    vec4 direction = InvView * vec4(normalize(target.xyz), 0);

    vec3 dir = vec3(0, 0, 1);
    hitValue = vec3(0, 0, 0);

    // Ray trace against BVH
    traceRayEXT(TLAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin.xyz, 0.01f, direction.xyz, 10000.0f, 0);

    imageStore(Output, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0f));
}

//------------------------------------------------------------------------------
/**
*/
shader void
SphereIntersect(
)
{
    reportIntersectionEXT(0.5f, 0);
}

//------------------------------------------------------------------------------
/**
*/
shader void
ClosestHit(
    [ray_payload] in vec3 color
)
{
    color = vec3(1, 0, 0);
}

//------------------------------------------------------------------------------
/**
*/
shader void
AnyHit(
    [ray_payload] in vec3 color
)
{
    color = vec3(0, 1, 0);
}

//------------------------------------------------------------------------------
/**
*/
shader void
Miss(
    [ray_payload] in vec3 color
)
{
    color = vec3(0, 0, 1);
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "test";]
{
    RayGenerationShader = Raygen();
    RayClosestHitShader = ClosestHit();
    RayAnyHitShader = AnyHit();
    RayMissShader = Miss();
};