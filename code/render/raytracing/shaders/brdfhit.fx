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

    // Sample the index buffer
    uvec3 indexes;
    if (obj.Use16BitIndex == 1)
        indexes = uvec3(obj.IndexPtr[gl_PrimitiveID * 3].index, obj.IndexPtr[gl_PrimitiveID * 3 + 1].index, obj.IndexPtr[gl_PrimitiveID * 3 + 2].index);
    else
    {
        Indexes32 i32Ptr = Indexes32(obj.IndexPtr);
        indexes = uvec3(i32Ptr[gl_PrimitiveID * 3].index, i32Ptr[gl_PrimitiveID * 3 + 1].index, i32Ptr[gl_PrimitiveID * 3 + 2].index);
    }

    vec2 uv0 = UnpackUV32((obj.PositionsPtr + indexes.x).uv);
    vec2 uv1 = UnpackUV32((obj.PositionsPtr + indexes.y).uv);
    vec2 uv2 = UnpackUV32((obj.PositionsPtr + indexes.z).uv);
    vec2 uv = BaryCentricVec2(uv0, uv1, uv2, baryCoords);

    vec3 n1 = UnpackNormal32((obj.AttrPtr + indexes.x).normal_tangent.x);
    vec3 n2 = UnpackNormal32((obj.AttrPtr + indexes.y).normal_tangent.x);
    vec3 n3 = UnpackNormal32((obj.AttrPtr + indexes.z).normal_tangent.x);
    vec3 norm = BaryCentricVec3(n1, n2, n3, baryCoords);

    BRDFMaterial mat = BRDFMaterials + obj.MaterialOffset;
    vec4 albedo = sample2DLod(mat.AlbedoMap, Basic2DSampler, uv, 0);
    Result.radiance = albedo.rgb;
    Result.normal = norm;
    Result.depth = gl_RayTmaxEXT;
}   

//------------------------------------------------------------------------------
/**
*/
program Main[string Mask = "Hit"; ]
{
    RayClosestHitShader = ClosestHit();
};
