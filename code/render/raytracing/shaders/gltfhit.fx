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

    GLTFMaterial mat = GLTFMaterials + obj.MaterialOffset;

    /* Tangent space normal transform */
    vec4 normals = sample2DLod(mat.normalTexture, Basic2DSampler, uv, 0);
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
    material[MAT_EMISSIVE] = 0.0f;

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
