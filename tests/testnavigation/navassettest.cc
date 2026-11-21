//------------------------------------------------------------------------------
//  @file navassettest.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testbase/testcase.h"
#include "navassettest.h"

#include "navbuild/navmeshasset.h"
#include "navbuild/geometrysources.h"
#include "flat/navigation/navareas.h"
#include "math/transform.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "physics/geometry.h"
#include "io/ioserver.h"
#include "io/path.h"
#include "io/assignregistry.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

using namespace Navigation;
using namespace Navigation::Build;

namespace Test
{
__ImplementClass(Test::NavAssetTest, 'NVAT', Test::TestCase);

/// edge length of the quad the test loader returns, changing it changes the geometry of the source
static float quadSize = 20.0f;

//------------------------------------------------------------------------------
/**
    Geometry loader which knows test:quad:msh, a flat quad, and test:flat:tex, a flat height field
*/
static void
SetupTestLoader()
{
    Physics::GeometryLoader loader;
    loader.LoadMesh = [](Util::StringAtom resource, IndexT meshIndex, IndexT primGroup, Physics::MeshGeometryInput& out) -> bool
    {
        if (resource != Util::StringAtom("test:quad:msh") || meshIndex != 0 || primGroup != 0)
        {
            return false;
        }
        const float s = quadSize;
        const float verts[] = { 0, 0, 0,   0, 0, s,   s, 0, 0,   s, 0, s };
        out.positions.AppendArray(verts, 12);
        out.vertexCount = 4;
        const uint32_t indices[] = { 0, 1, 2,   2, 1, 3 };
        out.indexType = Physics::IndexType::Index32;
        out.indices.AppendArray((const byte*)indices, sizeof(indices));
        out.indexCount = 6;
        return true;
    };
    loader.LoadHeightField = [](Util::StringAtom resource, Physics::HeightFieldInput& out) -> bool
    {
        if (resource != Util::StringAtom("test:flat:tex"))
        {
            return false;
        }
        out.width = 33;
        out.height = 33;
        out.samples.Resize(33 * 33);
        out.samples.Fill(0, 33 * 33, 0);
        return true;
    };
    Physics::SetGeometryLoader(loader);
}

//------------------------------------------------------------------------------
/**
*/
static NavMeshT
MakeDescription()
{
    NavMeshT description;
    description.name = "test";
    description.agent_kind = std::make_unique<AgentT>();
    description.agent_kind->name = "human";
    description.agent_kind->agent_height = 2.0f;
    description.agent_kind->agent_radius = 0.5f;
    description.agent_kind->agent_max_climb = 0.5f;
    description.navmesh_settings = std::make_unique<NavMeshSettingsT>();

    auto mesh = std::make_unique<GeometryEntryT>();
    MeshSourceT meshSource;
    meshSource.resource = "test:quad:msh";
    mesh->geometry.Set(meshSource);
    mesh->transform = Math::mat4::identity;
    description.sources.push_back(std::move(mesh));

    // a height field next to the quad
    auto field = std::make_unique<GeometryEntryT>();
    HeightFieldSourceT fieldSource;
    fieldSource.resource = "test:flat:tex";
    fieldSource.height_range = 10.0f;
    fieldSource.size_x = 16.0f;
    fieldSource.size_z = 16.0f;
    field->geometry.Set(fieldSource);
    field->transform = Math::translation(40.0f, 0.0f, 0.0f);
    description.sources.push_back(std::move(field));
    return description;
}

//------------------------------------------------------------------------------
/**
*/
static bool
WriteFile(const IO::URI& file, const void* data, SizeT size)
{
    Ptr<IO::Stream> stream = IO::CreateStream(file);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        return false;
    }
    stream->Write(data, size);
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static bool
WriteDescription(const IO::URI& file, const NavMeshT& description)
{
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavMesh>(description);
    return WriteFile(file, data.GetPtr(), data.Size());
}

//------------------------------------------------------------------------------
/**
    Loads a baked navmesh file the way the runtime loader does
*/
static dtNavMesh*
LoadBaked(const IO::URI& file, int& tileCount, Util::String& agentName)
{
    Ptr<IO::Stream> stream = IO::CreateStream(file);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return nullptr;
    }
    dtNavMesh* navMesh = nullptr;
    const void* data = stream->MemoryMap();
    if (IsBakedNavMesh(data, stream->GetSize()))
    {
        const BakedNavMesh* baked = GetBakedNavMesh(data);
        tileCount = baked->tiles()->size();
        agentName = baked->agent_kind() != nullptr && baked->agent_kind()->name() != nullptr ? baked->agent_kind()->name()->c_str() : "";
        navMesh = CreateDetourNavMesh(baked);
    }
    stream->MemoryUnmap();
    stream->Close();
    return navMesh;
}

//------------------------------------------------------------------------------
/**
*/
void
NavAssetTest::Run()
{
    // a test app has no project assigns, exports go to the temp folder
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("nav", "temp:navassettest"));

    // nav URNs resolve to .nanav work files and .navmesh exports
    RegisterPathMappings();
    {
        IO::Path path = IO::Path::Parse("levels:arena:nav");
        // URIs resolve their assign when they are built
        VERIFY(path.ExportURI().LocalPath() == IO::URI("nav:levels/arena.navmesh").LocalPath());
        VERIFY(path.GetWorkFile() == "arena.nanav");
    }

    // a collider is placed like physics composes the actor pose with the shape offset
    {
        const Math::vec3 position(3.0f, -2.0f, 7.0f);
        const Math::quat orientation = Math::rotationquataxis(Math::normalize(Math::vec3(0.3f, 1.0f, -0.2f)), 0.7f);
        const Math::vec3 localPosition(1.0f, 0.5f, -2.0f);
        const Math::quat localOrientation = Math::rotationquataxis(Math::normalize(Math::vec3(-0.5f, 0.2f, 1.0f)), -1.1f);
        const Math::mat4 m = ColliderTransform(position, orientation, localPosition, localOrientation);
        const Math::transform physicsPose = Math::transform(position, orientation) * Math::transform(localPosition, localOrientation);
        bool same = true;
        const Math::point points[] = { Math::point(0, 0, 0), Math::point(1, 2, 3), Math::point(-4, 0.5f, 2) };
        for (const Math::point& p : points)
        {
            const Math::vec4 a = m * Math::vec4(p.x, p.y, p.z, 1.0f);
            const Math::point b = physicsPose * p;
            same &= Math::abs(a.x - b.x) < 1e-4f && Math::abs(a.y - b.y) < 1e-4f && Math::abs(a.z - b.z) < 1e-4f;
        }
        VERIFY(same);
    }

    SetupTestLoader();
    quadSize = 20.0f;

    const IO::URI folder("temp:navassettest");
    IO::CreateDirectory(folder);
    const IO::URI workFile("temp:navassettest/test.nanav");
    const IO::URI exportFile("temp:navassettest/test.navmesh");
    IO::DeleteFile(exportFile);

    VERIFY(WriteDescription(workFile, MakeDescription()));

    // baking writes the export, baking again with the same inputs is skipped
    VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);
    uint64_t firstHash = 0;
    VERIFY(ReadInputHash(exportFile, firstHash));
    VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::UpToDate);
    VERIFY(BakeNavMesh(workFile, exportFile, true) == BakeResult::Baked);

    // the baked tiles load into a Detour navmesh which covers both sources
    {
        int tileCount = 0;
        Util::String agentName;
        dtNavMesh* navMesh = LoadBaked(exportFile, tileCount, agentName);
        VERIFY(navMesh != nullptr);
        VERIFY(tileCount > 1);
        VERIFY(agentName == "human");
        if (navMesh != nullptr)
        {
            dtNavMeshQuery* query = dtAllocNavMeshQuery();
            query->init(navMesh, 512);
            dtQueryFilter filter;
            const float extents[3] = { 1.0f, 2.0f, 1.0f };
            const float onQuad[3] = { 10.0f, 0.0f, 10.0f };
            const float onField[3] = { 48.0f, 0.0f, 8.0f };
            const float between[3] = { 30.0f, 0.0f, 10.0f };
            dtPolyRef ref = 0;
            float nearest[3];
            VERIFY(dtStatusSucceed(query->findNearestPoly(onQuad, extents, &filter, &ref, nearest)) && ref != 0);
            VERIFY(dtStatusSucceed(query->findNearestPoly(onField, extents, &filter, &ref, nearest)) && ref != 0);
            VERIFY(dtStatusSucceed(query->findNearestPoly(between, extents, &filter, &ref, nearest)) && ref == 0);
            dtFreeNavMeshQuery(query);
            dtFreeNavMesh(navMesh);
        }
    }

    // changed geometry behind an unchanged work file bakes again
    quadSize = 24.0f;
    VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);
    uint64_t secondHash = 0;
    VERIFY(ReadInputHash(exportFile, secondHash));
    VERIFY(secondHash != firstHash);
    VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::UpToDate);

    // a changed work file bakes again
    {
        NavMeshT description = MakeDescription();
        description.navmesh_settings->cell_size = 0.25f;
        VERIFY(WriteDescription(workFile, description));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);
    }

    // a missing source fails and leaves the export as it was
    {
        uint64_t before = 0;
        VERIFY(ReadInputHash(exportFile, before));
        NavMeshT description = MakeDescription();
        description.sources[0]->geometry.AsMeshSource()->resource = "test:missing:msh";
        VERIFY(WriteDescription(workFile, description));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Failed);
        uint64_t after = 0;
        VERIFY(ReadInputHash(exportFile, after) && after == before);
    }

    // a description without an agent can't be built
    {
        NavMeshT description = MakeDescription();
        description.agent_kind = nullptr;
        BuildInput input;
        VERIFY(!SetupBuildInput(description, AreaTable(), input));
    }

    // files which aren't navmeshes are rejected
    {
        const char garbage[] = "this is not a flatbuffer, but it is long enough to look like one";
        VERIFY(!IsBakedNavMesh(garbage, sizeof(garbage)));
        VERIFY(WriteFile(workFile, garbage, sizeof(garbage)));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Failed);
        uint64_t hash = 0;
        VERIFY(!ReadInputHash("temp:navassettest/missing.navmesh", hash));
    }

    // the area table defines project areas 1 to 62, the built in ones always exist
    const IO::URI tableFile(AreaTableFile);
    {
        NavAreasT table;
        auto add = [&table](int id, const char* name, float cost, unsigned short flags)
        {
            auto area = std::make_unique<NavAreaDefinitionT>();
            area->id = (uint8_t)id;
            area->name = name;
            area->cost = cost;
            area->flags = (NavAreaFlags)flags;
            table.areas.push_back(std::move(area));
        };
        add(4, "water", 10.0f, NavAreaFlags_Swim);
        add(1, "road", 0.5f, NavAreaFlags_Walk);
        add(63, "invalid", 1.0f, NavAreaFlags_Walk);
        add(2, "road", 1.0f, NavAreaFlags_Walk);
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavAreas>(table);

        AreaTable areas;
        VERIFY(!areas.Load(data.GetPtr(), data.Size()));
        VERIFY(areas.Areas().Size() == 4);
        VERIFY(areas.Find("none") != nullptr && areas.Find("none")->id == NoArea);
        VERIFY(areas.Find("default") != nullptr && areas.Find("default")->id == DefaultWalkableArea);
        VERIFY(areas.Find("road") != nullptr && areas.Find("road")->id == 1 && areas.Find("road")->cost == 0.5f);
        VERIFY(areas.Find("invalid") == nullptr);
        VERIFY(areas.Get(2) == nullptr);
        VERIFY(areas.Areas()[0].id == 0 && areas.Areas()[1].id == 1);
        unsigned short flags[MaxAreas];
        areas.GetFlags(flags);
        VERIFY(flags[0] == 0 && flags[1] == PolyFlag_Walk && flags[4] == NavAreaFlags_Swim && flags[DefaultWalkableArea] == PolyFlag_Walk && flags[9] == 0);
        VERIFY(areas.GetColor(9).w > 0.0f);

        // the valid entries only
        table.areas.pop_back();
        table.areas.pop_back();
        data = Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavAreas>(table);
        VERIFY(areas.Load(data.GetPtr(), data.Size()));
        VERIFY(WriteFile(tableFile, data.GetPtr(), data.Size()));
    }

    // area modifiers name their area, the bake resolves it with the table and rebakes when the table changes
    {
        NavMeshT description = MakeDescription();
        auto road = std::make_unique<NavAreaModifierT>();
        road->area = "road";
        road->convex_area_points = { Math::vec3(2, 0, 2), Math::vec3(2, 0, 18), Math::vec3(18, 0, 18), Math::vec3(18, 0, 2) };
        road->min_y = -1.0f;
        road->max_y = 1.0f;
        description.area_modifiers.push_back(std::move(road));
        VERIFY(WriteDescription(workFile, description));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);

        int tileCount = 0;
        Util::String agentName;
        dtNavMesh* navMesh = LoadBaked(exportFile, tileCount, agentName);
        VERIFY(navMesh != nullptr);
        if (navMesh != nullptr)
        {
            dtNavMeshQuery* query = dtAllocNavMeshQuery();
            query->init(navMesh, 512);
            dtQueryFilter filter;
            const float extents[3] = { 1.0f, 2.0f, 1.0f };
            const float onRoad[3] = { 10.0f, 0.0f, 10.0f };
            dtPolyRef ref = 0;
            float nearest[3];
            unsigned char area = 0;
            VERIFY(dtStatusSucceed(query->findNearestPoly(onRoad, extents, &filter, &ref, nearest)) && ref != 0);
            VERIFY(dtStatusSucceed(navMesh->getPolyArea(ref, &area)) && area == 1);
            dtFreeNavMeshQuery(query);
            dtFreeNavMesh(navMesh);
        }
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::UpToDate);

        // a changed table bakes again, the flags of an area are part of the result
        NavAreasT table;
        auto roadArea = std::make_unique<NavAreaDefinitionT>();
        roadArea->id = 1;
        roadArea->name = "road";
        roadArea->flags = (NavAreaFlags)(NavAreaFlags_Walk | NavAreaFlags_Door);
        table.areas.push_back(std::move(roadArea));
        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<Navigation::NavAreas>(table);
        VERIFY(WriteFile(tableFile, data.GetPtr(), data.Size()));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);

        // an area the table doesn't have, a removed or renamed one, is baked as the default area
        description.area_modifiers[0]->area = "lava";
        AreaTable lavaless;
        VERIFY(lavaless.Load(tableFile));
        BuildInput lavaInput;
        VERIFY(SetupBuildInput(description, lavaless, lavaInput));
        VERIFY(lavaInput.areas.Size() == 1 && lavaInput.areas[0].area == DefaultWalkableArea);
        VERIFY(WriteDescription(workFile, description));
        VERIFY(BakeNavMesh(workFile, exportFile, false) == BakeResult::Baked);
    }

    // areas written as a table source, the way the editor saves them, compile and load back as they were
    {
        Util::Array<AreaDefinition> written;
        AreaDefinition water;
        water.id = 3;
        water.name = "water";
        water.cost = 7.5f;
        water.flags = PolyFlag_Walk | (1 << 1);
        water.color = Math::vec4(0.1f, 0.2f, 0.9f, 1.0f);
        written.Append(water);
        AreaDefinition blocked;
        blocked.id = 12;
        blocked.name = "blocked";
        blocked.cost = 1.0f;
        blocked.flags = 0;
        blocked.color = Math::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        written.Append(blocked);
        // the built in ones are left out
        AreaDefinition builtIn;
        builtIn.id = DefaultWalkableArea;
        builtIn.name = "default";
        written.Append(builtIn);

        const IO::URI source("temp:navassettest/navareas.json");
        VERIFY(WriteAreaTableSource(written, source));
        VERIFY(CompileFlatbuffer(Navigation::NavAreas, source, "temp:navassettest/"));
        AreaTable loaded;
        VERIFY(loaded.Load(IO::URI("temp:navassettest/navareas.navareas")));
        VERIFY(loaded.Areas().Size() == 4);
        const AreaDefinition* readWater = loaded.Find("water");
        VERIFY(readWater != nullptr && readWater->id == 3 && readWater->cost == 7.5f && readWater->flags == (PolyFlag_Walk | (1 << 1)));
        VERIFY(readWater != nullptr && Math::abs(readWater->color.z - 0.9f) < 0.001f);
        const AreaDefinition* readBlocked = loaded.Find("blocked");
        VERIFY(readBlocked != nullptr && readBlocked->id == 12 && readBlocked->flags == 0);
        IO::DeleteFile(source);
        IO::DeleteFile("temp:navassettest/navareas.navareas");
    }

    IO::DeleteFile(tableFile);
    IO::DeleteFile(workFile);
    IO::DeleteFile(exportFile);
    IO::DeleteDirectory(folder);
    Physics::SetGeometryLoader(Physics::GeometryLoader());
}

} // namespace Test
