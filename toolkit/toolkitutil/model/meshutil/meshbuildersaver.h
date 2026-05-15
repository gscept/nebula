#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderSaver
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/uri.h"
#include "model/meshutil/meshbuilder.h"
#include "toolkit-common/platform.h"
#include "io/stream.h"
#include "system/byteorder.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
struct MeshResourceT;
class MeshBuilderSaver
{
public:
    /// Save flatbuffers mesh file
    static bool SaveImport(const IO::URI& uri, const Util::Array<MeshBuilder*>& meshes, Platform::Code platform);
    /// Save binary mesh file
    static bool SaveBinary(const IO::URI& uri, const ToolkitUtil::MeshResourceT* resource, Platform::Code platform);
private:

    /// write header to stream using nvx3
    static void WriteHeader(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder);
    /// Write meshes
    static void WriteMeshes(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder);

    /// write the vertices to stream
    static void WriteVertices(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder);
    /// write the triangles to stream
    static void WriteTriangles(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder);
    /// Write meshlets
    static void WriteMeshlets(const Ptr<IO::Stream>& stream, const ToolkitUtil::MeshResourceT* resource, const System::ByteOrder& byteOrder);
};

} // namespace ToolkitUtil
    