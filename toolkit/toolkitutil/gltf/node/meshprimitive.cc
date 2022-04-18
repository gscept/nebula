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
        triangle.SetGroupId(primitiveGroupId);
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
MeshBuilderVertex::ComponentIndex
AttributeToComponentIndex(Gltf::Primitive::Attribute attribute, bool normalized)
{
    if (!normalized)
    {
        switch (attribute)
        {
        case Gltf::Primitive::Attribute::Position:
            return MeshBuilderVertex::ComponentIndex::CoordIndex;
        case Gltf::Primitive::Attribute::Normal:
            return MeshBuilderVertex::ComponentIndex::NormalIndex;
        case Gltf::Primitive::Attribute::Tangent:
            return MeshBuilderVertex::ComponentIndex::TangentIndex;
        case Gltf::Primitive::Attribute::TexCoord0:
            return MeshBuilderVertex::ComponentIndex::Uv0Index;
        case Gltf::Primitive::Attribute::TexCoord1:
            return MeshBuilderVertex::ComponentIndex::Uv1Index;
        case Gltf::Primitive::Attribute::TexCoord2:
            return MeshBuilderVertex::ComponentIndex::Uv2Index;
        case Gltf::Primitive::Attribute::TexCoord3:
            return MeshBuilderVertex::ComponentIndex::Uv3Index;
        case Gltf::Primitive::Attribute::Color0:
            return MeshBuilderVertex::ComponentIndex::ColorIndex;
        case Gltf::Primitive::Attribute::Joints0:
            return MeshBuilderVertex::ComponentIndex::JIndicesIndex;
        case Gltf::Primitive::Attribute::Weights0:
            return MeshBuilderVertex::ComponentIndex::WeightsIndex;
        }
    }
    else
    {
        switch (attribute)
        {
        case Gltf::Primitive::Attribute::Position:
            return MeshBuilderVertex::ComponentIndex::CoordIndex;
        case Gltf::Primitive::Attribute::Normal:
            return MeshBuilderVertex::ComponentIndex::NormalB4NIndex;
        case Gltf::Primitive::Attribute::Tangent:
            return MeshBuilderVertex::ComponentIndex::TangentB4NIndex;
        case Gltf::Primitive::Attribute::TexCoord0:
            return MeshBuilderVertex::ComponentIndex::Uv0S2Index;
        case Gltf::Primitive::Attribute::TexCoord1:
            return MeshBuilderVertex::ComponentIndex::Uv1S2Index;
        case Gltf::Primitive::Attribute::TexCoord2:
            return MeshBuilderVertex::ComponentIndex::Uv2S2Index;
        case Gltf::Primitive::Attribute::TexCoord3:
            return MeshBuilderVertex::ComponentIndex::Uv3S2Index;
        case Gltf::Primitive::Attribute::Color0:
            return MeshBuilderVertex::ComponentIndex::ColorUB4NIndex;
        case Gltf::Primitive::Attribute::Joints0:
            return MeshBuilderVertex::ComponentIndex::JIndicesUB4Index;
        case Gltf::Primitive::Attribute::Weights0:
            return MeshBuilderVertex::ComponentIndex::WeightsUB4NIndex;
        }
    }

    n_error("Primitive attribute not yet supported!");
    return MeshBuilderVertex::ComponentIndex::InvalidComponentIndex;
}

//------------------------------------------------------------------------------
/**
    The "enable_if" template magic just enables a linting and compile error if we try to use an index buffer type other than an integral type
*/
template <typename TYPE, int n>
void ReadVertexData(MeshBuilder const* meshBuilder, void const* buffer, uint const count, MeshBuilderVertex::ComponentIndex const vertexComponentIndex)
{
    
    TYPE const* const v = (TYPE*)(buffer);
    for (uint i = 0; i < count; i++)
    {
        MeshBuilderVertex& vertex = meshBuilder->VertexAt(i);
        if constexpr (n == 1)
        {
            vertex.SetComponent(vertexComponentIndex, { (float)v[i], 0.0f, 0.0f, 0.0f });
        }
        else if constexpr (n == 2)
        {
            uint const offset = i * n;
            vertex.SetComponent(vertexComponentIndex, { (float)v[offset], (float)v[offset + 1], 0.0f, 0.0f });
        }
        else if constexpr (n == 3)
        {
            uint const offset = i * n;
            vertex.SetComponent(vertexComponentIndex, { (float)v[offset], (float)v[offset + 1], (float)v[offset + 2], 0.0f });
        }
        else if constexpr (n == 4)
        {
            uint const offset = i * n;
            vertex.SetComponent(vertexComponentIndex, { (float)v[offset], (float)v[offset + 1], (float)v[offset + 2], (float)v[offset + 3] });
        }
        else
        {
            static_assert(false, "You're doing it wrong!");
        }
    }
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
        
        Gltf::Material const& material = scene->materials[primitive->material];
        output->material = &scene->materials[primitive->material];
        if (!mesh->name.IsEmpty())
        {
            output->name = mesh->name;
        }
        else
        {
            output->name = "unnamed_" + Util::String::FromInt(input->meshId);
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

        meshBuilder->Reserve(vertCount, triCount);
        
        for (uint i = 0; i < vertCount; i++)
        {
            meshBuilder->AddVertex(MeshBuilderVertex());
        }
    
        {
            Gltf::Accessor const& posAccessor = scene->accessors[primitive->attributes[Gltf::Primitive::Attribute::Position]];
            // extend bounding box
            output->bbox.pmin = Math::point(posAccessor.min[0], posAccessor.min[1], posAccessor.min[2]);
            output->bbox.pmax = Math::point(posAccessor.max[0], posAccessor.max[1], posAccessor.max[2]);
        }

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

            MeshBuilderVertex::ComponentIndex vertexComponentIndex = AttributeToComponentIndex(attribute.Key(), vertexBufferAccessor.normalized);

            void* vb = (byte*)buffer.data.GetPtr() + bufferOffset;

            switch (vertexBufferAccessor.format)
            {
            case VertexComponentBase::Format::Float: ReadVertexData<float, 1>(meshBuilder, vb, count, vertexComponentIndex); break;
            case VertexComponentBase::Format::Float2: ReadVertexData<float, 2>(meshBuilder, vb, count, vertexComponentIndex); break;
            case VertexComponentBase::Format::Float3: ReadVertexData<float, 3>(meshBuilder, vb, count, vertexComponentIndex); break;
            case VertexComponentBase::Format::Float4: ReadVertexData<float, 4>(meshBuilder, vb, count, vertexComponentIndex); break;
            case VertexComponentBase::Format::UShort4: ReadVertexData<ushort, 4>(meshBuilder, vb, count, vertexComponentIndex); break;
            default:
                n_error("ERROR: Invalid vertex component type!");
                break;
            }
        }

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

        if (attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Normal>() &&
            attributeFlags.IsSet<(IndexT)Gltf::Primitive::Attribute::Tangent>())
        {
            // If we have normals and tangents, we need to compute the binormals manually

            for (IndexT i = 0; i < meshBuilder->GetNumVertices(); i++)
            {
                MeshBuilderVertex& vertex = meshBuilder->VertexAt(i);

                Math::vec4 const& normal = vertex.HasComponent(MeshBuilderVertex::ComponentBit::NormalBit) ? vertex.GetComponent(MeshBuilderVertex::ComponentIndex::NormalIndex) : vertex.GetComponent(MeshBuilderVertex::ComponentIndex::NormalB4NIndex);
                Math::vec4 const& tangent = vertex.HasComponent(MeshBuilderVertex::ComponentBit::TangentBit) ? vertex.GetComponent(MeshBuilderVertex::ComponentIndex::TangentIndex) : vertex.GetComponent(MeshBuilderVertex::ComponentIndex::TangentB4NIndex);

                Math::vec4 binormal = normalize3(cross3(normal, tangent) * tangent.w);
                vertex.SetComponent(MeshBuilderVertex::ComponentIndex::BinormalB4NIndex, binormal);
            }
        }
        else
        {
            if (exportFlags & ExportFlags::CalcNormals)
            {
                meshBuilder->CalculateNormals();
            }
            if (exportFlags & ExportFlags::CalcBinormalsAndTangents)
            {
                meshBuilder->CalculateTangentsAndBinormals();
            }
        }

        if (exportFlags & ToolkitUtil::FlipUVs)
        {
            meshBuilder->FlipUvs();
        }
    }
}
} // namespace ToolkitUtil