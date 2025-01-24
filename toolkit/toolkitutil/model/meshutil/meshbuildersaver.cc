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
MeshBuilderSaver::Save(const IO::URI& uri, const Util::Array<MeshBuilder*>& meshes, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));

        MeshBuilderSaver::WriteHeader(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteMeshes(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteMeshlets(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteVertices(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteTriangles(stream, meshes, byteOrder);

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
MeshBuilderSaver::WriteHeader(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const System::ByteOrder& byteOrder)
{
    SizeT indexDataSize = 0;
    SizeT vertexDataSize = 0;
    SizeT meshDataSize = meshes.Size() * sizeof(Nvx3VertexRange);
    SizeT meshletDataSize = 0;
    for (IndexT i = 0; i < meshes.Size(); i++)
    {
        meshDataSize += meshes[i]->groups.Size() * sizeof(Nvx3Group);
        
        // The enum is the size of the type
        vertexDataSize += sizeof(CoreGraphics::BaseVertex) * meshes[i]->vertices.Size();
        vertexDataSize += MeshBuilderVertex::GetSize(meshes[i]->componentMask) * meshes[i]->vertices.Size();
        if (meshes[i]->vertices.Size() >= 0xFFFF)
            indexDataSize += meshes[i]->triangles.Size() * 3 * sizeof(uint);
        else
            indexDataSize += meshes[i]->triangles.Size() * 3 * sizeof(ushort);
    }

    // write header
    Nvx3Header nvx3Header;
    nvx3Header.magic = byteOrder.Convert<uint>(NEBULA_NVX_MAGICNUMBER);
    nvx3Header.meshDataOffset = sizeof(Nvx3Header);
    nvx3Header.numMeshes = byteOrder.Convert<uint>(meshes.Size());
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
MeshBuilderSaver::WriteMeshes(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const System::ByteOrder& byteOrder)
{
    uint indexByteOffset = 0;
    uint vertexByteOffset = 0;
    uint groupByteOffset = sizeof(Nvx3Header) + meshes.Size() * sizeof(Nvx3VertexRange);

    Util::Array<Nvx3Group, 32> groups;
    for (IndexT curMeshIndex = 0; curMeshIndex < meshes.Size(); curMeshIndex++)
    {
        const MeshBuilder* mesh = meshes[curMeshIndex];

        uint baseVertexDataSize = mesh->vertices.Size() * sizeof(BaseVertex);
        uint attributesVertexDataSize = mesh->vertices.Size() * MeshBuilderVertex::GetSize(mesh->componentMask);

        Nvx3VertexRange nvx3VertexRange;
        nvx3VertexRange.indexType = mesh->vertices.Size() > 0xFFFF ? CoreGraphics::IndexType::Index32 : CoreGraphics::IndexType::Index16;
        nvx3VertexRange.layout = MeshBuilderVertex::GetVertexLayoutType(mesh->componentMask);
        nvx3VertexRange.indexByteOffset = indexByteOffset;
        nvx3VertexRange.baseVertexByteOffset = vertexByteOffset;
        nvx3VertexRange.attributesVertexByteOffset = vertexByteOffset + baseVertexDataSize;
        nvx3VertexRange.firstGroupOffset = groupByteOffset;
        nvx3VertexRange.numGroups = mesh->groups.Size();
        stream->Write(&nvx3VertexRange, sizeof(Nvx3VertexRange));

        for (IndexT curGroupIndex = 0; curGroupIndex < mesh->groups.Size(); curGroupIndex++)
        {
            const MeshBuilderGroup& group = mesh->groups[curGroupIndex];
            int firstTriangle = group.GetFirstTriangleIndex();
            int numTriangles = group.GetNumTriangles();
            
            Nvx3Group nvx3Group;
            nvx3Group.firstIndex = byteOrder.Convert<uint>(firstTriangle * 3);
            nvx3Group.numIndices = byteOrder.Convert<uint>(numTriangles * 3);
            nvx3Group.primType = PrimitiveTopology::TriangleList;
            
            // TODO: Add support for meshlets
            nvx3Group.firstMeshlet = 0;
            nvx3Group.numMeshlets = 0;
            groups.Append(nvx3Group);
        }

        groupByteOffset += mesh->groups.Size() * sizeof(Nvx3Group);
        indexByteOffset += mesh->triangles.Size() * 3 * IndexType::SizeOf(nvx3VertexRange.indexType);
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
MeshBuilderSaver::WriteVertices(const Ptr<Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const ByteOrder& byteOrder)
{
    // First pass, output base attributes (positions and uvs)
    for (auto& mesh : meshes)
    {
        CoreGraphics::BaseVertex* baseBuffer = new CoreGraphics::BaseVertex[mesh->vertices.Size()];
        for (uint vertexIndex = 0; vertexIndex < mesh->vertices.Size(); vertexIndex++)
        {
            const MeshBuilderVertex& vtx = mesh->vertices[vertexIndex];
            CoreGraphics::BaseVertex& outVtx = baseBuffer[vertexIndex];
            outVtx.position[0] = vtx.base.position.x;
            outVtx.position[1] = vtx.base.position.y;
            outVtx.position[2] = vtx.base.position.z;
            outVtx.uv[0] = vtx.base.uv.x * 1000.0f;
            outVtx.uv[1] = vtx.base.uv.y * 1000.0f;
        }
        stream->Write(baseBuffer, sizeof(CoreGraphics::BaseVertex) * mesh->vertices.Size());
        delete[] baseBuffer;

        CoreGraphics::VertexLayoutType layout = MeshBuilderVertex::GetVertexLayoutType(mesh->componentMask);
        byte* attributeBuffer = nullptr;
        uint bufferSize = 0;
        auto normalLambda = [](CoreGraphics::NormalVertex& outVtx, const MeshBuilderVertex& vtx)
        {
            Math::vec3 const n = Math::normalize(vtx.attributes.normal.normal);
            outVtx.normal.x = n.x * 127.0f;
            outVtx.normal.y = n.y * 127.0f;
            outVtx.normal.z = n.z * 127.0f;
            outVtx.normal.w = 0.0f;

            Math::vec3 const t = Math::normalize(vtx.attributes.normal.tangent);
            outVtx.tangent.x = t.x * 127.0f;
            outVtx.tangent.y = t.y * 127.0f;
            outVtx.tangent.z = t.z * 127.0f;
            outVtx.tangent.w = vtx.attributes.normal.sign < 0.0f ? 0x80 : 0x7F;
        };
        switch (layout)
        {
            case CoreGraphics::VertexLayoutType::Normal:
            {
                bufferSize = sizeof(CoreGraphics::NormalVertex) * mesh->vertices.Size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::NormalVertex* attrBuffer = (CoreGraphics::NormalVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.Size(); vertexIndex++)
                {
                    const MeshBuilderVertex& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::NormalVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx);
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::Colors:
            {
                bufferSize = sizeof(CoreGraphics::ColorVertex) * mesh->vertices.Size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::ColorVertex* attrBuffer = (CoreGraphics::ColorVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.Size(); vertexIndex++)
                {
                    const MeshBuilderVertex& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::ColorVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx);

                    outVtx.color.x = vtx.attributes.color.color.x * 255.0f;
                    outVtx.color.y = vtx.attributes.color.color.y * 255.0f;
                    outVtx.color.z = vtx.attributes.color.color.z * 255.0f;
                    outVtx.color.w = vtx.attributes.color.color.w * 255.0f;
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::SecondUV:
            {
                bufferSize = sizeof(CoreGraphics::SecondUVVertex) * mesh->vertices.Size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::SecondUVVertex* attrBuffer = (CoreGraphics::SecondUVVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.Size(); vertexIndex++)
                {
                    const MeshBuilderVertex& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::SecondUVVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx);

                    outVtx.uv2[0] = vtx.attributes.secondUv.uv2.x * 65535.0f;
                    outVtx.uv2[1] = vtx.attributes.secondUv.uv2.y * 65535.0f;
                }
                break;
            }
            case CoreGraphics::VertexLayoutType::Skin:
            {
                bufferSize = sizeof(CoreGraphics::SkinVertex) * mesh->vertices.Size();
                attributeBuffer = (byte*)new byte[bufferSize];
                CoreGraphics::SkinVertex* attrBuffer = (CoreGraphics::SkinVertex*)attributeBuffer;
                for (uint vertexIndex = 0; vertexIndex < mesh->vertices.Size(); vertexIndex++)
                {
                    const MeshBuilderVertex& vtx = mesh->vertices[vertexIndex];
                    CoreGraphics::SkinVertex& outVtx = attrBuffer[vertexIndex];
                    normalLambda(outVtx, vtx);

                    outVtx.skinWeights[0] = vtx.attributes.skin.weights.x;
                    outVtx.skinWeights[1] = vtx.attributes.skin.weights.y;
                    outVtx.skinWeights[2] = vtx.attributes.skin.weights.z;
                    outVtx.skinWeights[3] = vtx.attributes.skin.weights.w;

                    outVtx.skinIndices.x = vtx.attributes.skin.remapIndices.x;
                    outVtx.skinIndices.y = vtx.attributes.skin.remapIndices.y;
                    outVtx.skinIndices.z = vtx.attributes.skin.remapIndices.z;
                    outVtx.skinIndices.w = vtx.attributes.skin.remapIndices.w;
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
MeshBuilderSaver::WriteTriangles(const Ptr<Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const ByteOrder& byteOrder)
{
    struct Triangle
    {
        uint v0, v1, v2;
    };

    struct Triangle16
    {
        ushort v0, v1, v2;
    };

    for (auto& mesh : meshes)
    {
        int numTriangles = mesh->GetNumTriangles();
        CoreGraphics::IndexType::Code indexType = mesh->vertices.Size() > 0xFFFF ? CoreGraphics::IndexType::Index32 : CoreGraphics::IndexType::Index16;
        SizeT bufferSize = numTriangles * 3 * CoreGraphics::IndexType::SizeOf(indexType);

        byte* buffer = new byte[bufferSize];

        for (IndexT i = 0; i < numTriangles; i++)
        {
            const MeshBuilderTriangle& curTriangle = mesh->TriangleAt(i);
            IndexT v0, v1, v2;
            curTriangle.GetVertexIndices(v0, v1, v2);

            switch (indexType)
            {
                case CoreGraphics::IndexType::Index32:
                {
                    Triangle* tris32 = (Triangle*)buffer;
                    tris32[i].v0 = v0;
                    tris32[i].v1 = v1;
                    tris32[i].v2 = v2;
                    break;
                }
                case CoreGraphics::IndexType::Index16:
                {
                    Triangle16* tris16 = (Triangle16*)buffer;
                    tris16[i].v0 = v0;
                    tris16[i].v1 = v1;
                    tris16[i].v2 = v2;
                    break;
                }
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
MeshBuilderSaver::WriteMeshlets(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const System::ByteOrder& byteOrder)
{
}

} // namespace ToolkitUtil
