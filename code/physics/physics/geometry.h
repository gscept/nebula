#pragma once
//------------------------------------------------------------------------------
/**
    Physics::Geometry

    Format agnostic geometry descriptions handed to the physics cooking layer.

    Physics receives plain geometry data and does not know about any file format.
    A project registers a GeometryLoader which fills these descriptions from
    whatever asset format it uses.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/array.h"
#include "util/stringatom.h"
#include "flat/physics/material.h"

#include <functional>

namespace Physics
{

enum class IndexType
{
    InvalidIndexType = 0,
    Index16,
    Index32,
};

//------------------------------------------------------------------------------
/**
    Byte size of an index type
*/
inline SizeT
IndexSize(Physics::IndexType type)
{
    switch (type)
    {
        case Physics::IndexType::Index16: return sizeof(uint16_t);
        case Physics::IndexType::Index32: return sizeof(uint32_t);
        default: return 0;
    }
}

/// Triangle geometry for the cooking layer.
struct MeshGeometryInput
{
    Util::Array<float> positions;           // 3 floats per vertex
    SizeT vertexCount = 0;                  // positions.Size() == vertexCount * 3
    SizeT positionStride = sizeof(float) * 3;

    Util::Array<byte> indices;             // indexCount indices of IndexSize() bytes each, triangle triplets
    SizeT indexCount = 0;
    Physics::IndexType indexType = Physics::IndexType::InvalidIndexType;

    // These can be used for the ApproxSkin topology, where every bone gets its own convex hull.
    Util::Array<float> weights;             // 4 floats per vertex, empty when not skinned
    Util::Array<uint> bones;                // one bone per vertex, empty when not skinned

    Physics::MeshTopology topology = Physics::MeshTopology_Triangles;
};

/// Height field samples for the cooking layer.
struct HeightFieldInput
{
    SizeT width = 0;
    SizeT height = 0;
    Util::Array<ushort> samples; // width * height samples
};

/// Holds function pointers for reading meshes and heightfields from resources.
struct GeometryLoader
{
    std::function<bool(Util::StringAtom resource, IndexT primGroup, MeshGeometryInput& out)> LoadMesh = nullptr;
    std::function<bool(Util::StringAtom resource, HeightFieldInput& out)> LoadHeightField = nullptr;
};

/// register the geometry loader to use
void SetGeometryLoader(const GeometryLoader& loader);
/// get the currently registered geometry loader
const GeometryLoader& GetGeometryLoader();

} // namespace Physics
//------------------------------------------------------------------------------
