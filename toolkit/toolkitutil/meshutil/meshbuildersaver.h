#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderSaver
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/uri.h"
#include "toolkitutil/meshutil/meshbuilder.h"
#include "toolkit-common/platform.h"
#include "io/stream.h"
#include "system/byteorder.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class MeshBuilderSaver
{
public:
    /// save nvx2 file
    static bool SaveNvx2(const IO::URI& uri, MeshBuilder& meshBuilder, Platform::Code platform);
    /// save nvx3 file
    static bool SaveNvx3(const IO::URI& uri, MeshBuilder& meshBuilder, Platform::Code platform);
private:

    /// write header to stream using nvx3
    static void WriteHeaderNvx3(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, SizeT numGroups, const System::ByteOrder& byteOrder);
    /// write groups to stream in nvx3
    static void WriteGroupsNvx3(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, Util::Array<MeshBuilderGroup>& groupMap, const System::ByteOrder& byteOrder);

    /// write header to stream in nvx2
    static void WriteHeaderNvx2(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, SizeT numGroups, const System::ByteOrder& byteOrder);
    /// write groups to stream in nvx2
    static void WriteGroupsNvx2(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, Util::Array<MeshBuilderGroup>& groupMap, const System::ByteOrder& byteOrder);

    /// write the vertices to stream
    static void WriteVertices(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, const System::ByteOrder& byteOrder);
    /// write the triangles to stream
    static void WriteTriangles(const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, const System::ByteOrder& byteOrder);
    

    
};



} // namespace ToolkitUtil
    