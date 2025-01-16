//------------------------------------------------------------------------------
//  @file brdfhit.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/raytracing.fxh>
#include <lib/pbr.fxh>

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
    
    float facing = dot(normal, gl_WorldRayDirectionEXT);
    Result.bits |= facing > 0 ? RAY_BACK_FACE_BIT : 0x0; 

    GLTFMaterial mat = GLTFMaterials + obj.MaterialOffset;

    /* Tangent space normal transform */
    vec4 normals = sample2DLod(mat.normalTexture, NormalSampler, uv, 0);
    vec3 tNormal = normalize(TangentSpaceNormal(normals.xy, tbn));

    /* Surface properties */
    vec4 albedo = sample2DLod(mat.baseColorTexture, Basic2DSampler, uv, 0);
    vec4 metallicRoughness = sample2DLod(mat.metallicRoughnessTexture, Basic2DSampler, uv, 0) * vec4(1.0f, mat.roughnessFactor, mat.metallicFactor, 1.0f);
    vec4 emissive = sample2DLod(mat.emissiveTexture, Basic2DSampler, uv, 0) * mat.emissiveFactor;
    vec4 occlusion = sample2DLod(mat.occlusionTexture, Basic2DSampler, uv, 0);

    /* Convert to material */
    vec4 material;
    material[MAT_METALLIC] = metallicRoughness.b;
    material[MAT_ROUGHNESS] = metallicRoughness.g;
    material[MAT_CAVITY] = Greyscale(occlusion);
    material[MAT_EMISSIVE] = 0; // Emissive isn't used by the light RT

    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 radiance = CalculateLightRT(worldPos, EyePos.xyz, gl_HitTEXT / gl_RayTmaxEXT, albedo.rgb, material, tNormal) + emissive.xyz * albedo.a;
        
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
