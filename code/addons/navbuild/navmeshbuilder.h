#pragma once
//------------------------------------------------------------------------------
/**
    Navigation::Build

    Builds tiled Detour navmesh data from world space geometry.

    The builder is format agnostic: it takes geometry which is already loaded, so it
    can run in the editor, the asset batcher or a test without any resource system.
    Use the functions in navbuild/geometrysources.h to fill sources from URNs.

    Every source carries the world transform it is placed with. Height fields follow
    the physics height field convention exactly, so the navmesh lies on the collision
    surface: the origin is the corner of the field, sample (x, z) sits at local
    (x * sizeX / width, height, z * sizeZ / height) and the height is
    (sample >> 1) * heightRange / 32767.

    The result is always tiled. A tile covers settings.tile_size cells along x and z,
    tiles without any walkable surface are left out.

    Areas are Recast areas: 0 is not walkable, 63 (RC_WALKABLE_AREA) is the default
    walkable area and anything in between can be assigned with area volumes. Detour
    polygons keep the Recast area id and get the flags BuildInput::areaFlags lists for it.

    Area volumes are applied after the surface is eroded by the agent radius, in the order
    of their priority, so where volumes overlap the one with the higher priority wins.
    Convex prisms come from NavArea entities, cylinders are the stamps of painted areas.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/array.h"
#include "util/blob.h"
#include "math/mat4.h"
#include "math/bbox.h"
#include "math/vec3.h"
#include "flat/navigation/navmeshsettings.h"
#include "flat/navigation/agentkinds.h"
#include "DetourNavMesh.h"

namespace Navigation
{
namespace Build
{

/// the area of walkable surface which no area volume covers, Recast's RC_WALKABLE_AREA
static constexpr unsigned char DefaultWalkableArea = 63;

/// polygon flags written into the Detour data
enum PolyFlags : unsigned short
{
    PolyFlag_Walk = 1 << 0,
};

/// triangle geometry placed in the world
struct MeshSource
{
    Util::Array<float> positions;   // 3 floats per vertex, in the space of the source
    Util::Array<uint> indices;      // triangle list, counter clockwise seen from the walkable side
    Math::mat4 transform;           // source to world
};

/// height field placed in the world, laid out like Physics::HeightFieldInput
struct HeightFieldSource
{
    SizeT width = 0;                // samples along x
    SizeT height = 0;               // samples along z
    Util::Array<ushort> samples;    // samples[z * width + x], 0 is the bottom and 65535 the top of the height range
    float heightRange = 1.0f;       // vertical span in meters
    float sizeX = 1.0f;             // world extent along x
    float sizeZ = 1.0f;             // world extent along z
    Math::mat4 transform;           // field to world, the origin is the corner of the field
};

/// assigns an area to the walkable surface inside it
struct AreaVolume
{
    enum Shape
    {
        ConvexPrism,                // points, between minY and maxY
        Cylinder                    // center and radius, between minY and maxY
    };
    Shape shape = ConvexPrism;
    Util::Array<Math::vec3> points; // convex outline in world space, only x and z are used
    Math::vec3 center;              // cylinder axis, only x and z are used
    float radius = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
    unsigned char area = 0;         // Recast area, 0 cuts the surface out
    int priority = 0;               // volumes are applied from low to high priority
};

/// everything a build needs
struct BuildInput
{
    Navigation::NavMeshSettingsT settings;
    Navigation::AgentT agent;
    /// build in the bounds of all sources, padded by the agent height, instead of bounds
    bool useSourceBounds = true;
    /// the world space volume the navmesh is built in
    Math::bbox bounds;
    Util::Array<MeshSource> meshes;
    Util::Array<HeightFieldSource> heightFields;
    Util::Array<AreaVolume> areas;
    /// Detour polygon flags of every area id
    unsigned short areaFlags[64];

    BuildInput()
    {
        // every area is walkable unless an area table says otherwise
        for (int i = 0; i < 64; i++)
        {
            this->areaFlags[i] = i == 0 ? 0 : PolyFlag_Walk;
        }
    }
};

/// one tile of Detour data, as dtCreateNavMeshData produced it
struct Tile
{
    int x = 0;
    int y = 0;
    int layer = 0;
    Util::Blob data;
};

/// the built navmesh
struct BuildOutput
{
    dtNavMeshParams params;
    Util::Array<Tile> tiles;
};

/// statistics of a build
struct BuildStats
{
    int tilesX = 0;
    int tilesZ = 0;
    int builtTiles = 0;         // tiles with walkable surface
    int failedTiles = 0;        // tiles which hit a Recast or Detour limit
    int inputTriangles = 0;     // mesh and height field triangles which were rasterized, counted once per tile
    int polygons = 0;
};

/// return the world bounds of all sources
Math::bbox SourceBounds(const BuildInput& input);
/// build the navmesh, returns false if nothing walkable was found or a tile failed
bool BuildNavMesh(const BuildInput& input, BuildOutput& output, BuildStats* stats = nullptr);
/// create a Detour navmesh from built data, the tile data is copied. Free with dtFreeNavMesh
dtNavMesh* CreateDetourNavMesh(const BuildOutput& output);

} // namespace Build
} // namespace Navigation
