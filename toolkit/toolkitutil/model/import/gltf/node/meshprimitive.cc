//------------------------------------------------------------------------------
//  meshprimitive.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshprimitive.h"
#include "util/bitfield.h"
#include "util/guid.h"
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
    The "enable_if" template magic just enables a linting and compile error if we try to use an index buffer type other than an integral type
*/
template <typename INDEXTYPE, std::enable_if_t<std::is_integral<INDEXTYPE>::value, int> = 0>
void SetupIndexBuffer(MeshBuilder& meshBuilder, Util::Blob const& indexBuffer, const uint offset, Gltf::Accessor const& indexBufferAccessor, const uint primitiveGroupId)
{
    INDEXTYPE const* const ib = (INDEXTYPE*)((byte*)indexBuffer.GetPtr() + offset);
    const uint count = indexBufferAccessor.count;
    n_assert2(count % 3 == 0, "Index buffer size is not divisible by 3!");

    // HACK: assume this is a triangle list
    for (uint i = 0; i < count; i += 3)
    {
        MeshBuilderTriangle triangle;
        triangle.SetVertexIndices((IndexT)ib[i], (IndexT)ib[i + 1], (IndexT)ib[i + 2]);

#if NEBULA_VALIDATE_GLTF
        n_assert(ib[i] <= indexBufferAccessor.max[0] && ib[i] >= indexBufferAccessor.min[0])
            n_assert(ib[i + 1] <= indexBufferAccessor.max[0] && ib[i + 1] >= indexBufferAccessor.min[0])
            n_assert(ib[i + 2] <= indexBufferAccessor.max[0] && ib[i + 2] >= indexBufferAccessor.min[0])
#endif

            meshBuilder.AddTriangle(triangle);
    };
}

//------------------------------------------------------------------------------
/**
*/
MeshBuilderVertex::Components
AttributeToComponentIndex(Gltf::Primitive::Attribute attribute, bool normalized)
{
    if (!normalized)
    {
        switch (attribute)
        {
        case Gltf::Primitive::Attribute::Position:
            return MeshBuilderVertex::Components::Position;
        case Gltf::Primitive::Attribute::Normal:
            return MeshBuilderVertex::Components::Normals;
        case Gltf::Primitive::Attribute::Tangent:
            return MeshBuilderVertex::Components::Tangents;
        case Gltf::Primitive::Attribute::TexCoord0:
            return MeshBuilderVertex::Components::Uvs;
        case Gltf::Primitive::Attribute::TexCoord1:
            return MeshBuilderVertex::Components::SecondUv;
        case Gltf::Primitive::Attribute::Color0:
            return MeshBuilderVertex::Components::Color;
        case Gltf::Primitive::Attribute::Joints0:
            return MeshBuilderVertex::Components::SkinIndices;
        case Gltf::Primitive::Attribute::Weights0:
            return MeshBuilderVertex::Components::SkinWeights;
        }
    }
   

    n_error("Primitive attribute not yet supported!");
    return MeshBuilderVertex::Components::Position;
}

//------------------------------------------------------------------------------
/**
    The "enable_if" template magic just enables a linting and compile error if we try to use an index buffer type other than an integral type
*/
template <typename TYPE, int n>
Math::vec4
ReadVertexData(void const* buffer, uint const count)
{
    static_assert(n >= 1 && n <= 4, "You're doing it wrong!");
    TYPE const* const v = (TYPE*)(buffer);
    Math::vec4 ret(0.0f);
    for (uint i = 0; i < count; i++)
    {
        if constexpr (n == 1)
        {
            ret = Math::vec4((float)v[i], 0.0f, 0.0f, 0.0f);
        }
        else if constexpr (n == 2)
        {
            uint const offset = i * n;
            ret = Math::vec4((float)v[offset], (float)v[offset + 1], 0.0f, 0.0f);
        }
        else if constexpr (n == 3)
        {
            uint const offset = i * n;
            ret = Math::vec4((float)v[offset], (float)v[offset + 1], (float)v[offset + 2], 0.0f);
        }
        else if constexpr (n == 4)
        {
            uint const offset = i * n;
            ret = Math::vec4((float)v[offset], (float)v[offset + 1], (float)v[offset + 2], (float)v[offset + 3]);
        }
    }
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
SetupPrimitiveGroupJobFunc(Jobs::JobFuncContext const& context)
{
    Gltf::Document const* const scene = (Gltf::Document const*)context.uniforms[0];
    
    for (uint slice = 0; slice < context.numSlices; slice++)
    {
        PrimitiveJobInput const* input = (PrimitiveJobInput const*)context.inputs[0] + slice;
    
        Gltf::Mesh const* const mesh = (Gltf::Mesh const*)input->mesh;
        Gltf::Primitive const* const primitive = (Gltf::Primitive const*)input->primitive;
        ExportFlags const exportFlags = input->exportFlags;

        PrimitiveJobOutput* output = (PrimitiveJobOutput*)context.outputs[0] + slice;
        MeshBuilder* meshBuilder = output->mesh;

        MeshBuilderVertex::ComponentMask components = 0x0;
        
        Gltf::Material const& material = scene->materials[primitive->material];
        output->material = &scene->materials[primitive->material];
        if (!mesh->name.IsEmpty())
        {
            output->name = mesh->name;
        }
        else
        {
            output->name = "unnamed_" + Util::String::FromInt(slice);
        }

        n_assert2(primitive->nebulaMode == CoreGraphics::PrimitiveTopology::Code::TriangleList, "Only triangle lists are supported currently!");

        // TODO: we should support more topology types...
        if (meshBuilder->GetPrimitiveTopology() == CoreGraphics::PrimitiveTopology::Code::InvalidPrimitiveTopology)
        {
            meshBuilder->SetPrimitiveTopology(primitive->nebulaMode);
        }
        n_assert2(meshBuilder->GetPrimitiveTopology() == primitive->nebulaMode, "We currently only support having one primitive topology type per mesh!");

        Util::BitField<64> attributeFlags;

        using namespace Base;

        // Find how many vertices and triangle we need
        uint const vertCount = scene->accessors[primitive->attributes[Gltf::Primitive::Attribute::Position]].count;
        uint const triCount = scene->accessors[primitive->indices].count / 3;

        // Extract vertex data
        for (auto const& attribute : primitive->attributes)
        {
            attributeFlags.SetBit((IndexT)attribute.Key());

            auto const& accessorIndex = attribute.Value();
            Gltf::Accessor const& vertexBufferAccessor = scene->accessors[accessorIndex];
            const uint count = vertexBufferAccessor.count;

            Gltf::BufferView const& vertexBufferView = scene->bufferViews[vertexBufferAccessor.bufferView];
            Gltf::Buffer const& buffer = scene->buffers[vertexBufferView.buffer];
            const uint bufferOffset = vertexBufferAccessor.byteOffset + vertexBufferView.byteOffset;

            // TODO: sparse accessors
            n_assert(vertexBufferAccessor.sparse.count == 0);

            MeshBuilderVertex::Components component = AttributeToComponentIndex(attribute.Key(), vertexBufferAccessor.normalized);

            components |= component;
            void* vb = (byte*)buffer.data.GetPtr() + bufferOffset;

            for (uint i = 0; i < count; i++)
            {
                Math::vec4 data;
                switch (vertexBufferAccessor.format)
                {
                    case CoreGraphics::VertexComponent::Format::Float:    data = ReadVertexData<float, 1>(vb, count); break;
                    case CoreGraphics::VertexComponent::Format::Float2:   data = ReadVertexData<float, 2>(vb, count); break;
                    case CoreGraphics::VertexComponent::Format::Float3:   data = ReadVertexData<float, 3>(vb, count); break;
                    case CoreGraphics::VertexComponent::Format::Float4:   data = ReadVertexData<float, 4>(vb, count); break;
                    case CoreGraphics::VertexComponent::Format::UShort4:  data = ReadVertexData<ushort, 4>(vb, count); break;
                    default:
                        n_error("ERROR: Invalid vertex component type!");
                        break;
                }

                MeshBuilderVertex& vtx = meshBuilder->VertexAt(i);
                switch (component)
                {
                    case MeshBuilderVertex::Components::Position:
                        vtx.SetPosition(data);
                        break;
                    case MeshBuilderVertex::Components::Uvs:
                        vtx.SetUv(Math::vec2(data.x, data.y));
                        break;
                    case MeshBuilderVertex::Components::Normals:
                        vtx.SetNormal(xyz(data));
                        break;
                    case MeshBuilderVertex::Components::Tangents:
                        vtx.SetTangent(xyz(data));
                        break;
                    case MeshBuilderVertex::Components::Color:
                        vtx.SetColor(data);
                        break;
                    case MeshBuilderVertex::Components::SecondUv:
                        vtx.SetSecondaryUv(Math::vec2(data.x, data.y));
                        break;
                    case MeshBuilderVertex::Components::SkinWeights:
                        vtx.SetSkinWeights(data);
                        break;
                    case MeshBuilderVertex::Components::SkinIndices:
                        vtx.SetSkinIndices(Math::uint4{ (uint)data.x, (uint)data.y, (uint)data.z, (uint)data.w });
                        break;
                }
            }

        }

        // Set components on the mesh
        meshBuilder->SetComponents(components);

        // Compute bounding box
        meshBuilder->ComputeBoundingBox(1.0f);

        // Setup triangles
        Gltf::Accessor const& indexBufferAccessor = scene->accessors[primitive->indices];
        Gltf::BufferView const& indexBufferView = scene->bufferViews[indexBufferAccessor.bufferView];
        Gltf::Buffer const& buffer = scene->buffers[indexBufferView.buffer];
        Util::Blob const& indexBuffer = buffer.data;
        const uint bufferOffset = indexBufferAccessor.byteOffset + indexBufferView.byteOffset;

        // TODO: sparse accessors
        n_assert2(indexBufferAccessor.sparse.count == 0, "Sparse accessors not yet supported!");

        switch (indexBufferAccessor.componentType)
        {
        case Gltf::Accessor::ComponentType::UnsignedShort: SetupIndexBuffer<ushort>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0); break;
        case Gltf::Accessor::ComponentType::UnsignedInt: SetupIndexBuffer<uint>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0); break;
        case Gltf::Accessor::ComponentType::UnsignedByte: SetupIndexBuffer<uchar>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0); break;
        default:
            n_error("ERROR: Invalid vertex index type!");
            break;
        }

        if (!(attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Normal>() &&
            attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Tangent>()))
        {
            if (exportFlags & ExportFlags::CalcNormals)
            {
                meshBuilder->CalculateNormals();
            }
            if (exportFlags & ExportFlags::CalcTangents)
            {
                meshBuilder->CalculateTangents();
            }
        }

        if (exportFlags & ToolkitUtil::FlipUVs)
        {
            meshBuilder->FlipUvs();
        }
    }
}
} // namespace ToolkitUtil