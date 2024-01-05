//------------------------------------------------------------------------------
//  raytracing.fxh
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#define MESH_BINDING group(BATCH_GROUP) binding(50)

// Declare type for vertex positions and uv
ptr struct VertexPosUv
{
    vec3 position;
    uint uv;
};

ptr struct VertexAttributeNormals
{
    uvec2 normal_tangent;
};

ptr struct VertexAttributeSecondaryUv
{
    uint uv;
};

ptr struct VertexAttributeColor
{
    uint color;
};

ptr struct VertexAttributeSkin
{
    vec4 weights;
    uint indices;
};