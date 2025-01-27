#pragma once
//------------------------------------------------------------------------------
/**
    @file nvx3fileformatstructs.h
    
    NVX3 file format structures.
    
    (C) 2013 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coregraphics/indextype.h"
#include "coregraphics/gpubuffertypes.h"
#include "coregraphics/vertexlayout.h"

namespace CoreGraphics
{

#define NEBULA_NVX_MAGICNUMBER 'NVX3'

#pragma pack(push, 1)

//------------------------------------------------------------------------------
/** 
    NVX3 file format structs.
*/
struct Nvx3Header
{
    uint magic;
    uint meshDataOffset;
    uint numMeshes;         // The number of Nvx3Mesh structs
    uint meshletDataOffset;
    uint numMeshlets;
    uint vertexDataOffset;
    uint vertexDataSize;
    uint indexDataOffset;
    uint indexDataSize;     // The total byte size of the index data for all meshes
};

struct Nvx3VertexRange
{
    uint indexByteOffset;
    uint baseVertexByteOffset;
    uint attributesVertexByteOffset;
    uint firstGroupOffset;
    uint numGroups;
    CoreGraphics::IndexType::Code indexType;
    CoreGraphics::VertexLayoutType layout;
};

struct Nvx3Group
{
    uint primType;
    uint firstIndex;
    uint numIndices;

    uint firstMeshlet;              // Offset to first meshlet (optional)
    uint numMeshlets;               // Number of meshlets for this primitive group
};

struct Nvx3Meshlet
{
    uint indexOffset;
    uint firstIndex;
    uint numIndices;
};


struct Nvx3Elements
{
    CoreGraphics::Nvx3VertexRange* ranges;
    ubyte* vertexData;
    ubyte* indexData;
    CoreGraphics::Nvx3Meshlet* meshlets;
};

namespace Nvx3
{
    void FillNvx3Elements(char* basePointer, Nvx3Header* header, Nvx3Elements& OutElements);
};

#pragma pack(pop)



namespace Nvx3
{

//------------------------------------------------------------------------------
/**
*/
inline void 
FillNvx3Elements(char* basePointer, Nvx3Header* header, Nvx3Elements& OutElements)
{
    n_assert(header != nullptr);
    OutElements.ranges = (CoreGraphics::Nvx3VertexRange*)(basePointer + header->meshDataOffset);
    OutElements.vertexData = (ubyte*)(basePointer + header->vertexDataOffset);
    OutElements.indexData = (ubyte*)(basePointer + header->indexDataOffset);
    OutElements.meshlets = (CoreGraphics::Nvx3Meshlet*)(basePointer + header->meshletDataOffset);
}

} // namespace Nvx3

} // namespace CoreGraphics   
