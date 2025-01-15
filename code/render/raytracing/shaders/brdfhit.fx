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
    [ray_payload] in LightResponsePayload Result,
    [hit_attribute] in vec2 baryCoords
)
{
    Object obj = Objects[gl_InstanceCustomIndexEXT];
    vec3 barycentricCoords = vec3(1.0f - baryCoords.x - baryCoords.y, baryCoords.x, baryCoords.y);

    uvec3 indices;
    vec2 uv;
    vec3 normal, tangent;
    float sign;
    SampleGeometry(obj, gl_PrimitiveID, barycentricCoords, indices, uv, normal, tangent, sign);
    
    normal = gl_ObjectToWorldEXT * vec4(normal, 0);
    tangent = gl_ObjectToWorldEXT * vec4(tangent, 0);
    mat3 tbn = TangentSpace(tangent, normal, sign);
    
    BRDFMaterial mat = BRDFMaterials + obj.MaterialOffset;
    vec4 normals = sample2DLod(mat.NormalMap, NormalSampler, uv, 0);
    vec3 tNormal = TangentSpaceNormal(normals.xy, tbn);
    
    float facing = dot(normal, gl_WorldRayDirectionEXT);
    Result.bits |= facing > 0 ? RAY_BACK_FACE_BIT : 0x0; 
        
    vec4 albedo = sample2DLod(mat.AlbedoMap, Basic2DSampler, uv, 0);
    vec4 material = sample2DLod(mat.ParameterMap, Basic2DSampler, uv, 0);
    
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
    RayClosestHitShader = ClosestHit();
};
