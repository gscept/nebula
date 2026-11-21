//------------------------------------------------------------------------------
//  @file navbuildtest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "navbuildtest.h"

#include "navbuild/navmeshbuilder.h"
#include "navbuild/geometrysources.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

using namespace Navigation::Build;

namespace Test
{
__ImplementClass(Test::NavBuildTest, 'NVBT', Test::TestCase);

//------------------------------------------------------------------------------
/**
    A flat square from (0, 0) to (size, size), counter clockwise seen from above
*/
static MeshSource
MakeQuad(float size, bool reversed = false)
{
    MeshSource quad;
    const float verts[] = { 0, 0, 0,   0, 0, size,   size, 0, 0,   size, 0, size };
    quad.positions.AppendArray(verts, 12);
    const uint up[] = { 0, 1, 2,   2, 1, 3 };
    const uint down[] = { 0, 2, 1,   2, 3, 1 };
    quad.indices.AppendArray(reversed ? down : up, 6);
    quad.transform = Math::mat4::identity;
    return quad;
}

//------------------------------------------------------------------------------
/**
*/
static BuildInput
MakeInput()
{
    BuildInput input;
    input.agent.agent_height = 2.0f;
    input.agent.agent_radius = 0.5f;
    input.agent.agent_max_climb = 0.5f;
    return input;
}

//------------------------------------------------------------------------------
/**
    Owns a Detour navmesh and a query on it
*/
struct NavQuery
{
    dtNavMesh* mesh = nullptr;
    dtNavMeshQuery* query = nullptr;
    dtQueryFilter filter;

    explicit NavQuery(const BuildOutput& output)
    {
        this->mesh = CreateDetourNavMesh(output);
        if (this->mesh != nullptr)
        {
            this->query = dtAllocNavMeshQuery();
            this->query->init(this->mesh, 2048);
        }
    }
    ~NavQuery()
    {
        dtFreeNavMeshQuery(this->query);
        dtFreeNavMesh(this->mesh);
    }
    bool IsValid() const
    {
        return this->query != nullptr;
    }
    /// nearest polygon within a small box around the point
    dtPolyRef Nearest(float x, float y, float z, float* nearest = nullptr) const
    {
        const float pos[3] = { x, y, z };
        const float extents[3] = { 1.0f, 2.0f, 1.0f };
        float point[3];
        dtPolyRef ref = 0;
        if (dtStatusFailed(this->query->findNearestPoly(pos, extents, &this->filter, &ref, point)))
        {
            return 0;
        }
        if (nearest != nullptr)
        {
            nearest[0] = point[0];
            nearest[1] = point[1];
            nearest[2] = point[2];
        }
        return ref;
    }
    /// height of the detail mesh under the point, or a huge negative number
    float Height(float x, float y, float z) const
    {
        dtPolyRef ref = this->Nearest(x, y, z);
        const float pos[3] = { x, y, z };
        float h = 0.0f;
        if (ref == 0 || dtStatusFailed(this->query->getPolyHeight(ref, pos, &h)))
        {
            return -1.0e6f;
        }
        return h;
    }
    /// returns true if a complete path between the points exists
    bool CompletePath(const float* start, const float* end) const
    {
        dtPolyRef startRef = this->Nearest(start[0], start[1], start[2]);
        dtPolyRef endRef = this->Nearest(end[0], end[1], end[2]);
        if (startRef == 0 || endRef == 0)
        {
            return false;
        }
        dtPolyRef path[256];
        int pathCount = 0;
        dtStatus status = this->query->findPath(startRef, endRef, start, end, &this->filter, path, &pathCount, 256);
        return dtStatusSucceed(status) && !dtStatusDetail(status, DT_PARTIAL_RESULT) && pathCount > 0 && path[pathCount - 1] == endRef;
    }
    /// flags of the polygon under the point
    int Flags(float x, float y, float z) const
    {
        dtPolyRef ref = this->Nearest(x, y, z);
        unsigned short flags = 0;
        if (ref == 0 || dtStatusFailed(this->mesh->getPolyFlags(ref, &flags)))
        {
            return -1;
        }
        return flags;
    }
    /// area of the polygon under the point
    int Area(float x, float y, float z) const
    {
        dtPolyRef ref = this->Nearest(x, y, z);
        unsigned char area = 0xFF;
        if (ref == 0 || dtStatusFailed(this->mesh->getPolyArea(ref, &area)))
        {
            return -1;
        }
        return area;
    }
};

//------------------------------------------------------------------------------
/**
    A height field with the sample function f(x, z)
*/
template <typename FUNC>
static HeightFieldSource
MakeHeightField(SizeT width, SizeT height, float sizeX, float sizeZ, float heightRange, FUNC sample)
{
    HeightFieldSource field;
    field.width = width;
    field.height = height;
    field.sizeX = sizeX;
    field.sizeZ = sizeZ;
    field.heightRange = heightRange;
    field.transform = Math::mat4::identity;
    field.samples.Resize(width * height);
    for (SizeT z = 0; z < height; z++)
    {
        for (SizeT x = 0; x < width; x++)
        {
            field.samples[z * width + x] = sample(x, z);
        }
    }
    return field;
}

//------------------------------------------------------------------------------
/**
*/
void
NavBuildTest::Run()
{
    // a flat 40 m square is split into 19.2 m tiles and can be crossed corner to corner
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(40.0f));
        BuildOutput output;
        BuildStats stats;
        VERIFY(BuildNavMesh(input, output, &stats));
        VERIFY(stats.tilesX == 3);
        VERIFY(stats.tilesZ == 3);
        VERIFY(stats.builtTiles == 9);
        VERIFY(stats.failedTiles == 0);
        VERIFY(output.tiles.Size() == 9);
        VERIFY(output.params.maxTiles >= 9);
        VERIFY(output.params.tileWidth == input.settings.tile_size * input.settings.cell_size);

        NavQuery nav(output);
        VERIFY(nav.IsValid());
        VERIFY(nav.Nearest(20.0f, 0.0f, 20.0f) != 0);
        const float start[3] = { 2.0f, 0.0f, 2.0f };
        const float end[3] = { 38.0f, 0.0f, 38.0f };
        VERIFY(nav.CompletePath(start, end));
        VERIFY(nav.Area(20.0f, 0.0f, 20.0f) == DefaultWalkableArea);

        // the same input gives the same bytes
        BuildOutput again;
        VERIFY(BuildNavMesh(input, again));
        VERIFY(again.tiles.Size() == output.tiles.Size());
        bool identical = again.tiles.Size() == output.tiles.Size();
        for (IndexT i = 0; identical && i < output.tiles.Size(); i++)
        {
            identical = again.tiles[i].x == output.tiles[i].x && again.tiles[i].y == output.tiles[i].y && again.tiles[i].data == output.tiles[i].data;
        }
        VERIFY(identical);
    }

    // a smaller tile size gives more tiles over the same surface
    {
        BuildInput input = MakeInput();
        input.settings.tile_size = 32;
        input.meshes.Append(MakeQuad(40.0f));
        BuildOutput output;
        BuildStats stats;
        VERIFY(BuildNavMesh(input, output, &stats));
        VERIFY(stats.tilesX == 5);
        VERIFY(stats.builtTiles == 25);
    }

    // the winding tells up from down. Mirroring reverses the winding of the placed triangles, the front of
    // the source stays the walkable side
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(10.0f, true));
        BuildOutput output;
        VERIFY(!BuildNavMesh(input, output));
        VERIFY(output.tiles.IsEmpty());

        input.meshes[0].transform = Math::scaling(-1.0f, 1.0f, 1.0f);
        VERIFY(!BuildNavMesh(input, output));

        input.meshes[0] = MakeQuad(10.0f);
        input.meshes[0].transform = Math::scaling(-1.0f, 1.0f, 1.0f);
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Nearest(-5.0f, 0.0f, 5.0f) != 0);
    }

    // sources are placed with their transform
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(10.0f));
        input.meshes[0].transform = Math::translation(100.0f, 5.0f, -50.0f);
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Nearest(5.0f, 0.0f, 5.0f) == 0);
        VERIFY(Math::abs(nav.Height(105.0f, 5.0f, -45.0f) - 5.0f) < 0.3f);
    }

    // explicit bounds limit the build to part of the sources
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(40.0f));
        input.useSourceBounds = false;
        input.bounds.pmin = Math::point(0.0f, -1.0f, 0.0f);
        input.bounds.pmax = Math::point(15.0f, 3.0f, 15.0f);
        BuildOutput output;
        BuildStats stats;
        VERIFY(BuildNavMesh(input, output, &stats));
        VERIFY(stats.tilesX == 1);
        VERIFY(stats.tilesZ == 1);
        NavQuery nav(output);
        VERIFY(nav.Nearest(7.0f, 0.0f, 7.0f) != 0);
        VERIFY(nav.Nearest(30.0f, 0.0f, 30.0f) == 0);
    }

    // a flat height field is walkable everywhere and spans (width - 1) * size / width, like the physics height field
    {
        BuildInput input = MakeInput();
        input.heightFields.Append(MakeHeightField(65, 65, 32.0f, 32.0f, 10.0f, [](SizeT, SizeT)
        {
            return (ushort)0;
        }));
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Nearest(16.0f, 0.0f, 16.0f) != 0);
        VERIFY(nav.Nearest(-5.0f, 0.0f, 16.0f) == 0);
        const float start[3] = { 1.0f, 0.0f, 1.0f };
        const float end[3] = { 30.0f, 0.0f, 30.0f };
        VERIFY(nav.CompletePath(start, end));
    }

    // the surface height follows the physics sampling: corner origin, size / width spacing and samples quantized to 15 bits
    {
        BuildInput input = MakeInput();
        input.heightFields.Append(MakeHeightField(65, 65, 32.0f, 32.0f, 10.0f, [](SizeT x, SizeT)
        {
            return (ushort)(x * 1000);
        }));
        input.heightFields[0].transform = Math::translation(10.0f, 2.0f, 0.0f);
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        // local x = 16 lies between samples 32 and 33
        const float scale = 10.0f / 32767.0f;
        const float expected = 2.0f + 0.5f * ((float)(32000 >> 1) + (float)(33000 >> 1)) * scale;
        VERIFY(Math::abs(nav.Height(26.0f, expected, 16.0f) - expected) < 0.3f);
    }

    // a rotated height field lies where its transform puts it
    {
        BuildInput input = MakeInput();
        input.heightFields.Append(MakeHeightField(33, 33, 16.0f, 16.0f, 10.0f, [](SizeT, SizeT)
        {
            return (ushort)0;
        }));
        // a quarter turn around y maps local +x to world -z
        input.heightFields[0].transform = Math::rotationy(Math::deg2rad(90.0f));
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        // the center of the field moves from (8, 8) to (8, -8) or (-8, 8), depending on the rotation direction
        Math::vec4 p = input.heightFields[0].transform * Math::vec4(8.0f, 0.0f, 8.0f, 1.0f);
        VERIFY(Math::abs(Math::abs(p.x) - 8.0f) < 0.01f && Math::abs(Math::abs(p.z) - 8.0f) < 0.01f && (p.x < 0.0f || p.z < 0.0f));
        VERIFY(nav.Nearest(p.x, 0.0f, p.z) != 0);
        VERIFY(nav.Nearest(8.0f, 0.0f, 8.0f) == 0);
    }

    // a 20 m cliff splits the surface, both sides are walkable but not connected
    {
        BuildInput input = MakeInput();
        input.heightFields.Append(MakeHeightField(65, 65, 32.0f, 32.0f, 20.0f, [](SizeT x, SizeT)
        {
            return (ushort)(x < 32 ? 0 : 65535);
        }));
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        const float low[3] = { 5.0f, 0.0f, 16.0f };
        const float high[3] = { 25.0f, 20.0f, 16.0f };
        VERIFY(nav.Nearest(low[0], low[1], low[2]) != 0);
        VERIFY(nav.Nearest(high[0], high[1], high[2]) != 0);
        VERIFY(!nav.CompletePath(low, high));
    }

    // area volumes mark the surface inside them, area 0 cuts it out
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(40.0f));
        AreaVolume road;
        road.points.Append(Math::vec3(15.0f, 0.0f, 15.0f));
        road.points.Append(Math::vec3(15.0f, 0.0f, 25.0f));
        road.points.Append(Math::vec3(25.0f, 0.0f, 25.0f));
        road.points.Append(Math::vec3(25.0f, 0.0f, 15.0f));
        road.minY = -1.0f;
        road.maxY = 1.0f;
        road.area = 5;
        input.areas.Append(road);

        AreaVolume hole = road;
        for (Math::vec3& p : hole.points)
        {
            p.x -= 12.0f;
        }
        hole.area = 0;
        input.areas.Append(hole);

        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Area(20.0f, 0.0f, 20.0f) == 5);
        VERIFY(nav.Area(35.0f, 0.0f, 35.0f) == DefaultWalkableArea);
        float nearest[3];
        dtPolyRef ref = nav.Nearest(8.0f, 0.0f, 20.0f, nearest);
        // nothing is left inside the hole, the nearest surface is at least half a meter away
        VERIFY(ref == 0 || Math::abs(nearest[0] - 8.0f) + Math::abs(nearest[2] - 20.0f) > 0.5f);

        input.areas[0].area = 64;
        VERIFY(!BuildNavMesh(input, output));
    }

    // the area flags decide the polygon flags, volumes are applied by priority, cylinders mark discs
    {
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(40.0f));
        input.areaFlags[5] = 1 << 1;
        input.areaFlags[6] = 1 << 0;

        auto square = [](float x0, float z0, float x1, float z1, unsigned char area, int priority)
        {
            AreaVolume volume;
            volume.points.Append(Math::vec3(x0, 0.0f, z0));
            volume.points.Append(Math::vec3(x0, 0.0f, z1));
            volume.points.Append(Math::vec3(x1, 0.0f, z1));
            volume.points.Append(Math::vec3(x1, 0.0f, z0));
            volume.minY = -1.0f;
            volume.maxY = 1.0f;
            volume.area = area;
            volume.priority = priority;
            return volume;
        };
        // the first one has the higher priority, it wins where they overlap although it comes first
        input.areas.Append(square(5.0f, 5.0f, 20.0f, 20.0f, 5, 1));
        input.areas.Append(square(10.0f, 10.0f, 30.0f, 30.0f, 6, 0));

        AreaVolume disc;
        disc.shape = AreaVolume::Cylinder;
        disc.center = Math::vec3(33.0f, 0.0f, 8.0f);
        disc.radius = 3.0f;
        disc.minY = -1.0f;
        disc.maxY = 1.0f;
        disc.area = 7;
        input.areas.Append(disc);

        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Area(15.0f, 0.0f, 15.0f) == 5);
        VERIFY(nav.Flags(15.0f, 0.0f, 15.0f) == (1 << 1));
        VERIFY(nav.Area(25.0f, 0.0f, 25.0f) == 6);
        VERIFY(nav.Flags(25.0f, 0.0f, 25.0f) == (1 << 0));
        VERIFY(nav.Area(33.0f, 0.0f, 8.0f) == 7);
        VERIFY(nav.Area(35.0f, 0.0f, 35.0f) == DefaultWalkableArea);
        VERIFY(nav.Flags(35.0f, 0.0f, 35.0f) == PolyFlag_Walk);

        // swapping the priorities lets the other one win the overlap
        input.areas[0].priority = 0;
        input.areas[1].priority = 1;
        VERIFY(BuildNavMesh(input, output));
        NavQuery swapped(output);
        VERIFY(swapped.Area(15.0f, 0.0f, 15.0f) == 6);
        VERIFY(swapped.Area(7.0f, 0.0f, 7.0f) == 5);
    }

    // a box turned around y is outlined by its four corners seen from above, duplicates fall away
    {
        Util::Array<Math::vec3> corners;
        const Math::mat4 m = Math::rotationy(Math::deg2rad(30.0f));
        for (int i = 0; i < 8; i++)
        {
            Math::vec4 p = m * Math::vec4((i & 1) ? 2.0f : -2.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : -1.0f, 1.0f);
            corners.Append(Math::vec3(p.x, 0.0f, p.z));
        }
        corners.Append(Math::vec3(0.0f, 0.0f, 0.0f));
        Util::Array<Math::vec3> hull = ConvexHullXZ(corners);
        VERIFY(hull.Size() == 4);
        // counter clockwise in x and z, every turn has the same sign
        bool convex = true;
        for (IndexT i = 0; i < hull.Size(); i++)
        {
            const Math::vec3& a = hull[i];
            const Math::vec3& b = hull[(i + 1) % hull.Size()];
            const Math::vec3& c = hull[(i + 2) % hull.Size()];
            convex &= ((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) > 0.0f;
        }
        VERIFY(convex);
    }

    // an area volume is its box, or the outline its points give, which may be concave
    {
        const Math::mat4 box = Math::trs(Math::vec3(10.0f, 2.0f, 10.0f), Math::quat(), Math::vec3(2.0f, 1.0f, 2.0f));
        Math::bbox local;
        local.pmin = Math::point(-1.0f, -1.0f, -1.0f);
        local.pmax = Math::point(1.0f, 1.0f, 1.0f);
        const Math::mat4 frame = Math::translation(10.0f, 2.0f, 10.0f);

        AreaPrism plain = MakeAreaPrism(box, local, frame, nullptr, 0);
        VERIFY(plain.outline.Size() == 4);
        VERIFY(plain.minY == 1.0f && plain.maxY == 3.0f);
        VERIFY(BoxOutlineXZ(box, local).Size() == 4);

        // a point inserts into the edge closest to it, between the two outline points of that edge
        Util::Array<Math::vec3> outline;
        outline.Append(Math::vec3(-2, 0, -2));
        outline.Append(Math::vec3(2, 0, -2));
        outline.Append(Math::vec3(2, 0, 2));
        outline.Append(Math::vec3(-2, 0, 2));
        const Math::vec3 east(6.0f, -1.0f, 0.5f);
        VERIFY(OutlineInsertIndex(outline.Begin(), outline.Size(), east) == 1);
        outline.Insert(2, east);
        VERIFY(OutlineInsertIndex(outline.Begin(), outline.Size(), Math::vec3(0.0f, 0.0f, -5.0f)) == 0);
        VERIFY(OutlineInsertIndex(outline.Begin(), outline.Size(), Math::vec3(-5.0f, 0.0f, 0.0f)) == 4);

        // the outline keeps its order, the height span covers the points give or take half the box height
        AreaPrism extended = MakeAreaPrism(box, local, frame, outline.Begin(), outline.Size());
        VERIFY(extended.outline.Size() == 5);
        VERIFY(extended.outline[2].x == 16.0f && extended.outline[2].z == 10.5f);
        VERIFY(extended.minY == 0.0f && extended.maxY == 3.0f);

        // edges which cross are found
        VERIFY(IsSimpleOutlineXZ(outline.Begin(), outline.Size()));
        Util::Array<Math::vec3> bowtie;
        bowtie.Append(Math::vec3(0, 0, 0));
        bowtie.Append(Math::vec3(4, 0, 4));
        bowtie.Append(Math::vec3(4, 0, 0));
        bowtie.Append(Math::vec3(0, 0, 4));
        VERIFY(!IsSimpleOutlineXZ(bowtie.Begin(), bowtie.Size()));

        // a concave outline, an L, marks both arms and leaves the notch alone
        BuildInput input = MakeInput();
        input.meshes.Append(MakeQuad(40.0f));
        AreaVolume volume;
        volume.points.Append(Math::vec3(5, 0, 5));
        volume.points.Append(Math::vec3(35, 0, 5));
        volume.points.Append(Math::vec3(35, 0, 15));
        volume.points.Append(Math::vec3(15, 0, 15));
        volume.points.Append(Math::vec3(15, 0, 35));
        volume.points.Append(Math::vec3(5, 0, 35));
        volume.minY = -1.0f;
        volume.maxY = 1.0f;
        volume.area = 9;
        input.areas.Append(volume);
        BuildOutput output;
        VERIFY(BuildNavMesh(input, output));
        NavQuery nav(output);
        VERIFY(nav.Area(30.0f, 0.0f, 10.0f) == 9);
        VERIFY(nav.Area(10.0f, 0.0f, 30.0f) == 9);
        VERIFY(nav.Area(28.0f, 0.0f, 28.0f) == DefaultWalkableArea);

        // a ray from above hits the surface where the navmesh is, and misses beside it
        float t = 0.0f;
        VERIFY(RaycastNavMesh(*nav.mesh, Math::point(18.0f, 10.0f, 10.0f), Math::vector(0.0f, -1.0f, 0.0f), t));
        VERIFY(Math::abs(t - 10.0f) < 0.3f);
        VERIFY(RaycastNavMesh(*nav.mesh, Math::point(0.0f, 30.0f, -20.0f), Math::vector(1.0f, -1.0f, 1.0f), t));
        VERIFY(!RaycastNavMesh(*nav.mesh, Math::point(60.0f, 10.0f, 60.0f), Math::vector(0.0f, -1.0f, 0.0f), t));
    }

    // invalid input fails without tiles
    {
        BuildInput input = MakeInput();
        BuildOutput output;
        VERIFY(!BuildNavMesh(input, output));

        MeshSource broken = MakeQuad(10.0f);
        broken.indices[0] = 100;
        input.meshes.Append(broken);
        VERIFY(!BuildNavMesh(input, output));
        VERIFY(output.tiles.IsEmpty());

        // more tiles than 32 bit poly refs can address
        input = MakeInput();
        input.meshes.Append(MakeQuad(1000.0f));
        input.settings.cell_size = 0.01f;
        BuildStats stats;
        VERIFY(!BuildNavMesh(input, output, &stats));
        VERIFY(output.tiles.IsEmpty());
    }
}

} // namespace Test
