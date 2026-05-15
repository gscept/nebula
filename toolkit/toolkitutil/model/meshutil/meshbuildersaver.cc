//------------------------------------------------------------------------------
//  meshbuildersaver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshbuildersaver.h"
#include "coregraphics/legacy/nvx2fileformatstructs.h"
#include "coregraphics/nvx3fileformatstructs.h"
#include "io/ioserver.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/mesh.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace CoreGraphics;
using namespace System;
using namespace Math;

#define SWAPINT16ENDIAN(x) ( (((x) & 0xFF) << 8) | ((unsigned short)(x) >> 8) )
#define SWAPINT32ENDIAN(x) ( ((x) << 24) | (( (x) << 8) & 0x00FF0000) | (( (x) >> 8) & 0x0000FF00) | ((x) >> 24) )


//------------------------------------------------------------------------------
/**
*/
bool 
MeshBuilderSaver::SaveImport(const IO::URI& uri, const Util::Array<MeshBuilder*>& meshes, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ToolkitUtil::MeshResourceT meshResource;
        for (auto& builder : meshes)
        {
            auto mesh = std::make_unique<ToolkitUtil::MeshInstanceT>();
            mesh->component_mask = builder->GetComponents();
            mesh->topology = builder->GetPrimitiveTopology();
            mesh->vertices.reserve(builder->vertices.size());

            CoreGraphics::VertexLayoutType layout = MeshBuilderVertex::GetVertexLayoutType(builder->GetComponents());
            for (auto& vertex : builder->vertices)
            {
                auto vertexT = std::make_unique<ToolkitUtil::MeshVertexT>();
                vertexT->position = vertex.base.position;
                vertexT->uv = vertex.base.uv;

                switch (layout)
                {
                    case CoreGraphics::VertexLayoutType::Normal:
                    {
                        auto normalAttributes = std::make_unique<ToolkitUtil::MeshNormalAttributesT>();
                        normalAttributes->normal = vertex.attributes.normal.normal;
                        normalAttributes->tangent = vertex.attributes.normal.tangent;
                        normalAttributes->sign = vertex.attributes.normal.sign;
                        vertexT->normal_attributes = std::move(normalAttributes);
                        break;
                    }
                    case CoreGraphics::VertexLayoutType::Colors:
                    {
                        auto normalAttributes = std::make_unique<ToolkitUtil::MeshNormalAttributesT>();
                        normalAttributes->normal = vertex.attributes.normal.normal;
                        normalAttributes->tangent = vertex.attributes.normal.tangent;
                        normalAttributes->sign = vertex.attributes.normal.sign;

                        auto colorAttributes = std::make_unique<ToolkitUtil::MeshColorAttributesT>();
                        colorAttributes->normals = std::move(normalAttributes);
                        colorAttributes->color = vertex.attributes.color.color;
                        vertexT->color_attributes = std::move(colorAttributes);
                        break;
                    }
                    case CoreGraphics::VertexLayoutType::SecondUV:
                    {
                        auto normalAttributes = std::make_unique<ToolkitUtil::MeshNormalAttributesT>();
                        normalAttributes->normal = vertex.attributes.normal.normal;
                        normalAttributes->tangent = vertex.attributes.normal.tangent;
                        normalAttributes->sign = vertex.attributes.normal.sign;

                        auto secondaryUvAttributes = std::make_unique<ToolkitUtil::MeshSecondaryUVAttributesT>();
                        secondaryUvAttributes->normals = std::move(normalAttributes);
                        secondaryUvAttributes->uv = vertex.attributes.secondUv.uv2;
                        vertexT->secondary_uv_attributes = std::move(secondaryUvAttributes);
                        break;
                    }
                    case CoreGraphics::VertexLayoutType::Skin:
                    {
                        auto normalAttributes = std::make_unique<ToolkitUtil::MeshNormalAttributesT>();
                        normalAttributes->normal = vertex.attributes.normal.normal;
                        normalAttributes->tangent = vertex.attributes.normal.tangent;
                        normalAttributes->sign = vertex.attributes.normal.sign;

                        auto skinAttributes = std::make_unique<ToolkitUtil::MeshSkinAttributesT>();
                        skinAttributes->normals = std::move(normalAttributes);
                        skinAttributes->weights = vertex.attributes.skin.weights;
                        skinAttributes->indices = { vertex.attributes.skin.indices.x, vertex.attributes.skin.indices.y, vertex.attributes.skin.indices.z, vertex.attributes.skin.indices.w };
                        vertexT->skin_attributes = std::move(skinAttributes);
                        break;
                    }
                }
                mesh->vertices.push_back(std::move(vertexT));
            }
            mesh->triangles.reserve(builder->triangles.size());
            for (auto& triangle : builder->triangles)
            {
                uint32_t indices[3] = 
                {
                    static_cast<uint32_t>(triangle.GetVertexIndex(0)),
                    static_cast<uint32_t>(triangle.GetVertexIndex(1)),
                    static_cast<uint32_t>(triangle.GetVertexIndex(2))
                };
                auto triangleT = ToolkitUtil::MeshTriangle(indices);
                mesh->triangles.push_back(std::move(triangleT));
            }
            mesh->groups.reserve(builder->groups.size());
            for (auto& group : builder->groups)
            {
                auto groupT = ToolkitUtil::MeshTriangleGroup(group.GetFirstTriangleIndex(), group.GetNumTriangles());
                mesh->groups.push_back(std::move(groupT));
            }
            meshResource.meshes.push_back(std::move(mesh));
        }

        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MeshResource>(meshResource);
        stream->Write(data.GetPtr(), data.Size());

        stream->Close();
        stream = nullptr;
        return true;
    }
    else
    {
        // failed to open write stream
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderSaver::SaveBinary(const IO::URI& uri, const ToolkitUtil::MeshResourceT* resource, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        
        MeshBuilderSaver::WriteHeader(stream, resource, byteOrder);
        MeshBuilderSaver::WriteMeshes(stream, resource, byteOrder);
        MeshBuilderSaver::WriteMeshlets(stream, resource, byteOrder);
        MeshBuilderSaver::WriteVertices(stream, resource, byteOrder);
        MeshBuilderSaver::WriteTriangles(stream, resource, byteOrder);

        stream->Close();
        stream = nullptr;
        return true;
    }
    else
    {
        // failed to open write stream
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteHeader(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder)
{
    size_t indexDataSize = 0;
    size_t vertexDataSize = 0;
    size_t meshDataSize = resource->meshes.size() * sizeof(Nvx3VertexRange);
    size_t meshletDataSize = 0;
    for (IndexT i = 0; i < resource->meshes.size(); i++)
    {
        meshDataSize += resource->meshes[i]->groups.size() * sizeof(Nvx3Group);
        
        // The enum is the size of the type
        vertexDataSize += sizeof(CoreGraphics::BaseVertex) * resource->meshes[i]->vertices.size();
        vertexDataSize += MeshBuilderVertex::GetSize(resource->meshes[i]->component_mask) * resource->meshes[i]->vertices.size();
        if (resource->meshes[i]->vertices.size() >= 0xFFFF)
            indexDataSize += resource->meshes[i]->triangles.size() * 3 * sizeof(uint);
        else
            indexDataSize += resource->meshes[i]->triangles.size() * 3 * sizeof(ushort);
    }

    // write header
    Nvx3Header nvx3Header;
    nvx3Header.magic = byteOrder.Convert<uint>(NEBULA_NVX_MAGICNUMBER);
    nvx3Header.meshDataOffset = sizeof(Nvx3Header);
    nvx3Header.numMeshes = byteOrder.Convert<uint>((uint)resource->meshes.size());
    nvx3Header.meshletDataOffset = sizeof(Nvx3Header) + meshDataSize;
    nvx3Header.numMeshlets = 0; // TODO: Add meshlet support
    nvx3Header.vertexDataOffset = sizeof(Nvx3Header) + meshDataSize + meshletDataSize; 
    nvx3Header.vertexDataSize = vertexDataSize;
    nvx3Header.indexDataOffset = sizeof(Nvx3Header) + meshDataSize + meshletDataSize + vertexDataSize;
    nvx3Header.indexDataSize = indexDataSize;

    // write header
    stream->Write(&nvx3Header, sizeof(Nvx3Header));
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteMeshes(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder)
{
    size_t indexByteOffset = 0;
    size_t vertexByteOffset = 0;
    size_t groupByteOffset = sizeof(Nvx3Header) + resource->meshes.size() * sizeof(Nvx3VertexRange);

    Util::Array<Nvx3Group, 32> groups;
    for (IndexT curMeshIndex = 0; curMeshIndex < resource->meshes.size(); curMeshIndex++)
    {
        const auto& mesh = resource->meshes[curMeshIndex];

        size_t baseVertexDataSize = mesh->vertices.size() * sizeof(BaseVertex);
        size_t attributesVertexDataSize = mesh->vertices.size() * MeshBuilderVertex::GetSize(mesh->component_mask);

        Nvx3VertexRange nvx3VertexRange;
        nvx3VertexRange.indexType = mesh->vertices.size() > 0xFFFF ? CoreGraphics::IndexType::Index32 : CoreGraphics::IndexType::Index16;
        nvx3VertexRange.layout = MeshBuilderVertex::GetVertexLayoutType(mesh->component_mask);
        nvx3VertexRange.indexByteOffset = indexByteOffset;
        nvx3VertexRange.baseVertexByteOffset = vertexByteOffset;
        nvx3VertexRange.attributesVertexByteOffset = vertexByteOffset + baseVertexDataSize;
        nvx3VertexRange.firstGroupOffset = groupByteOffset;
        nvx3VertexRange.numGroups = mesh->groups.size();
        stream->Write(&nvx3VertexRange, sizeof(Nvx3VertexRange));

        for (IndexT curGroupIndex = 0; curGroupIndex < mesh->groups.size(); curGroupIndex++)
        {
            const auto& group = mesh->groups[curGroupIndex];
            int firstTriangle = group.first_triangle();
            int numTriangles = group.num_triangles();
            
            Nvx3Group nvx3Group;
            nvx3Group.firstIndex = byteOrder.Convert<uint>(firstTriangle * 3);
            nvx3Group.numIndices = byteOrder.Convert<uint>(numTriangles * 3);
            nvx3Group.primType = PrimitiveTopology::TriangleList;
            
            // TODO: Add support for meshlets
            nvx3Group.firstMeshlet = 0;
            nvx3Group.numMeshlets = 0;
            groups.Append(nvx3Group);
        }

        groupByteOffset += mesh->groups.size() * sizeof(Nvx3Group);
        indexByteOffset += mesh->triangles.size() * 3 * IndexType::SizeOf(nvx3VertexRange.indexType);
        vertexByteOffset += baseVertexDataSize + attributesVertexDataSize;
    }

    // Write groups after all meshes
    for (IndexT groupIndex = 0; groupIndex < groups.Size(); groupIndex++)
    {
        stream->Write(&groups[groupIndex], sizeof(Nvx3Group));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteVertices(const Ptr<Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const ByteOrder& byteOrder)
{
    // First pass, output base attributes (positions and uvs)
    for (auto& mesh : resource->meshes)
    {
        CoreGraphics::BaseVertex* baseBuffer = new CoreGraphics::BaseVertex[mesh->vertices.size()];
        for (uint vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
        {
            const auto& vtx = mesh->vertices[vertexIndex];
            CoreGraphics::BaseVertex& outVtx = baseBuffer[vertexIndex];
            outVtx.position[0] = vtx->position.x;
            outVtx.position[1] = vtx->position.y;
            outVtx.position[2] = vtx->position.z;
            outVtx.uv[0] = vtx->uv.x * 1000.0f;
            outVtx.uv[1] = vtx->uv.y * 1000.0f;
        }
        stream->Write(baseBuffer, sizeof(CoreGraphics::BaseVertex) * mesh->vertices.size());
        delete[] baseBuffer;

        CoreGraphics::VertexLayoutType layout = MeshBuilderVertex::GetVertexLayoutType(mesh->component_mask);
        byte* attributeBuffer = nullptr;
        uint bufferSize = 0;
        auto normalLambda = [](CoreGraphics::NormalVertex& outVtx, const std::unique_ptr<ToolkitUtil::MeshNormalAttributesT>& vtx)
        {
            Math::vec3 const n = Math::normalize(vtx->normal);
            outVtx.normal.x = n.x * 127.0f;
            outVtx.normal.y = n.y * 127.0f;
            outVtx.normal.z = n.z * 127.0f;
            outVtx.normal.w = 0.0f;

            Math::vec3 const t = Math::normalize(vtx->tangent);
            outVtx.tangent.x = t.x * 127.0f;
            outVtx.tangent.y = t.y * 127.0f;
            outVtx.tangent.z = t.z * 127.0f;
            outVtx.tangent.w = vtx->sign < 0.0f ? 0x80 : 0x7F;
        };
        switch (layout)
        {
            case CoreGraphics::VertexLayoutType::Normal:
            {
                bufferSize = sizeof(CoreGraphics::NormalVertex) * mesh->vertices.size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::NormalVertex* attrBuffer = (CoreGraphics::NormalVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
                {
                    const auto& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::NormalVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx->normal_attributes);
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::Colors:
            {
                bufferSize = sizeof(CoreGraphics::ColorVertex) * mesh->vertices.size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::ColorVertex* attrBuffer = (CoreGraphics::ColorVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
                {
                    const auto& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::ColorVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx->normal_attributes);

                    outVtx.color.x = vtx->color_attributes->color.x * 255.0f;
                    outVtx.color.y = vtx->color_attributes->color.y * 255.0f;
                    outVtx.color.z = vtx->color_attributes->color.z * 255.0f;
                    outVtx.color.w = vtx->color_attributes->color.w * 255.0f;
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::SecondUV:
            {
                bufferSize = sizeof(CoreGraphics::SecondUVVertex) * mesh->vertices.size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::SecondUVVertex* attrBuffer = (CoreGraphics::SecondUVVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
                {
                    const auto& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::SecondUVVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx->normal_attributes);

                    outVtx.uv2[0] = vtx->secondary_uv_attributes->uv.x * 65535.0f;
                    outVtx.uv2[1] = vtx->secondary_uv_attributes->uv.y * 65535.0f;
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::Skin:
            {
                bufferSize = sizeof(CoreGraphics::SkinVertex) * mesh->vertices.size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::SkinVertex* attrBuffer = (CoreGraphics::SkinVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.size(); vertexIndex++)
                {
                    const auto& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::SkinVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx->normal_attributes);

                    outVtx.skinWeights[0] = vtx->skin_attributes->weights.x;
                    outVtx.skinWeights[1] = vtx->skin_attributes->weights.y;
                    outVtx.skinWeights[2] = vtx->skin_attributes->weights.z;
                    outVtx.skinWeights[3] = vtx->skin_attributes->weights.w;

                    outVtx.skinIndices.x = vtx->skin_attributes->indices[0];
                    outVtx.skinIndices.y = vtx->skin_attributes->indices[1];
                    outVtx.skinIndices.z = vtx->skin_attributes->indices[2];
                    outVtx.skinIndices.w = vtx->skin_attributes->indices[3];
                }
                break;
            }
        }
        stream->Write(attributeBuffer, bufferSize);
        delete[] attributeBuffer;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteTriangles(const Ptr<Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const ByteOrder& byteOrder)
{
    struct Triangle
    {
        uint v0, v1, v2;
    };

    struct Triangle16
    {
        ushort v0, v1, v2;
    };

    for (auto& mesh : resource->meshes)
    {
        size_t numTriangles = mesh->triangles.size();
        CoreGraphics::IndexType::Code indexType = mesh->vertices.size() > 0xFFFF ? CoreGraphics::IndexType::Index32 : CoreGraphics::IndexType::Index16;
        size_t bufferSize = numTriangles * 3 * CoreGraphics::IndexType::SizeOf(indexType);

        byte* buffer = new byte[bufferSize];

        switch (indexType)
        {
            case CoreGraphics::IndexType::Index32:
            {
                Triangle* tris32 = (Triangle*)buffer;
                for (IndexT i = 0; i < numTriangles; i++)
                {
                    const auto& curTriangle = mesh->triangles[i];
                    tris32[i].v0 = curTriangle.indices()->Get(0);
                    tris32[i].v1 = curTriangle.indices()->Get(1);
                    tris32[i].v2 = curTriangle.indices()->Get(2);
                }
                break;
            }
            case CoreGraphics::IndexType::Index16:
            {
                Triangle16* tris16 = (Triangle16*)buffer;
                for (IndexT i = 0; i < numTriangles; i++)
                {
                    const auto& curTriangle = mesh->triangles[i];
                    tris16[i].v0 = curTriangle.indices()->Get(0);
                    tris16[i].v1 = curTriangle.indices()->Get(1);
                    tris16[i].v2 = curTriangle.indices()->Get(2);
                }
                break;
            }
        }

        // write triangle to stream
        stream->Write(buffer, bufferSize);
        delete[] buffer;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
MeshBuilderSaver::WriteMeshlets(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder)
{
}

} // namespace ToolkitUtil
