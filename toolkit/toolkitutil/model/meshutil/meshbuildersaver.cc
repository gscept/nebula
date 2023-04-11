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
MeshBuilderSaver::Save(const IO::URI& uri, const Util::Array<MeshBuilder*>& meshes, const Util::Array<MeshBuilderGroup>& groups, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));

        MeshBuilderSaver::WriteHeader(stream, meshes, groups, byteOrder);
        MeshBuilderSaver::WriteMeshes(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteGroups(stream, groups, byteOrder);
        MeshBuilderSaver::WriteVertices(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteTriangles(stream, meshes, byteOrder);
        MeshBuilderSaver::WriteMeshlets(stream, meshes, byteOrder);

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
MeshBuilderSaver::WriteHeader(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const Util::Array<MeshBuilderGroup>& groups, const System::ByteOrder& byteOrder)
{
    SizeT indexDataSize = 0;
    SizeT vertexDataSize = 0;
    for (IndexT i = 0; i < meshes.Size(); i++)
    {
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
    nvx3Header.numMeshes = byteOrder.Convert<uint>(meshes.Size());
    nvx3Header.numGroups = byteOrder.Convert<uint>(groups.Size());
    nvx3Header.numMeshlets = 0;
    nvx3Header.indexDataSize = indexDataSize;
    nvx3Header.vertexDataSize = vertexDataSize;

    // write header
    stream->Write(&nvx3Header, sizeof(nvx3Header));
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteMeshes(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilder*>& meshes, const System::ByteOrder& byteOrder)
{
    uint indexByteOffset = 0;
    uint vertexByteOffset = 0;
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

        stream->Write(&nvx3VertexRange, sizeof(nvx3VertexRange));

        indexByteOffset += mesh->triangles.Size() * 3 * IndexType::SizeOf(nvx3VertexRange.indexType);
        vertexByteOffset += baseVertexDataSize + attributesVertexDataSize;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteGroups(const Ptr<IO::Stream>& stream, const Util::Array<MeshBuilderGroup>& groups, const System::ByteOrder& byteOrder)
{
    for (IndexT groupIndex = 0; groupIndex < groups.Size(); groupIndex++)
    {
        const MeshBuilderGroup& curGroup = groups[groupIndex];
        int firstTriangle = curGroup.GetFirstTriangleIndex();
        int numTriangles = curGroup.GetNumTriangles();

        Nvx3Group nvx3Group;
        nvx3Group.firstIndex = byteOrder.Convert<uint>(firstTriangle * 3);
        nvx3Group.numIndices = byteOrder.Convert<uint>(numTriangles * 3);
        nvx3Group.primType = PrimitiveTopology::TriangleList;
        nvx3Group.firstMeshlet = 0;
        nvx3Group.numMeshlets = 0; // TODO

        // write group to stream
        stream->Write(&nvx3Group, sizeof(nvx3Group));
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
        CoreGraphics::BaseVertex* baseBuffer = n_new_array(CoreGraphics::BaseVertex, mesh->vertices.Size());
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
        n_delete_array(baseBuffer);

        CoreGraphics::VertexLayoutType layout = MeshBuilderVertex::GetVertexLayoutType(mesh->componentMask);
        byte* attributeBuffer = nullptr;
        uint bufferSize = 0;
        auto normalLambda = [](CoreGraphics::NormalVertex& outVtx, const MeshBuilderVertex& vtx)
        {
            outVtx.normal.x = vtx.attributes.normal.normal.x * 0.5f * 255.0f;
            outVtx.normal.y = vtx.attributes.normal.normal.y * 0.5f * 255.0f;
            outVtx.normal.z = vtx.attributes.normal.normal.z * 0.5f * 255.0f;
            outVtx.normal.w = 0.0f;

            outVtx.tangent.x = vtx.attributes.normal.tangent.x * 0.5f * 255.0f;
            outVtx.tangent.y = vtx.attributes.normal.tangent.y * 0.5f * 255.0f;
            outVtx.tangent.z = vtx.attributes.normal.tangent.z * 0.5f * 255.0f;
            outVtx.tangent.w = vtx.attributes.normal.sign > 0 ? 128.0f : -127.0f; // unused
        };
        switch (layout)
        {
            case CoreGraphics::VertexLayoutType::Normal:
            {
                bufferSize = sizeof(CoreGraphics::NormalVertex) * mesh->vertices.Size();
                attributeBuffer = (byte*)n_new_array(byte, bufferSize);
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
                attributeBuffer = (byte*)n_new_array(byte, bufferSize);
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
                attributeBuffer = (byte*)n_new_array(byte, bufferSize);
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
                attributeBuffer = (byte*)n_new_array(byte, bufferSize);
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
        n_delete_array(attributeBuffer);
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

        byte* buffer = n_new_array(byte, bufferSize);

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
        n_delete_array(buffer);
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
