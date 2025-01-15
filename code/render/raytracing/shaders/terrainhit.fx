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
    [ray_payload] in LightResponsePayload Result,
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
    
    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 radiance = CalculateLightRT(worldPos, EyePos.xyz, gl_HitTEXT / gl_RayTmaxEXT, albedo.rgb, material, tNormal) + albedo.xyz * material[MAT_EMISSIVE] * albedo.a;
        
    Result.alpha = albedo.a;
    Result.albedo = albedo.rgb;
    Result.radiance = radiance;
    Result.normal = tNormal;
    Result.material = material;
    Result.depth = gl_HitTEXT;
}

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "Hit"; ]
{
    RayClosestHitShader = BoxHit();
};
