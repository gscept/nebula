//------------------------------------------------------------------------------
//  @file navmeshbuilder.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navbuild/navmeshbuilder.h"
#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourAlloc.h"
#include "DetourCommon.h"
#include <cfloat>
#include <algorithm>

namespace Navigation
{
namespace Build
{

static_assert(DefaultWalkableArea == RC_WALKABLE_AREA);
static constexpr int MaxVertsPerPoly = 6;
static constexpr int MinTileSize = 8;
static constexpr int MaxTileBits = 14;
/// physics stores height field samples as signed 16 bit, see Physics::CookHeightField
static constexpr float MaxStoredHeight = 32767.0f;

//------------------------------------------------------------------------------
/**
    Forwards Recast errors to the log
*/
class LogContext : public rcContext
{
public:
    LogContext() : rcContext(true) {}

protected:
    void doLog(const rcLogCategory category, const char* msg, const int len) override
    {
        // Recast warns about every dangling triangle it drops from the detail mesh, which is noise for real geometry
        if (category == RC_LOG_ERROR)
        {
            n_warning("Recast: %.*s\n", len, msg);
        }
    }
};

//------------------------------------------------------------------------------
/**
    Triangles of all mesh sources in world space, plus the tiles each triangle touches
*/
struct WorldMeshes
{
    Util::Array<float> verts;
    Util::Array<int> tris;
    /// triangle indices per tile, a triangle is listed in every tile its bounds overlap, including the border
    Util::Array<Util::Array<int>> tileTris;
};

//------------------------------------------------------------------------------
/**
*/
struct TileGrid
{
    float origin[3];
    float bmax[3];
    int tilesX = 0;
    int tilesZ = 0;
    int tileSize = 0;       // in cells
    float tileWorld = 0;    // tileSize * cell size
    float border = 0;       // border in world units
};

//------------------------------------------------------------------------------
/**
*/
static inline Math::vec3
TransformPoint(const Math::mat4& m, float x, float y, float z)
{
    Math::vec4 p = m * Math::vec4(x, y, z, 1.0f);
    return Math::vec3(p.x, p.y, p.z);
}

//------------------------------------------------------------------------------
/**
    A mirroring transform turns the triangles over, Recast uses the winding to tell up from down
*/
static inline bool
FlipsWinding(const Math::mat4& m)
{
    return Math::determinant(m) < 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
static inline float
HeightFieldSampleHeight(const HeightFieldSource& field, SizeT x, SizeT z)
{
    // the same quantization physics uses, the low bit is dropped
    return (float)(field.samples[z * field.width + x] >> 1) * (field.heightRange / MaxStoredHeight);
}

//------------------------------------------------------------------------------
/**
*/
static bool
IsValidHeightField(const HeightFieldSource& field)
{
    return field.width >= 2 && field.height >= 2 && field.samples.Size() == field.width * field.height
        && field.sizeX > 0.0f && field.sizeZ > 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
static Math::bbox
HeightFieldBounds(const HeightFieldSource& field)
{
    ushort minSample = 0xFFFF, maxSample = 0;
    for (IndexT i = 0; i < field.samples.Size(); i++)
    {
        minSample = Math::min(minSample, field.samples[i]);
        maxSample = Math::max(maxSample, field.samples[i]);
    }
    const float scale = field.heightRange / MaxStoredHeight;
    const float minY = (float)(minSample >> 1) * scale;
    const float maxY = (float)(maxSample >> 1) * scale;
    const float maxX = (float)(field.width - 1) * field.sizeX / (float)field.width;
    const float maxZ = (float)(field.height - 1) * field.sizeZ / (float)field.height;

    Math::bbox box;
    box.begin_extend();
    for (int i = 0; i < 8; i++)
    {
        box.extend(TransformPoint(field.transform, (i & 1) ? maxX : 0.0f, (i & 2) ? maxY : minY, (i & 4) ? maxZ : 0.0f));
    }
    return box;
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox
SourceBounds(const BuildInput& input)
{
    Math::bbox box;
    box.begin_extend();
    for (const MeshSource& mesh : input.meshes)
    {
        for (IndexT i = 0; i + 2 < mesh.positions.Size(); i += 3)
        {
            box.extend(TransformPoint(mesh.transform, mesh.positions[i], mesh.positions[i + 1], mesh.positions[i + 2]));
        }
    }
    for (const HeightFieldSource& field : input.heightFields)
    {
        if (IsValidHeightField(field))
        {
            box.extend(HeightFieldBounds(field));
        }
    }
    box.end_extend();
    return box;
}

//------------------------------------------------------------------------------
/**
    Transforms all meshes into one world space triangle soup and sorts the triangles into tiles
*/
static void
SetupWorldMeshes(const BuildInput& input, const TileGrid& grid, WorldMeshes& out)
{
    for (IndexT m = 0; m < input.meshes.Size(); m++)
    {
        const MeshSource& mesh = input.meshes[m];
        const int vertexCount = (int)(mesh.positions.Size() / 3);
        bool valid = mesh.indices.Size() % 3 == 0;
        for (IndexT i = 0; valid && i < mesh.indices.Size(); i++)
        {
            valid = mesh.indices[i] < (uint)vertexCount;
        }
        if (!valid)
        {
            n_warning("BuildNavMesh: mesh source %d has indices which are out of range or not a triangle list, it is skipped\n", m);
            continue;
        }

        const int base = (int)(out.verts.Size() / 3);
        out.verts.Reserve(out.verts.Size() + vertexCount * 3);
        for (int i = 0; i < vertexCount; i++)
        {
            Math::vec3 p = TransformPoint(mesh.transform, mesh.positions[i * 3], mesh.positions[i * 3 + 1], mesh.positions[i * 3 + 2]);
            out.verts.Append(p.x);
            out.verts.Append(p.y);
            out.verts.Append(p.z);
        }

        const bool flip = FlipsWinding(mesh.transform);
        out.tris.Reserve(out.tris.Size() + mesh.indices.Size());
        for (IndexT i = 0; i < mesh.indices.Size(); i += 3)
        {
            out.tris.Append(base + (int)mesh.indices[i]);
            out.tris.Append(base + (int)mesh.indices[i + (flip ? 2 : 1)]);
            out.tris.Append(base + (int)mesh.indices[i + (flip ? 1 : 2)]);
        }
    }

    out.tileTris.Resize(grid.tilesX * grid.tilesZ);
    const int triCount = out.tris.Size() / 3;
    for (int t = 0; t < triCount; t++)
    {
        float minX = FLT_MAX, minZ = FLT_MAX, maxX = -FLT_MAX, maxZ = -FLT_MAX;
        for (int k = 0; k < 3; k++)
        {
            const float* v = &out.verts[out.tris[t * 3 + k] * 3];
            minX = Math::min(minX, v[0]);
            maxX = Math::max(maxX, v[0]);
            minZ = Math::min(minZ, v[2]);
            maxZ = Math::max(maxZ, v[2]);
        }
        const int x0 = (int)floorf((minX - grid.border - grid.origin[0]) / grid.tileWorld);
        const int x1 = (int)floorf((maxX + grid.border - grid.origin[0]) / grid.tileWorld);
        const int z0 = (int)floorf((minZ - grid.border - grid.origin[2]) / grid.tileWorld);
        const int z1 = (int)floorf((maxZ + grid.border - grid.origin[2]) / grid.tileWorld);
        if (x1 < 0 || z1 < 0 || x0 >= grid.tilesX || z0 >= grid.tilesZ)
        {
            continue;
        }
        for (int z = Math::max(z0, 0); z <= Math::min(z1, grid.tilesZ - 1); z++)
        {
            for (int x = Math::max(x0, 0); x <= Math::min(x1, grid.tilesX - 1); x++)
            {
                out.tileTris[z * grid.tilesX + x].Append(t);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Appends the height field triangles which can touch the world space box to verts and tris
*/
static void
AppendHeightFieldTriangles(const HeightFieldSource& field, const float* bmin, const float* bmax, Util::Array<float>& verts, Util::Array<int>& tris)
{
    // find the sample range the box covers by moving its corners into the space of the field
    const Math::mat4 toLocal = Math::inverse(field.transform);
    float minX = FLT_MAX, minZ = FLT_MAX, maxX = -FLT_MAX, maxZ = -FLT_MAX;
    for (int i = 0; i < 8; i++)
    {
        Math::vec3 p = TransformPoint(toLocal, (i & 1) ? bmax[0] : bmin[0], (i & 2) ? bmax[1] : bmin[1], (i & 4) ? bmax[2] : bmin[2]);
        minX = Math::min(minX, p.x);
        maxX = Math::max(maxX, p.x);
        minZ = Math::min(minZ, p.z);
        maxZ = Math::max(maxZ, p.z);
    }

    const float dx = field.sizeX / (float)field.width;
    const float dz = field.sizeZ / (float)field.height;
    const int x0 = Math::max((int)floorf(minX / dx), 0);
    const int x1 = Math::min((int)ceilf(maxX / dx), (int)field.width - 1);
    const int z0 = Math::max((int)floorf(minZ / dz), 0);
    const int z1 = Math::min((int)ceilf(maxZ / dz), (int)field.height - 1);
    if (x0 >= x1 || z0 >= z1)
    {
        return;
    }

    const int base = (int)(verts.Size() / 3);
    const int columns = x1 - x0 + 1;
    verts.Reserve(verts.Size() + columns * (z1 - z0 + 1) * 3);
    for (int z = z0; z <= z1; z++)
    {
        for (int x = x0; x <= x1; x++)
        {
            Math::vec3 p = TransformPoint(field.transform, (float)x * dx, HeightFieldSampleHeight(field, x, z), (float)z * dz);
            verts.Append(p.x);
            verts.Append(p.y);
            verts.Append(p.z);
        }
    }

    // a = (x, z), b = (x, z + 1), c = (x + 1, z), d = (x + 1, z + 1), counter clockwise seen from above
    const bool flip = FlipsWinding(field.transform);
    tris.Reserve(tris.Size() + (columns - 1) * (z1 - z0) * 6);
    for (int z = 0; z < z1 - z0; z++)
    {
        for (int x = 0; x < columns - 1; x++)
        {
            const int a = base + z * columns + x;
            const int b = a + columns;
            const int c = a + 1;
            const int d = b + 1;
            const int cell[6] = { a, b, c, c, b, d };
            for (int k = 0; k < 6; k += 3)
            {
                tris.Append(cell[k]);
                tris.Append(cell[k + (flip ? 2 : 1)]);
                tris.Append(cell[k + (flip ? 1 : 2)]);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Marks walkable triangles and rasterizes them into the heightfield
*/
static bool
RasterizeTriangles(rcContext& ctx, const rcConfig& config, const float* verts, int vertCount, const int* tris, int triCount, Util::Array<unsigned char>& areas, rcHeightfield& solid)
{
    if (triCount == 0)
    {
        return true;
    }
    areas.Clear();
    areas.Resize(triCount);
    memset(areas.Begin(), 0, triCount);
    rcMarkWalkableTriangles(&ctx, config.walkableSlopeAngle, verts, vertCount, tris, triCount, areas.Begin());
    return rcRasterizeTriangles(&ctx, verts, vertCount, tris, areas.Begin(), triCount, solid, config.walkableClimb);
}

enum class TileResult
{
    Built,
    Empty,
    Failed
};

//------------------------------------------------------------------------------
/**
    Owns the Recast intermediates of one tile
*/
struct TileScratch
{
    rcHeightfield* solid = nullptr;
    rcCompactHeightfield* chf = nullptr;
    rcContourSet* cset = nullptr;
    rcPolyMesh* pmesh = nullptr;
    rcPolyMeshDetail* dmesh = nullptr;

    ~TileScratch()
    {
        rcFreeHeightField(this->solid);
        rcFreeCompactHeightfield(this->chf);
        rcFreeContourSet(this->cset);
        rcFreePolyMesh(this->pmesh);
        rcFreePolyMeshDetail(this->dmesh);
    }
};

//------------------------------------------------------------------------------
/**
*/
static TileResult
BuildTile(rcContext& ctx, const BuildInput& input, const Util::Array<IndexT>& areaOrder, const rcConfig& baseConfig, const TileGrid& grid, const WorldMeshes& meshes, int tx, int tz, Tile& tile, BuildStats& stats)
{
    rcConfig config = baseConfig;
    config.bmin[0] = grid.origin[0] + tx * grid.tileWorld - grid.border;
    config.bmin[1] = grid.origin[1];
    config.bmin[2] = grid.origin[2] + tz * grid.tileWorld - grid.border;
    config.bmax[0] = grid.origin[0] + (tx + 1) * grid.tileWorld + grid.border;
    config.bmax[1] = grid.bmax[1];
    config.bmax[2] = grid.origin[2] + (tz + 1) * grid.tileWorld + grid.border;

    TileScratch s;
    s.solid = rcAllocHeightfield();
    if (s.solid == nullptr || !rcCreateHeightfield(&ctx, *s.solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
    {
        return TileResult::Failed;
    }

    Util::Array<unsigned char> areas;
    bool anyGeometry = false;

    // mesh triangles listed for this tile
    const Util::Array<int>& tileTris = meshes.tileTris[tz * grid.tilesX + tx];
    if (!tileTris.IsEmpty())
    {
        Util::Array<int> tris;
        tris.Reserve(tileTris.Size() * 3);
        for (int t : tileTris)
        {
            tris.Append(meshes.tris[t * 3]);
            tris.Append(meshes.tris[t * 3 + 1]);
            tris.Append(meshes.tris[t * 3 + 2]);
        }
        if (!RasterizeTriangles(ctx, config, meshes.verts.Begin(), meshes.verts.Size() / 3, tris.Begin(), tileTris.Size(), areas, *s.solid))
        {
            return TileResult::Failed;
        }
        stats.inputTriangles += tileTris.Size();
        anyGeometry = true;
    }

    // height field triangles are generated for the tile only, a whole terrain doesn't have to exist as triangles at once
    for (const HeightFieldSource& field : input.heightFields)
    {
        if (!IsValidHeightField(field))
        {
            continue;
        }
        Util::Array<float> verts;
        Util::Array<int> tris;
        AppendHeightFieldTriangles(field, config.bmin, config.bmax, verts, tris);
        const int triCount = tris.Size() / 3;
        if (triCount == 0)
        {
            continue;
        }
        if (!RasterizeTriangles(ctx, config, verts.Begin(), verts.Size() / 3, tris.Begin(), triCount, areas, *s.solid))
        {
            return TileResult::Failed;
        }
        stats.inputTriangles += triCount;
        anyGeometry = true;
    }

    if (!anyGeometry)
    {
        return TileResult::Empty;
    }

    // remove overhangs from conservative rasterization and spans the agent can't stand on
    rcFilterLowHangingWalkableObstacles(&ctx, config.walkableClimb, *s.solid);
    rcFilterLedgeSpans(&ctx, config.walkableHeight, config.walkableClimb, *s.solid);
    rcFilterWalkableLowHeightSpans(&ctx, config.walkableHeight, *s.solid);

    s.chf = rcAllocCompactHeightfield();
    if (s.chf == nullptr || !rcBuildCompactHeightfield(&ctx, config.walkableHeight, config.walkableClimb, *s.solid, *s.chf))
    {
        return TileResult::Failed;
    }
    rcFreeHeightField(s.solid);
    s.solid = nullptr;

    if (!rcErodeWalkableArea(&ctx, config.walkableRadius, *s.chf))
    {
        return TileResult::Failed;
    }

    for (IndexT index : areaOrder)
    {
        const AreaVolume& volume = input.areas[index];
        if (volume.shape == AreaVolume::Cylinder)
        {
            if (volume.radius <= 0.0f || volume.maxY <= volume.minY)
            {
                continue;
            }
            const float base[3] = { volume.center.x, volume.minY, volume.center.z };
            rcMarkCylinderArea(&ctx, base, volume.radius, volume.maxY - volume.minY, volume.area, *s.chf);
            continue;
        }
        if (volume.points.Size() < 3)
        {
            continue;
        }
        Util::Array<float> points;
        points.Reserve(volume.points.Size() * 3);
        for (const Math::vec3& p : volume.points)
        {
            points.Append(p.x);
            points.Append(p.y);
            points.Append(p.z);
        }
        rcMarkConvexPolyArea(&ctx, points.Begin(), volume.points.Size(), volume.minY, volume.maxY, volume.area, *s.chf);
    }

    // watershed partitioning, the border keeps polygons out of the area shared with the neighbour tiles
    if (!rcBuildDistanceField(&ctx, *s.chf)
        || !rcBuildRegions(&ctx, *s.chf, config.borderSize, config.minRegionArea, config.mergeRegionArea))
    {
        return TileResult::Failed;
    }

    s.cset = rcAllocContourSet();
    if (s.cset == nullptr || !rcBuildContours(&ctx, *s.chf, config.maxSimplificationError, config.maxEdgeLen, *s.cset))
    {
        return TileResult::Failed;
    }
    if (s.cset->nconts == 0)
    {
        return TileResult::Empty;
    }

    s.pmesh = rcAllocPolyMesh();
    if (s.pmesh == nullptr || !rcBuildPolyMesh(&ctx, *s.cset, config.maxVertsPerPoly, *s.pmesh))
    {
        return TileResult::Failed;
    }
    s.dmesh = rcAllocPolyMeshDetail();
    if (s.dmesh == nullptr || !rcBuildPolyMeshDetail(&ctx, *s.pmesh, *s.chf, config.detailSampleDist, config.detailSampleMaxError, *s.dmesh))
    {
        return TileResult::Failed;
    }
    if (s.pmesh->npolys == 0)
    {
        return TileResult::Empty;
    }
    if (s.pmesh->nverts >= 0xffff)
    {
        n_warning("BuildNavMesh: tile (%d, %d) has %d vertices, Detour supports %d. Use a smaller tile size\n", tx, tz, s.pmesh->nverts, 0xffff);
        return TileResult::Failed;
    }

    // Detour keeps the Recast areas, the flags come from the area
    for (int i = 0; i < s.pmesh->npolys; i++)
    {
        s.pmesh->flags[i] = input.areaFlags[s.pmesh->areas[i] & RC_WALKABLE_AREA];
    }

    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = s.pmesh->verts;
    params.vertCount = s.pmesh->nverts;
    params.polys = s.pmesh->polys;
    params.polyAreas = s.pmesh->areas;
    params.polyFlags = s.pmesh->flags;
    params.polyCount = s.pmesh->npolys;
    params.nvp = s.pmesh->nvp;
    params.detailMeshes = s.dmesh->meshes;
    params.detailVerts = s.dmesh->verts;
    params.detailVertsCount = s.dmesh->nverts;
    params.detailTris = s.dmesh->tris;
    params.detailTriCount = s.dmesh->ntris;
    params.walkableHeight = input.agent.agent_height;
    params.walkableRadius = input.agent.agent_radius;
    params.walkableClimb = input.agent.agent_max_climb;
    params.tileX = tx;
    params.tileY = tz;
    params.tileLayer = 0;
    rcVcopy(params.bmin, s.pmesh->bmin);
    rcVcopy(params.bmax, s.pmesh->bmax);
    params.cs = config.cs;
    params.ch = config.ch;
    params.buildBvTree = true;

    unsigned char* navData = nullptr;
    int navDataSize = 0;
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
    {
        n_warning("BuildNavMesh: Detour could not create the data of tile (%d, %d)\n", tx, tz);
        return TileResult::Failed;
    }

    tile.x = tx;
    tile.y = tz;
    tile.layer = 0;
    tile.data.Set(navData, navDataSize);
    dtFree(navData);
    stats.polygons += s.pmesh->npolys;
    return TileResult::Built;
}

//------------------------------------------------------------------------------
/**
*/
static void
SetupConfig(const BuildInput& input, int tileSize, rcConfig& config)
{
    const NavMeshSettingsT& settings = input.settings;
    const AgentT& agent = input.agent;

    memset(&config, 0, sizeof(rcConfig));
    config.cs = settings.cell_size;
    config.ch = settings.cell_height;
    config.walkableSlopeAngle = settings.max_slope;
    config.walkableHeight = (int)ceilf(agent.agent_height / config.ch);
    config.walkableClimb = (int)floorf(agent.agent_max_climb / config.ch);
    config.walkableRadius = (int)ceilf(agent.agent_radius / config.cs);
    config.maxEdgeLen = (int)(settings.max_edge_length / config.cs);
    config.maxSimplificationError = settings.max_edge_error;
    config.minRegionArea = settings.region_min_size * settings.region_min_size;
    config.mergeRegionArea = settings.region_merge_size * settings.region_merge_size;
    config.maxVertsPerPoly = MaxVertsPerPoly;
    config.tileSize = tileSize;
    // the border has to fit the erosion by the agent radius plus a few cells for the filters
    config.borderSize = config.walkableRadius + 3;
    config.width = config.tileSize + config.borderSize * 2;
    config.height = config.tileSize + config.borderSize * 2;
    config.detailSampleDist = settings.detail_sample_dist < 0.9f ? 0 : config.cs * settings.detail_sample_dist;
    config.detailSampleMaxError = config.ch * settings.detail_sample_max_error;
}

//------------------------------------------------------------------------------
/**
*/
bool
BuildNavMesh(const BuildInput& input, BuildOutput& output, BuildStats* outStats)
{
    output.tiles.Clear();
    memset(&output.params, 0, sizeof(output.params));
    BuildStats stats;

    if (input.settings.cell_size <= 0.0f || input.settings.cell_height <= 0.0f)
    {
        n_warning("BuildNavMesh: cell size and cell height must be positive\n");
        return false;
    }
    for (const AreaVolume& volume : input.areas)
    {
        if (volume.area > DefaultWalkableArea)
        {
            n_warning("BuildNavMesh: area %d is out of range, areas go from 0 to %d\n", volume.area, DefaultWalkableArea);
            return false;
        }
    }

    Math::bbox bounds = input.bounds;
    if (input.useSourceBounds)
    {
        bounds = SourceBounds(input);
        // room for the agent to stand on the highest surface
        bounds.pmin.y -= input.settings.cell_height;
        bounds.pmax.y += input.agent.agent_height + input.settings.cell_height;
    }
    if (bounds.pmax.x <= bounds.pmin.x || bounds.pmax.z <= bounds.pmin.z)
    {
        n_warning("BuildNavMesh: the build bounds are empty\n");
        return false;
    }

    const int tileSize = Math::max(input.settings.tile_size, MinTileSize);
    rcConfig config;
    SetupConfig(input, tileSize, config);

    TileGrid grid;
    grid.origin[0] = bounds.pmin.x;
    grid.origin[1] = bounds.pmin.y;
    grid.origin[2] = bounds.pmin.z;
    grid.bmax[0] = bounds.pmax.x;
    grid.bmax[1] = bounds.pmax.y;
    grid.bmax[2] = bounds.pmax.z;
    int gridWidth = 0, gridHeight = 0;
    rcCalcGridSize(grid.origin, grid.bmax, config.cs, &gridWidth, &gridHeight);
    grid.tileSize = tileSize;
    grid.tileWorld = tileSize * config.cs;
    grid.tilesX = (gridWidth + tileSize - 1) / tileSize;
    grid.tilesZ = (gridHeight + tileSize - 1) / tileSize;
    grid.border = config.borderSize * config.cs;
    stats.tilesX = grid.tilesX;
    stats.tilesZ = grid.tilesZ;

    // with 32 bit poly refs the tile and polygon indices share 22 bits
    const int tileBits = (int)dtIlog2(dtNextPow2((unsigned int)(grid.tilesX * grid.tilesZ)));
    if (tileBits > MaxTileBits)
    {
        n_warning("BuildNavMesh: %d x %d tiles is more than Detour can address, use a larger tile size or cell size\n", grid.tilesX, grid.tilesZ);
        return false;
    }
    const int polyBits = 22 - tileBits;
    dtVcopy(output.params.orig, grid.origin);
    output.params.tileWidth = grid.tileWorld;
    output.params.tileHeight = grid.tileWorld;
    output.params.maxTiles = 1 << tileBits;
    output.params.maxPolys = 1 << polyBits;

    WorldMeshes meshes;
    SetupWorldMeshes(input, grid, meshes);

    // volumes of equal priority keep the order they were given in
    Util::Array<IndexT> areaOrder;
    for (IndexT i = 0; i < input.areas.Size(); i++)
    {
        areaOrder.Append(i);
    }
    std::stable_sort(areaOrder.begin(), areaOrder.end(),
        [&input](IndexT a, IndexT b)
        {
            return input.areas[a].priority < input.areas[b].priority;
        });

    LogContext ctx;
    for (int tz = 0; tz < grid.tilesZ; tz++)
    {
        for (int tx = 0; tx < grid.tilesX; tx++)
        {
            Tile tile;
            switch (BuildTile(ctx, input, areaOrder, config, grid, meshes, tx, tz, tile, stats))
            {
                case TileResult::Built:
                    output.tiles.Append(std::move(tile));
                    stats.builtTiles++;
                    break;
                case TileResult::Failed:
                    n_warning("BuildNavMesh: tile (%d, %d) failed\n", tx, tz);
                    stats.failedTiles++;
                    break;
                case TileResult::Empty:
                    break;
            }
        }
    }

    if (outStats != nullptr)
    {
        *outStats = stats;
    }
    if (stats.builtTiles == 0 && stats.failedTiles == 0)
    {
        n_warning("BuildNavMesh: no walkable surface found\n");
    }
    return stats.builtTiles > 0 && stats.failedTiles == 0;
}

//------------------------------------------------------------------------------
/**
*/
dtNavMesh*
CreateDetourNavMesh(const BuildOutput& output)
{
    dtNavMesh* navMesh = dtAllocNavMesh();
    if (navMesh == nullptr)
    {
        return nullptr;
    }
    if (dtStatusFailed(navMesh->init(&output.params)))
    {
        dtFreeNavMesh(navMesh);
        return nullptr;
    }
    for (const Tile& tile : output.tiles)
    {
        // Detour writes links into the tile data and frees it with the tile
        unsigned char* data = (unsigned char*)dtAlloc(tile.data.Size(), DT_ALLOC_PERM);
        Memory::Copy(tile.data.GetPtr(), data, tile.data.Size());
        if (dtStatusFailed(navMesh->addTile(data, (int)tile.data.Size(), DT_TILE_FREE_DATA, 0, nullptr)))
        {
            n_warning("CreateDetourNavMesh: tile (%d, %d) could not be added\n", tile.x, tile.y);
            dtFree(data);
            dtFreeNavMesh(navMesh);
            return nullptr;
        }
    }
    return navMesh;
}

} // namespace Build
} // namespace Navigation
