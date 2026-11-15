//------------------------------------------------------------------------------
//  @file nvx.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/geometry.h"
#include "physics/formats/formats.h"
#include "io/ioserver.h"
#include "io/streamreader.h"

#if WITH_NEBULA_PHYSICS_FORMAT_NVX
#include "coregraphics/nvx3fileformatstructs.h"
#include "coregraphics/indextype.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
bool
NvxMeshGeometry(Util::StringAtom resource, IndexT primGroup, MeshGeometryInput& out)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(resource.AsString()));
    stream->SetAccessMode(IO::Stream::ReadAccess);

    Ptr<IO::StreamReader> nvx3Reader = IO::StreamReader::Create();
    nvx3Reader->SetStream(stream);
    if (!nvx3Reader->Open())
    {
        return false;
    }

    n_assert(stream->CanBeMapped());
    char* basePtr = (char*)stream->MemoryMap();
    n_assert(nullptr != basePtr);

    CoreGraphics::Nvx3Header* header = (CoreGraphics::Nvx3Header*)basePtr;
    n_assert(header != nullptr);

    if (header->magic != NEBULA_NVX_MAGICNUMBER)
    {
        // not a nvx3 file, break hard
        n_error("NvxMeshGeometry: '%s' is not a nvx file!", stream->GetURI().AsString().AsCharPtr());
        stream->MemoryUnmap();
        nvx3Reader->Close();
        return false;
    }

    n_assert(header->numMeshes > 0);

    CoreGraphics::Nvx3Elements elements;
    CoreGraphics::Nvx3::FillNvx3Elements(basePtr, header, elements);

    const CoreGraphics::Nvx3VertexRange& range = elements.ranges[0];
    const CoreGraphics::Nvx3Group* group =
        (const CoreGraphics::Nvx3Group*)(basePtr + range.firstGroupOffset + sizeof(CoreGraphics::Nvx3Group) * primGroup);

    out.indexType = range.indexType == CoreGraphics::IndexType::Index16 ? Physics::IndexType::Index16 : Physics::IndexType::Index32;

    const CoreGraphics::BaseVertex* vertexBuffer = (const CoreGraphics::BaseVertex*)(elements.vertexData + range.baseVertexByteOffset);
    out.vertexCount = (range.attributesVertexByteOffset - range.baseVertexByteOffset) / sizeof(CoreGraphics::BaseVertex);

    out.positions.Reserve(out.vertexCount * 3);
    for (SizeT i = 0; i < out.vertexCount; i++)
    {
        out.positions.Append(vertexBuffer[i].position[0]);
        out.positions.Append(vertexBuffer[i].position[1]);
        out.positions.Append(vertexBuffer[i].position[2]);
    }

    out.indexCount = group->numIndices;
    SizeT indexByteSize = out.indexCount * Physics::IndexSize(out.indexType);
    const ubyte* indexBuffer = elements.indexData + range.indexByteOffset;
    out.indices.Reserve(indexByteSize);
    for (SizeT i = 0; i < indexByteSize; i++)
    {
        out.indices.Append(indexBuffer[i]);
    }

    // skin data is only present when the mesh was exported with a skin layout
    if (range.layout == CoreGraphics::VertexLayoutType::Skin)
    {
        const CoreGraphics::SkinVertex* skinBuffer = (const CoreGraphics::SkinVertex*)(elements.vertexData + range.attributesVertexByteOffset);
        out.weights.Reserve(out.vertexCount * 4);
        out.bones.Reserve(out.vertexCount);
        for (SizeT i = 0; i < out.vertexCount; i++)
        {
            for (IndexT k = 0; k < 4; k++)
            {
                out.weights.Append(skinBuffer[i].skinWeights[k]);
            }
            out.bones.Append((uint)skinBuffer[i].skinIndices.x);
        }
    }

    stream->MemoryUnmap();
    nvx3Reader->Close();
    return out.vertexCount > 0;
}

} // namespace Physics
#endif
