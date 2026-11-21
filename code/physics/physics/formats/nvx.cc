//------------------------------------------------------------------------------
//  @file nvx.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "physics/geometry.h"
#include "physics/formats/formats.h"
#include "io/ioserver.h"
#include "io/streamreader.h"
#include "io/path.h"

#if WITH_NEBULA_PHYSICS_FORMAT_NVX
#include "coregraphics/nvx3fileformatstructs.h"
#include "coregraphics/indextype.h"

namespace Physics
{

//------------------------------------------------------------------------------
/**
    The resource is a mesh URN, folder:file:msh, which is read from the exported nvx file.
    meshIndex selects the vertex range in the file, primGroup the group within that range.
*/
bool
NvxMeshGeometry(Util::StringAtom resource, IndexT meshIndex, IndexT primGroup, MeshGeometryInput& out)
{
    const IO::Path path = IO::Path::Parse(resource.AsString());
    if (!path.IsFile() || path.GetType() != "msh")
    {
        n_warning("NvxMeshGeometry: '%s' is not a mesh URN (folder:file:msh)\n", resource.Value());
        return false;
    }

    Ptr<IO::Stream> stream = IO::CreateStream(path.ExportURI());
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
        n_warning("NvxMeshGeometry: '%s' is not a nvx file\n", stream->GetURI().AsString().AsCharPtr());
        stream->MemoryUnmap();
        nvx3Reader->Close();
        return false;
    }

    if (meshIndex < 0 || (uint)meshIndex >= header->numMeshes)
    {
        n_warning("NvxMeshGeometry: '%s' has %u meshes, mesh %d requested\n", resource.Value(), header->numMeshes, meshIndex);
        stream->MemoryUnmap();
        nvx3Reader->Close();
        return false;
    }

    CoreGraphics::Nvx3Elements elements;
    CoreGraphics::Nvx3::FillNvx3Elements(basePtr, header, elements);

    const CoreGraphics::Nvx3VertexRange& range = elements.ranges[meshIndex];
    if (primGroup < 0 || (uint)primGroup >= range.numGroups)
    {
        n_warning("NvxMeshGeometry: mesh %d of '%s' has %u groups, group %d requested\n", meshIndex, resource.Value(), range.numGroups, primGroup);
        stream->MemoryUnmap();
        nvx3Reader->Close();
        return false;
    }

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
    // firstIndex counts indices from the start of the range's index data, the same as the mesh loader's base index
    const ubyte* indexBuffer = elements.indexData + range.indexByteOffset + group->firstIndex * Physics::IndexSize(out.indexType);
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
