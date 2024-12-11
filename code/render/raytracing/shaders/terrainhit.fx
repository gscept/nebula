//------------------------------------------------------------------------------
//  @file brdfhit.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/raytracing.fxh>

//------------------------------------------------------------------------------
/**
*/
shader void
BoxHit(
    [ray_payload] in HitResult Result,
    [hit_attribute] in vec2 baryCoords
)
{
    Object obj = Objects[gl_InstanceCustomIndexEXT];
    vec3 barycentricCoords = vec3(1.0f - baryCoords.x - baryCoords.y, baryCoords.x, baryCoords.y);

    uvec3 indices;
    vec2 uv;
    mat3 tbn;
    TerrainMaterial mat = TerrainMaterials[obj.MaterialOffset];
    SampleTerrain(obj, gl_PrimitiveID, barycentricCoords, indices, uv, tbn);

    vec4 normals = sample2DLod(mat.LowresNormalFallback, NormalSampler, uv, 0);
    vec3 tNormal = TangentSpaceNormal(normals.xy, tbn);

    vec4 albedo = sample2DLod(mat.LowresAlbedoFallback, Basic2DSampler, uv, 0);
    vec4 material = sample2DLod(mat.LowresMaterialFallback, Basic2DSampler, uv, 0);
    Result.alpha = albedo.a;
    Result.albedo = albedo.rgb;
    Result.material = material;
    Result.normal = tNormal;
    Result.depth = gl_HitTEXT;
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "Hit"; ]
{
    RayClosestHitShader = BoxHit();
};
