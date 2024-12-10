//------------------------------------------------------------------------------
//  @file brdfhit.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/raytracing.fxh>

//------------------------------------------------------------------------------
/**
*/
shader void
ClosestHit(
    [ray_payload] in HitResult Result,
    [hit_attribute] in vec2 baryCoords
)
{
    Object obj = Objects[gl_InstanceCustomIndexEXT];
    vec3 barycentricCoords = vec3(1.0f - baryCoords.x - baryCoords.y, baryCoords.x, baryCoords.y);

    uvec3 indices;
    vec2 uv;
    mat3 tbn;
    SampleGeometry(obj, gl_PrimitiveID, barycentricCoords, indices, uv, tbn);

    BRDFMaterial mat = BRDFMaterials + obj.MaterialOffset;
    vec4 normals = sample2DLod(mat.NormalMap, Basic2DSampler, uv, 0);
    vec3 tNormal = TangentSpaceNormal(normals.xy, tbn);
    
    float facing = dot(tNormal, gl_WorldRayDirectionEXT);
    Result.bits |= facing <= 0 ? RAY_BACK_FACE_BIT : 0x0; 
        
    vec4 albedo = sample2DLod(mat.AlbedoMap, Basic2DSampler, uv, 0);
    vec4 material = sample2DLod(mat.ParameterMap, Basic2DSampler, uv, 0);
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
    RayClosestHitShader = ClosestHit();
};
