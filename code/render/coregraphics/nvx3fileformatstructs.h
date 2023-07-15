#pragma once
//------------------------------------------------------------------------------
/**
    @file nvx3fileformatstructs.h
    
    NVX3 file format structures.
    
    (C) 2013 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
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
    uint numMeshes;         // The number of Nvx3Mesh structs
    uint numGroups;
    uint numMeshlets;
    uint indexDataSize;     // The total byte size of the index data for all meshes
    uint vertexDataSize;
};

struct Nvx3VertexRange
{
    uint indexByteOffset;
    uint baseVertexByteOffset;
    uint attributesVertexByteOffset;
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
    CoreGraphics::Nvx3Group* groups;
    ubyte* vertexData;
    ubyte* indexData;
    CoreGraphics::Nvx3Meshlet* meshlets;
};

namespace Nvx3
{
    void FillNvx3Elements(Nvx3Header* header, Nvx3Elements& OutElements);
};

#pragma pack(pop)



namespace Nvx3
{

//------------------------------------------------------------------------------
/**
*/
inline void 
FillNvx3Elements(Nvx3Header* header, Nvx3Elements& OutElements)
{
    n_assert(header != nullptr);
    OutElements.ranges = (CoreGraphics::Nvx3VertexRange*)(header + 1);
    OutElements.groups = (CoreGraphics::Nvx3Group*)(OutElements.ranges + header->numMeshes);
    OutElements.vertexData = (ubyte*)(OutElements.groups + header->numGroups);
    OutElements.indexData = (ubyte*)(OutElements.vertexData + header->vertexDataSize);
    OutElements.meshlets = (CoreGraphics::Nvx3Meshlet*)(OutElements.indexData + header->indexDataSize);
}

} // namespace Nvx3

} // namespace CoreGraphics   
