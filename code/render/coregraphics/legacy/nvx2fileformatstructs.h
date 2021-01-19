#pragma once
//------------------------------------------------------------------------------
/**
    @file naxfileformatstructs.h
    
    nvx2 file format structures.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace CoreGraphics
{
#pragma pack(push, 1)

#define NEBULA_NVX2_MAGICNUMBER 'NVX2'

//------------------------------------------------------------------------------
/** 
    NVX2 file format structs.

    NOTE: keep all header-structs 4-byte aligned!
*/
struct Nvx2Header
{
    uint magic;
    uint numGroups;
    uint numVertices;
    uint vertexWidth;
    uint numIndices;
    uint numEdges;
    uint vertexComponentMask;
};

struct Nvx2Group
{
    uint firstVertex;
    uint numVertices;
    uint firstTriangle;
    uint numTriangles;
    uint firstEdge;
    uint numEdges;
};

#pragma pack(pop)
} // namespace CoreGraphics   