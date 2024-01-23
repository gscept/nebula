//------------------------------------------------------------------------------
//  @file brdfhit.fx
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/materials.fxh>
#include <lib/raytracing.fxh>
#include <lib/shared.fxh>

struct Object
{
    BRDFMaterial MaterialPtr;
    VertexPosUv PositionsPtr;
    VertexAttributeNormals AttributePtr;
};

MATERIAL_BINDING rw_buffer ObjectBuffer
{
    Object Objects[];
};

//------------------------------------------------------------------------------
/**
*/
shader void
ClosestHit(
    [ray_payload] in HitResult Result,
    [hit_attribute] in vec2 baryCoords
)
{
    Object obj = Objects[gl_InstanceID];
    vec2 uv0 = UnpackUV32((obj.PositionsPtr + gl_PrimitiveID).uv);
    vec2 uv1 = UnpackUV32((obj.PositionsPtr + gl_PrimitiveID + 1).uv);
    vec2 uv2 = UnpackUV32((obj.PositionsPtr + gl_PrimitiveID + 2).uv);
    vec2 uv = BaryCentricVec2(uv0, uv1, uv2, baryCoords);

    vec3 n1 = UnpackNormal32((obj.AttributePtr + gl_PrimitiveID).normal_tangent.x);
    vec3 n2 = UnpackNormal32((obj.AttributePtr + gl_PrimitiveID + 1).normal_tangent.x);
    vec3 n3 = UnpackNormal32((obj.AttributePtr + gl_PrimitiveID + 2).normal_tangent.x);
    vec3 norm = BaryCentricVec3(n1, n2, n3, baryCoords);

    vec4 albedo = sample2DLod(obj.MaterialPtr.AlbedoMap, Basic2DSampler, uv, 0);
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
