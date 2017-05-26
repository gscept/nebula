#pragma once
//------------------------------------------------------------------------------
/**
    @file nvx3fileformatstructs.h
    
    NVX3 file format structures.
    
    (C) 2013 Gustav Sterbrant
*/
#include "core/types.h"
#include "base/resourcebase.h"

namespace CoreGraphics
{

#pragma pack(push, 1)

#define NEBULA3_NVX3_MAGICNUMBER 'NVX3'

//------------------------------------------------------------------------------
/** 
    NVX3 file format structs.

    NOTE: keep all header-structs 4-byte aligned!
*/
struct Nvx3Header
{
	uint magic;
	uint numGroups;
	uint numVertices;
	uint vertexWidth;
	uint numIndices;
	uint vertexComponentMask;
	Base::ResourceBase::Usage usage;
	Base::ResourceBase::Access access;
};

struct Nvx3Group
{
	uint primType;
	uint firstVertex;
	uint numVertices;
	uint firstTriangle;
	uint numTriangles;
};

#pragma pack(pop)
} // namespace CoreGraphics   