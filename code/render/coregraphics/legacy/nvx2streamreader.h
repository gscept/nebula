#pragma once
//------------------------------------------------------------------------------
/**
    @class Legacy::Nvx2StreamReader
  
    A stream reader which reads legacy nvx2 binary mesh files.

    NOTE: this class exists purely for debugging and shouldn't be used in
    production code!

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/    
#include "core/config.h"
#if NEBULA_LEGACY_SUPPORT
#include "io/streamreader.h"
#include "coregraphics/buffer.h"
#include "coregraphics/primitivegroup.h"

//------------------------------------------------------------------------------
namespace Legacy
{
class Nvx2StreamReader : public IO::StreamReader
{
    __DeclareClass(Nvx2StreamReader);
public:
    /// contructor
    Nvx2StreamReader();
    /// destructor
    virtual ~Nvx2StreamReader();

    /// enable/disable raw mode (raw mode does not setup vertex/index buffers), default is false
    void SetRawMode(bool b);
    /// get raw mode flag
    bool IsRawMode() const;
    /// set the intended resource usage (default is UsageImmutable)
    void SetUsage(CoreGraphics::GpuBufferTypes::Usage usage);
    /// get resource usage
	CoreGraphics::GpuBufferTypes::Usage GetUsage() const;
    /// set the intended resource access (default is AccessNone)
    void SetAccess(CoreGraphics::GpuBufferTypes::Access access);
    /// get the resource access
	CoreGraphics::GpuBufferTypes::Access GetAccess() const;
    /// begin reading from the stream, read entire data
    virtual bool Open(const Resources::ResourceName& name);
    /// end reading from the stream, destroys loaded objects
    virtual void Close();
	/// get vertex buffer
	const CoreGraphics::BufferId GetVertexBuffer() const;
	/// get index buffer
	const CoreGraphics::BufferId GetIndexBuffer() const;
    /// get primitive groups
    const Util::Array<CoreGraphics::PrimitiveGroup>& GetPrimitiveGroups() const;
    /// get pointer to raw vertex data
    float* GetVertexData() const;
    /// get pointer to raw index data
    ushort* GetIndexData() const;
    /// get number of vertices
    SizeT GetNumVertices() const;
    /// get number of indices
    SizeT GetNumIndices() const;
    /// get vertex width
    SizeT GetVertexWidth() const;
    /// get number of edges
    SizeT GetNumEdges() const;
    /// get vertex components
    const Util::Array<CoreGraphics::VertexComponent>& GetVertexComponents() const;

    /// helper method to convert vertex buffer endianess
    static void ConvertVertexBufferEndianess(void* vertexPtr, SizeT numVertices, const Util::Array<CoreGraphics::VertexComponent>& vertexComps);

private:
    /// read header data from stream
    void ReadHeaderData();
    /// read primitive groups from stream
    void ReadPrimitiveGroups();
    /// setup vertex components array
    void SetupVertexComponents();
    /// update primitive group bounding boxes
    void UpdateGroupBoundingBoxes();
    /// setup the vertex buffer object (not called in raw mode)
    void SetupVertexBuffer(const Resources::ResourceName& name);
    /// setup the index buffer object (not called in raw mode)
    void SetupIndexBuffer(const Resources::ResourceName& name);

    /// Nebula2 vertex components, see Nebula2's nMesh2 class for details
    enum N2VertexComponent
    {
        N2Coord        = (1<<0),      // 3 floats
        N2Normal       = (1<<1),      // 3 floats
        N2NormalB4N   = (1<<2),      // 4 unsigned bytes, normalized
        N2Uv0          = (1<<3),      // 2 floats
        N2Uv0S2        = (1<<4),      // 2 shorts, 4.12 fixed point
        N2Uv1          = (1<<5),      // 2 floats
        N2Uv1S2        = (1<<6),      // 2 shorts, 4.12 fixed point
        N2Uv2          = (1<<7),      // 2 floats
        N2Uv2S2        = (1<<8),      // 2 shorts, 4.12 fixed point
        N2Uv3          = (1<<9),      // 2 floats
        N2Uv3S2        = (1<<10),     // 2 shorts, 4.12 fixed point
        N2Color        = (1<<11),     // 4 floats
        N2ColorUB4N    = (1<<12),     // 4 unsigned bytes, normalized
        N2Tangent      = (1<<13),     // 3 floats
        N2TangentB4N  = (1<<14),     // 4 unsigned bytes, normalized
        N2Binormal     = (1<<15),     // 3 floats
        N2BinormalB4N = (1<<16),     // 4 unsigned bytes, normalized
        N2Weights      = (1<<17),     // 4 floats
        N2WeightsUB4N  = (1<<18),     // 4 unsigned bytes, normalized
        N2JIndices     = (1<<19),     // 4 floats
        N2JIndicesUB4  = (1<<20),     // 4 unsigned bytes

        N2NumVertexComponents = 21,
        N2AllComponents = ((1<<N2NumVertexComponents) - 1),
    };

	CoreGraphics::GpuBufferTypes::Usage usage;
	CoreGraphics::GpuBufferTypes::Access access;

    bool rawMode;
	Util::StringAtom tag;
	Resources::ResourceName name;
	CoreGraphics::BufferId vbo;
	CoreGraphics::BufferId ibo;
    CoreGraphics::VertexLayoutId layout;

    Util::Array<CoreGraphics::PrimitiveGroup> primGroups;
    void* mapPtr;
    void* groupDataPtr;
    void* vertexDataPtr;
    void* indexDataPtr;

    SizeT groupDataSize;
    SizeT vertexDataSize;
    SizeT indexDataSize;
    
    uint numGroups;
    uint numVertices;
    uint vertexWidth;
    uint numIndices;
    uint numEdges;
    uint vertexComponentMask;
    Util::Array<CoreGraphics::VertexComponent> vertexComponents;   
};

//------------------------------------------------------------------------------
/**
*/
inline void
Nvx2StreamReader::SetRawMode(bool b)
{
    this->rawMode = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
Nvx2StreamReader::IsRawMode() const
{
    return this->rawMode;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::BufferId
Nvx2StreamReader::GetVertexBuffer() const
{
	return this->vbo;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::BufferId
Nvx2StreamReader::GetIndexBuffer() const
{
	return this->ibo;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<CoreGraphics::PrimitiveGroup>&
Nvx2StreamReader::GetPrimitiveGroups() const
{
    return this->primGroups;
}

//------------------------------------------------------------------------------
/**
*/
inline float*
Nvx2StreamReader::GetVertexData() const
{
    return (float*) this->vertexDataPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline ushort*
Nvx2StreamReader::GetIndexData() const
{
    return (ushort*) this->indexDataPtr;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Nvx2StreamReader::GetNumVertices() const
{
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Nvx2StreamReader::GetNumIndices() const
{
    return this->numIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Nvx2StreamReader::GetVertexWidth() const
{
    return this->vertexWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Nvx2StreamReader::GetNumEdges() const
{
    return this->numEdges;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<CoreGraphics::VertexComponent>&
Nvx2StreamReader::GetVertexComponents() const
{
    return this->vertexComponents;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Nvx2StreamReader::SetUsage(CoreGraphics::GpuBufferTypes::Usage usage_)
{
    this->usage = usage_;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::GpuBufferTypes::Usage
Nvx2StreamReader::GetUsage() const
{
    return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
Nvx2StreamReader::SetAccess(CoreGraphics::GpuBufferTypes::Access access_)
{
    this->access = access_;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::GpuBufferTypes::Access
Nvx2StreamReader::GetAccess() const
{
    return this->access;
}

} // namespace Legacy
//------------------------------------------------------------------------------
#endif // NEBULA_LEGACY_SUPPORT
