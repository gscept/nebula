//------------------------------------------------------------------------------
//  nvx2streamreader.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/nvx3streamreader.h"
#include "coregraphics/memoryvertexbufferloader.h"
#include "coregraphics/memoryindexbufferloader.h"
#include "coregraphics/nvx3fileformatstructs.h"

namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Nvx3StreamReader, 'N3SR', IO::StreamReader);

using namespace CoreGraphics;
using namespace Util;
using namespace Math;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
Nvx3StreamReader::Nvx3StreamReader() :
    usage(Base::ResourceBase::UsageImmutable),
    access(Base::ResourceBase::AccessNone),
    rawMode(false),
    mapPtr(0),
    groupDataPtr(0),
    vertexDataPtr(0),
    indexDataPtr(0),
    groupDataSize(0),
    vertexDataSize(0),
    indexDataSize(0),
    numGroups(0),
    numVertices(0),
    vertexWidth(0),
    numIndices(0),
    vertexComponentMask(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Nvx3StreamReader::~Nvx3StreamReader()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
Nvx3StreamReader::Open()
{
    n_assert(0 == this->primGroups.Size());
    n_assert(this->stream->CanBeMapped());
    n_assert(this->vertexComponents.IsEmpty());
    n_assert(0 == this->mapPtr);
    if (StreamReader::Open())
    {
        // map the stream to memory
        this->mapPtr = this->stream->Map();
        n_assert(0 != this->mapPtr);

        // read data
        this->ReadHeaderData();
        this->ReadPrimitiveGroups();
        this->SetupVertexComponents();
        if (!this->rawMode)
        {
            this->SetupVertexBuffer();
            this->SetupIndexBuffer();
            this->UpdateGroupBoundingBoxes();
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
Nvx3StreamReader::Close()
{
    this->mapPtr = 0;
    this->groupDataPtr = 0;
    this->vertexDataPtr = 0;
    this->indexDataPtr = 0;
    this->vertexBuffer = 0;
    this->indexBuffer = 0;
	this->usage = Base::ResourceBase::UsageImmutable;
	this->access = Base::ResourceBase::AccessNone;
    this->primGroups.Clear();
    this->vertexComponents.Clear();
    stream->Unmap();
    StreamReader::Close();
}

//------------------------------------------------------------------------------
/**
    This reads the nvx3 header data and checks whether the file is 
    actually an nvx3 file through the magic number. All header data
    will be read into member variables, and pointers to the
    start of the group-, vertex- and index-data will be setup.

    NOTE: we assume that the file is in the correct byte order!
*/
void
Nvx3StreamReader::ReadHeaderData()
{
    n_assert(0 != this->mapPtr);
    
    // endian-convert header
    struct Nvx3Header* header = (struct Nvx3Header*) this->mapPtr;
    header->numIndices *= 3; // header holds number of tris, not indices

    // check magic number
    if (FourCC(header->magic) != FourCC('NVX3'))
    {
        // not an nvx3 file, break hard
        n_error("Nvx3StreamReader: '%s' is not a nvx3 file!", this->stream->GetURI().AsString().AsCharPtr());
    }    
    this->numGroups = header->numGroups;
    this->numVertices = header->numVertices;
    this->vertexWidth = header->vertexWidth;
    this->numIndices = header->numIndices;
	this->usage = header->usage;
	this->access = header->access;
    this->vertexComponentMask = header->vertexComponentMask;
    this->groupDataSize = 6 * sizeof(uint) * this->numGroups;
    this->vertexDataSize = this->numVertices * this->vertexWidth * sizeof(float);
    this->indexDataSize = this->numIndices * sizeof(ushort);
    this->groupDataPtr = header + 1;
    this->vertexDataPtr = ((uchar*)this->groupDataPtr) + this->groupDataSize;
    this->indexDataPtr = ((uchar*)this->vertexDataPtr) + this->vertexDataSize;
}

//------------------------------------------------------------------------------
/**
*/
void
Nvx3StreamReader::ReadPrimitiveGroups()
{
    n_assert(this->primGroups.IsEmpty());
    n_assert(this->numGroups > 0);
    n_assert(0 != this->groupDataPtr);
    Nvx3Group* group = (Nvx3Group*) this->groupDataPtr;
    IndexT i;
    for (i = 0; i < (SizeT)this->numGroups; i++)
    {
        // setup a primitive group object
        PrimitiveGroup primGroup;
        primGroup.SetBaseVertex(group->firstVertex);
        primGroup.SetNumVertices(group->numVertices);
        primGroup.SetBaseIndex(group->firstTriangle * 3);
        primGroup.SetNumIndices(group->numTriangles * 3);
        primGroup.SetPrimitiveTopology(group->primType);
        this->primGroups.Append(primGroup);

        // set top next group
        group++;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Nvx3StreamReader::SetupVertexComponents()
{
    n_assert(this->vertexComponents.IsEmpty());

    IndexT i;
    for (i = 0; i < N3NumVertexComponents; i++)
    {
        VertexComponent::SemanticName sem;
        VertexComponent::Format fmt;
        IndexT index = 0;
        if (vertexComponentMask & (1<<i))
        {
            switch (1<<i)
            {
                case N3Coord:        sem = VertexComponent::Position;     fmt = VertexComponent::Float3; break;
                case N3Normal:       sem = VertexComponent::Normal;       fmt = VertexComponent::Float3; break;
                case N3NormalUB4N:   sem = VertexComponent::Normal;       fmt = VertexComponent::UByte4N; break;
                case N3Uv0:          sem = VertexComponent::TexCoord;     fmt = VertexComponent::Float2; index = 0; break;
                case N3Uv0S2:        sem = VertexComponent::TexCoord;     fmt = VertexComponent::Short2; index = 0; break;
                case N3Uv1:          sem = VertexComponent::TexCoord;     fmt = VertexComponent::Float2; index = 1; break;
                case N3Uv1S2:        sem = VertexComponent::TexCoord;     fmt = VertexComponent::Short2; index = 1; break;
                case N3Uv2:          sem = VertexComponent::TexCoord;     fmt = VertexComponent::Float2; index = 2; break;
                case N3Uv2S2:        sem = VertexComponent::TexCoord;     fmt = VertexComponent::Short2; index = 2; break;
                case N3Uv3:          sem = VertexComponent::TexCoord;     fmt = VertexComponent::Float2; index = 3; break;
                case N3Uv3S2:        sem = VertexComponent::TexCoord;     fmt = VertexComponent::Short2; index = 3; break;
                case N3Color:        sem = VertexComponent::Color;        fmt = VertexComponent::Float4; break;
                case N3ColorUB4N:    sem = VertexComponent::Color;        fmt = VertexComponent::UByte4N; break;
                case N3Tangent:      sem = VertexComponent::Tangent;      fmt = VertexComponent::Float3; break;
                case N3TangentUB4N:  sem = VertexComponent::Tangent;      fmt = VertexComponent::UByte4N; break;
                case N3Binormal:     sem = VertexComponent::Binormal;     fmt = VertexComponent::Float3; break;
                case N3BinormalUB4N: sem = VertexComponent::Binormal;     fmt = VertexComponent::UByte4N; break;
                case N3Weights:      sem = VertexComponent::SkinWeights;  fmt = VertexComponent::Float4; break;
                case N3WeightsUB4N:  sem = VertexComponent::SkinWeights;  fmt = VertexComponent::UByte4N; break;
                case N3JIndices:     sem = VertexComponent::SkinJIndices; fmt = VertexComponent::Float4; break;
                case N3JIndicesUB4:  sem = VertexComponent::SkinJIndices; fmt = VertexComponent::UByte4; break;
                default:
                    n_error("Invalid Nebula VertexComponent in Nvx3StreamReader::SetupVertexComponents");
                    sem = VertexComponent::Position;
                    fmt = VertexComponent::Float3;
                    break;
            }
            this->vertexComponents.Append(VertexComponent(sem, index, fmt));
        }
    }
}

//------------------------------------------------------------------------------
/**
    Since nvx2 files don't contain any bounding box information
    we need to compute per-primitive-group bounding boxes
    manually by walking the triangle indices. This may be inefficient
    with large meshes.
*/
void
Nvx3StreamReader::UpdateGroupBoundingBoxes()
{
    n_assert(0 != this->vertexDataPtr);
    n_assert(0 != this->indexDataPtr);
    n_assert(this->primGroups.Size() > 0);

    float* vertexPtr = (float*) this->vertexDataPtr;
    ushort* indexPtr = (ushort*) this->indexDataPtr;
    IndexT groupIndex;
    for (groupIndex = 0; groupIndex < this->primGroups.Size(); groupIndex++)
    {
        PrimitiveGroup& group = this->primGroups[groupIndex];        
        bbox box;
        box.begin_extend();
        point p;
        IndexT ii;
        for (ii = 0; ii < group.GetNumIndices(); ii++)
        {
            float* curVertexPtr = vertexPtr + (indexPtr[ii + group.GetBaseIndex()] * this->vertexWidth);
            p.set(curVertexPtr[0], curVertexPtr[1], curVertexPtr[2]);
            box.extend(p);
        }
        group.SetBoundingBox(box);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Nvx3StreamReader::SetupVertexBuffer()
{
    n_assert(!this->rawMode);
    n_assert(0 != this->vertexDataPtr);
    n_assert(this->vertexDataSize > 0);
    n_assert(this->numVertices > 0);    
    n_assert(this->vertexComponents.Size() > 0);

    // setup new vertex buffer
    if (!this->vertexBuffer.isvalid())
    {
        this->vertexBuffer = VertexBuffer::Create();
    }
    if (!this->vertexBufferLoader.isvalid())
    {
        this->vertexBufferLoader = MemoryVertexBufferLoader::Create();
    }
    this->vertexBufferLoader->Setup(this->vertexComponents, this->numVertices, this->vertexDataPtr, 
                                    this->vertexDataSize, this->usage, this->access);
    this->vertexBuffer->SetLoader(this->vertexBufferLoader.upcast<ResourceLoader>());
    this->vertexBuffer->Load();
    this->vertexBuffer->SetLoader(0);
    this->vertexBufferLoader = 0;
    n_assert(this->vertexBuffer->GetState() == VertexBuffer::Loaded);
}

//------------------------------------------------------------------------------
/**
*/
void
Nvx3StreamReader::SetupIndexBuffer()
{
    n_assert(!this->rawMode);
    n_assert(0 != this->indexDataPtr);
    n_assert(this->indexDataSize > 0);
    n_assert(this->numIndices > 0);
    
    // setup a new index buffer
    if (!this->indexBuffer.isvalid())
    {
        this->indexBuffer = IndexBuffer::Create();
    }
    if (!this->indexBufferLoader.isvalid())
    {
        this->indexBufferLoader = MemoryIndexBufferLoader::Create();
    }
    this->indexBufferLoader->Setup(IndexType::Index16, this->numIndices, this->indexDataPtr, 
                                   this->indexDataSize, this->usage, this->access);
    this->indexBuffer->SetLoader(this->indexBufferLoader.upcast<ResourceLoader>());
    this->indexBuffer->Load();
    this->indexBuffer->SetLoader(0);
    this->indexBufferLoader = 0;
    n_assert(this->indexBuffer->GetState() == IndexBuffer::Loaded);
}

} // namespace CoreGraphics

