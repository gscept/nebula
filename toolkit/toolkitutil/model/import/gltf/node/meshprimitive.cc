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

    return (MeshBuilderVertex::Components)0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE, int n>
Math::vec4
ReadVertexData(void const* const buffer, const uint i)
{
    static_assert(n >= 1 && n <= 4, "You're doing it wrong!");
    TYPE const* const v = (TYPE*)(buffer);
    Math::vec4 ret(0.0f);
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
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4
ReadVertexData(CoreGraphics::VertexComponent::Format format, void const* const buffer, uint32_t index)
{
    Math::vec4 data;
    switch (format)
    {
        //clang-format off
        case CoreGraphics::VertexComponent::Format::Float:    data = ReadVertexData<float, 1>(buffer, index); break;
        case CoreGraphics::VertexComponent::Format::Float2:   data = ReadVertexData<float, 2>(buffer, index); break;
        case CoreGraphics::VertexComponent::Format::Float3:   data = ReadVertexData<float, 3>(buffer, index); break;
        case CoreGraphics::VertexComponent::Format::Float4:   data = ReadVertexData<float, 4>(buffer, index); break;
        case CoreGraphics::VertexComponent::Format::UShort4:  data = ReadVertexData<ushort, 4>(buffer, index); break;
        case CoreGraphics::VertexComponent::Format::UByte4:   data = ReadVertexData<ubyte, 4>(buffer, index); break;
        default:
            n_error("ERROR: Invalid vertex component type!");
            break;
        //clang-format on
    }
    return data;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshPrimitiveFunc(SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
{
    MeshPrimitiveJobContext* context = static_cast<MeshPrimitiveJobContext*>(ctx);
    for (uint i = 0; i < groupSize; i++)
    {
        IndexT index = i + invocationOffset;
        if (index >= totalJobs)
            return;

        MeshBuilderVertex::ComponentMask components = 0x0;
        MeshBuilder* meshBuilder = context->outMeshes[index];
        const Gltf::Primitive* primitive = context->primitives[index];
        SceneNode* node = context->outSceneNodes[index];


        if (primitive->material == -1)
        {
            node->mesh.material = "syssur:gltf_default.sur";
        }
        else
        {
            Gltf::Material& material = context->scene->materials[primitive->material];
            if (!material.name.IsEmpty())
            {
                node->mesh.material = material.name;
            }
            else
            {
                node->mesh.material.Format("unnamed_%d", primitive->material);
            }
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
        uint const vertCount = context->scene->accessors[primitive->attributes[Gltf::Primitive::Attribute::Position]].count;
        uint const triCount = primitive->indices == -1 ? vertCount / 3 : context->scene->accessors[primitive->indices].count / 3;

        meshBuilder->NewMesh(vertCount, triCount);

        for (uint j = 0; j < vertCount; j++)
        {
            meshBuilder->AddVertex(MeshBuilderVertex());
        }

        // Extract vertex data
        for (auto const& attribute : primitive->attributes)
        {
            attributeFlags.SetBit((IndexT)attribute.Key());

            auto const& accessorIndex = attribute.Value();
            Gltf::Accessor const& vertexBufferAccessor = context->scene->accessors[accessorIndex];
            const uint count = vertexBufferAccessor.count;

            Gltf::BufferView const& vertexBufferView = context->scene->bufferViews[vertexBufferAccessor.bufferView];
            Gltf::Buffer const& buffer = context->scene->buffers[vertexBufferView.buffer];
            const uint bufferOffset = vertexBufferAccessor.byteOffset + vertexBufferView.byteOffset;

            MeshBuilderVertex::Components component = AttributeToComponentIndex(attribute.Key(), vertexBufferAccessor.normalized);

            // If component is invalid, skip it
            if (component == 0xFFFFFFFF)
                continue;

            components |= component;
            void* vb = (byte*)buffer.data.GetPtr() + bufferOffset;

            for (uint j = 0; j < count; j++)
            {
                Math::vec4 data = ReadVertexData(vertexBufferAccessor.format, vb, j);
                
                MeshBuilderVertex& vtx = meshBuilder->VertexAt(j);
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
                        vtx.SetSign(data.w);
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

            if (vertexBufferAccessor.sparse.count > 0)
            {
                // substitute the vertex data from the sparse buffer

                Gltf::BufferView const& sparseIndexBufferView =
                    context->scene->bufferViews[vertexBufferAccessor.sparse.indices.bufferView];
                Util::Blob const& sparseIndices = context->scene->buffers[sparseIndexBufferView.buffer].data;
                int const sparseIndexBufferOffset = vertexBufferAccessor.sparse.indices.byteOffset + sparseIndexBufferView.byteOffset;

                byte const* const ibuf = (byte*)sparseIndices.GetPtr() + sparseIndexBufferOffset;
                const uint count = vertexBufferAccessor.sparse.count;

                Gltf::BufferView const& sparseVertexBufferView =
                    context->scene->bufferViews[vertexBufferAccessor.sparse.values.bufferView];
                Util::Blob const& sparseVertices = context->scene->buffers[sparseVertexBufferView.buffer].data;
                int const sparseVertexBufferOffset =
                    vertexBufferAccessor.sparse.values.byteOffset + sparseVertexBufferView.byteOffset;

                void const* const vbuf = (void*)((byte*)sparseIndices.GetPtr() + sparseVertexBufferOffset);
                
                uint64_t indexByteStride;
                uint32_t indexMask;

                switch (vertexBufferAccessor.sparse.indices.componentType)
                {
                    case Gltf::Accessor::ComponentType::UnsignedByte:   indexByteStride = 1; indexMask = 0x000000FF; break;
                    case Gltf::Accessor::ComponentType::UnsignedShort:  indexByteStride = 2; indexMask = 0x0000FFFF; break;
                    case Gltf::Accessor::ComponentType::UnsignedInt:    indexByteStride = 4; indexMask = 0xFFFFFFFF; break;
                    default:
                        n_error("ERROR: Invalid index type!");
                        break;
                }

                for (size_t j = 0; j < count; j++)
                {
                    Math::vec4 data = ReadVertexData(vertexBufferAccessor.format, vbuf, j);
                    uint32_t idx = *((uint32_t*)(ibuf + (j * indexByteStride)));
                    idx &= indexMask;
                    MeshBuilderVertex& vtx = meshBuilder->VertexAt(idx);
                    vtx.SetPosition(data);
                }
            }

        }

        // Set components on the mesh
        meshBuilder->SetComponents(components);

        // Compute bounding box
        node->base.boundingBox = meshBuilder->ComputeBoundingBox();

        // Setup triangles
        if (primitive->indices == -1)
        {
            // TODO: Mesh builder needs to support meshes without indexbuffer.
            for (size_t i = 0; i < triCount * 3; i += 3)
            {
                MeshBuilderTriangle triangle;
                triangle.SetVertexIndices(i, i + 1, i + 2);
                meshBuilder->AddTriangle(triangle);
            }
        }
        else // Read indexbuffer from GLTF
        {
            Gltf::Accessor const& indexBufferAccessor = context->scene->accessors[primitive->indices];
            Gltf::BufferView const& indexBufferView = context->scene->bufferViews[indexBufferAccessor.bufferView];
            Gltf::Buffer const& buffer = context->scene->buffers[indexBufferView.buffer];
            Util::Blob const& indexBuffer = buffer.data;
            const uint bufferOffset = indexBufferAccessor.byteOffset + indexBufferView.byteOffset;

            // TODO: sparse accessors
            n_assert2(indexBufferAccessor.sparse.count == 0, "Sparse accessors not yet supported!");

            switch (indexBufferAccessor.componentType)
            {
            case Gltf::Accessor::ComponentType::UnsignedByte:
                SetupIndexBuffer<uchar>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0);
                break;
            case Gltf::Accessor::ComponentType::UnsignedShort:
                SetupIndexBuffer<ushort>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0);
                break;
            case Gltf::Accessor::ComponentType::UnsignedInt:
                SetupIndexBuffer<uint>(*meshBuilder, indexBuffer, bufferOffset, indexBufferAccessor, 0);
                break;
            default:
                n_error("ERROR: Invalid vertex index type!");
                break;
            }
        }
        
        // We currently don't support meshes without normals and tangents.
        if (!attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Normal>())
        {
            //if (AllBits(context->flags, ExportFlags::CalcNormals))
            {
                meshBuilder->CalculateNormals();
            }
        }
        if (!attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Tangent>())
        {
            //if (AllBits(context->flags, ExportFlags::CalcTangents))
            {
                meshBuilder->CalculateTangents();
            }
        }

        if (!AllBits(context->flags, ToolkitUtil::FlipUVs))
        {
            meshBuilder->FlipUvs();
        }
    }
}

} // namespace ToolkitUtil