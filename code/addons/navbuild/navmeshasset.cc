//------------------------------------------------------------------------------
//  @file navmeshasset.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navbuild/navmeshasset.h"
#include "navbuild/geometrysources.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "io/ioserver.h"
#include "io/path.h"
#include "physics/geometry.h"
#include "physics/formats/formats.h"
#include "DetourAlloc.h"

namespace Navigation
{
namespace Build
{

//------------------------------------------------------------------------------
/**
    FNV-1a, 64 bit
*/
class InputHasher
{
public:
    void Add(const void* data, size_t size)
    {
        const uint8_t* bytes = (const uint8_t*)data;
        for (size_t i = 0; i < size; i++)
        {
            this->hash = (this->hash ^ bytes[i]) * 0x100000001b3ull;
        }
    }
    template <typename T>
    void Add(const T& value)
    {
        this->Add(&value, sizeof(T));
    }
    template <typename T>
    void AddArray(const Util::Array<T>& array)
    {
        this->Add(array.Size());
        if (!array.IsEmpty())
        {
            this->Add(array.Begin(), array.Size() * sizeof(T));
        }
    }
    void AddMatrix(const Math::mat4& m)
    {
        float values[16];
        m.store(values);
        this->Add(values, sizeof(values));
    }
    uint64_t Get() const
    {
        return this->hash;
    }

private:
    uint64_t hash = 0xcbf29ce484222325ull;
};

//------------------------------------------------------------------------------
/**
*/
void
RegisterPathMappings()
{
    IO::Path::AddWorkMapping("nav", "nanav");
    IO::Path::AddExportMapping("nav", "navmesh");
}

//------------------------------------------------------------------------------
/**
*/
bool
SetupBuildInput(const NavMeshT& description, const AreaTable& areas, BuildInput& out)
{
    out = BuildInput();
    if (description.agent_kind == nullptr)
    {
        n_warning("SetupBuildInput: '%s' has no agent kind\n", description.name.c_str());
        return false;
    }
    out.agent = *description.agent_kind;
    if (description.navmesh_settings != nullptr)
    {
        out.settings = *description.navmesh_settings;
    }
    out.useSourceBounds = description.use_source_bounds;
    out.bounds = Math::bbox(Math::point(description.bounds_center), Math::vector(description.bounds_extents));

    for (IndexT i = 0; i < (IndexT)description.sources.size(); i++)
    {
        const GeometryEntryT& entry = *description.sources[i];
        if (const MeshSourceT* mesh = entry.geometry.AsMeshSource())
        {
            MeshSource source;
            if (!LoadMeshSource(mesh->resource.c_str(), mesh->mesh_index, mesh->prim_group, entry.transform, source))
            {
                n_warning("SetupBuildInput: '%s' could not read mesh '%s'\n", description.name.c_str(), mesh->resource.c_str());
                return false;
            }
            out.meshes.Append(std::move(source));
        }
        else if (const HeightFieldSourceT* field = entry.geometry.AsHeightFieldSource())
        {
            HeightFieldSource source;
            if (!LoadHeightFieldSource(field->resource.c_str(), field->height_range, field->size_x, field->size_z, entry.transform, source))
            {
                n_warning("SetupBuildInput: '%s' could not read height field '%s'\n", description.name.c_str(), field->resource.c_str());
                return false;
            }
            out.heightFields.Append(std::move(source));
        }
        else
        {
            n_warning("SetupBuildInput: source %d of '%s' has no geometry\n", i, description.name.c_str());
            return false;
        }
    }

    areas.GetFlags(out.areaFlags);
    for (const auto& modifier : description.area_modifiers)
    {
        AreaVolume volume;
        for (const Math::vec3& p : modifier->convex_area_points)
        {
            volume.points.Append(p);
        }
        volume.minY = modifier->min_y;
        volume.maxY = modifier->max_y;
        volume.priority = modifier->priority;
        if (!modifier->area.empty())
        {
            // an area which was removed or renamed in the table falls back to the default one
            const AreaDefinition* area = areas.Find(modifier->area.c_str());
            if (area == nullptr)
            {
                n_warning("SetupBuildInput: '%s' uses area '%s', which isn't in the area table, it is baked as default\n", description.name.c_str(), modifier->area.c_str());
                volume.area = DefaultWalkableArea;
            }
            else
            {
                volume.area = area->id;
            }
        }
        else
        {
            volume.area = modifier->area_id;
        }
        out.areas.Append(std::move(volume));
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    The description is part of the work data, only the geometry it points at has to be added
*/
uint64_t
InputHash(const void* workData, SizeT workSize, const AreaTable& areas, const BuildInput& input)
{
    InputHasher hasher;
    hasher.Add(BuilderVersion);
    hasher.Add(workSize);
    hasher.Add(workData, workSize);
    hasher.Add(areas.GetData().Size());
    if (areas.GetData().IsValid())
    {
        hasher.Add(areas.GetData().GetPtr(), areas.GetData().Size());
    }
    hasher.Add(input.meshes.Size());
    for (const MeshSource& mesh : input.meshes)
    {
        hasher.AddArray(mesh.positions);
        hasher.AddArray(mesh.indices);
        hasher.AddMatrix(mesh.transform);
    }
    hasher.Add(input.heightFields.Size());
    for (const HeightFieldSource& field : input.heightFields)
    {
        hasher.Add(field.width);
        hasher.Add(field.height);
        hasher.AddArray(field.samples);
        hasher.Add(field.heightRange);
        hasher.Add(field.sizeX);
        hasher.Add(field.sizeZ);
        hasher.AddMatrix(field.transform);
    }
    return hasher.Get();
}

//------------------------------------------------------------------------------
/**
*/
bool
IsBakedNavMesh(const void* data, SizeT size)
{
    if (data == nullptr || size < (SizeT)(sizeof(flatbuffers::uoffset_t) + flatbuffers::kFileIdentifierLength))
    {
        return false;
    }
    flatbuffers::Verifier verifier((const uint8_t*)data, (size_t)size);
    return VerifyBakedNavMeshBuffer(verifier);
}

//------------------------------------------------------------------------------
/**
*/
bool
ReadInputHash(const IO::URI& exportFile, uint64_t& hash)
{
    if (!IO::FileExists(exportFile))
    {
        return false;
    }
    Ptr<IO::Stream> stream = IO::CreateStream(exportFile);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        return false;
    }
    bool found = false;
    const void* data = stream->MemoryMap();
    if (IsBakedNavMesh(data, stream->GetSize()))
    {
        hash = GetBakedNavMesh(data)->input_hash();
        found = true;
    }
    stream->MemoryUnmap();
    stream->Close();
    return found;
}

//------------------------------------------------------------------------------
/**
*/
static bool
ReadWorkFile(const IO::URI& workFile, Util::Blob& data, NavMeshT& description)
{
    Ptr<IO::Stream> stream = IO::CreateStream(workFile);
    stream->SetAccessMode(IO::Stream::ReadAccess);
    if (!stream->Open())
    {
        n_warning("BakeNavMesh: could not open '%s'\n", workFile.LocalPath().AsCharPtr());
        return false;
    }
    const SizeT size = stream->GetSize();
    const void* mapped = stream->MemoryMap();
    if (mapped != nullptr && size > 0)
    {
        data.Set(mapped, size);
    }
    stream->MemoryUnmap();
    stream->Close();

    flatbuffers::Verifier verifier((const uint8_t*)data.GetPtr(), data.Size());
    if (!data.IsValid() || !verifier.VerifyBuffer<Navigation::NavMesh>(nullptr))
    {
        n_warning("BakeNavMesh: '%s' is not a navmesh work file\n", workFile.LocalPath().AsCharPtr());
        return false;
    }
    Flat::FlatbufferInterface::DeserializeFlatbuffer<Navigation::NavMesh>(description, (const uint8_t*)data.GetPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
static bool
WriteBakedNavMesh(const IO::URI& exportFile, const NavMeshT& description, const BuildOutput& output, uint64_t hash)
{
    BakedNavMeshT baked;
    baked.name = description.name;
    baked.agent_kind = std::make_unique<AgentT>(*description.agent_kind);
    baked.origin = Math::vec3(output.params.orig[0], output.params.orig[1], output.params.orig[2]);
    baked.tile_width = output.params.tileWidth;
    baked.tile_height = output.params.tileHeight;
    baked.max_tiles = output.params.maxTiles;
    baked.max_polys = output.params.maxPolys;
    baked.input_hash = hash;
    for (const Tile& tile : output.tiles)
    {
        auto bakedTile = std::make_unique<BakedNavTileT>();
        bakedTile->x = tile.x;
        bakedTile->y = tile.y;
        bakedTile->layer = tile.layer;
        const uint8_t* bytes = (const uint8_t*)tile.data.GetPtr();
        bakedTile->data.assign(bytes, bytes + tile.data.Size());
        baked.tiles.push_back(std::move(bakedTile));
    }

    flatbuffers::FlatBufferBuilder builder(65536);
    // the identifier lets the loader verify the buffer
    builder.Finish(BakedNavMesh::Pack(builder, &baked), BakedNavMeshIdentifier());

    // the local path always has a folder part, a URN in the root of the project has none
    IO::CreateDirectory(IO::URI(exportFile.LocalPath().ExtractDirName()));
    Ptr<IO::Stream> stream = IO::CreateStream(exportFile);
    stream->SetAccessMode(IO::Stream::WriteAccess);
    if (!stream->Open())
    {
        n_warning("BakeNavMesh: could not write '%s'\n", exportFile.LocalPath().AsCharPtr());
        return false;
    }
    stream->Write(builder.GetBufferPointer(), builder.GetSize());
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
BakeResult
BakeNavMesh(const IO::URI& workFile, const IO::URI& exportFile, bool force)
{
    const Physics::GeometryLoader& loader = Physics::GetGeometryLoader();
    if (loader.LoadMesh == nullptr && loader.LoadHeightField == nullptr)
    {
        Physics::RegisterDefaultGeometryLoader();
    }

    Util::Blob workData;
    NavMeshT description;
    if (!ReadWorkFile(workFile, workData, description))
    {
        return BakeResult::Failed;
    }

    AreaTable areas;
    if (!areas.Load(IO::URI(AreaTableFile)))
    {
        n_warning("BakeNavMesh: the area table '%s' has errors, see above\n", AreaTableFile);
    }

    BuildInput input;
    if (!SetupBuildInput(description, areas, input))
    {
        return BakeResult::Failed;
    }

    const uint64_t hash = InputHash(workData.GetPtr(), workData.Size(), areas, input);
    uint64_t exportedHash = 0;
    if (!force && ReadInputHash(exportFile, exportedHash) && exportedHash == hash)
    {
        return BakeResult::UpToDate;
    }

    BuildOutput output;
    BuildStats stats;
    if (!BuildNavMesh(input, output, &stats))
    {
        n_warning("BakeNavMesh: '%s' failed, %d of %d x %d tiles built, %d failed\n",
            workFile.LocalPath().AsCharPtr(), stats.builtTiles, stats.tilesX, stats.tilesZ, stats.failedTiles);
        return BakeResult::Failed;
    }
    if (!WriteBakedNavMesh(exportFile, description, output, hash))
    {
        return BakeResult::Failed;
    }
    n_printf("Baked navmesh '%s': %d of %d x %d tiles, %d polygons\n",
        exportFile.LocalPath().AsCharPtr(), stats.builtTiles, stats.tilesX, stats.tilesZ, stats.polygons);
    return BakeResult::Baked;
}

//------------------------------------------------------------------------------
/**
*/
dtNavMesh*
CreateDetourNavMesh(const BakedNavMesh* baked)
{
    if (baked == nullptr || baked->origin() == nullptr || baked->tiles() == nullptr)
    {
        return nullptr;
    }
    BuildOutput output;
    memset(&output.params, 0, sizeof(output.params));
    output.params.orig[0] = baked->origin()->x();
    output.params.orig[1] = baked->origin()->y();
    output.params.orig[2] = baked->origin()->z();
    output.params.tileWidth = baked->tile_width();
    output.params.tileHeight = baked->tile_height();
    output.params.maxTiles = baked->max_tiles();
    output.params.maxPolys = baked->max_polys();

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
    for (const BakedNavTile* tile : *baked->tiles())
    {
        if (tile->data() == nullptr || tile->data()->size() == 0)
        {
            continue;
        }
        // Detour writes links into the tile data and frees it with the tile, the mapped file can't be used
        const int size = (int)tile->data()->size();
        unsigned char* data = (unsigned char*)dtAlloc(size, DT_ALLOC_PERM);
        Memory::Copy(tile->data()->data(), data, size);
        if (dtStatusFailed(navMesh->addTile(data, size, DT_TILE_FREE_DATA, 0, nullptr)))
        {
            n_warning("CreateDetourNavMesh: tile (%d, %d) of '%s' could not be added\n", tile->x(), tile->y(), baked->name() ? baked->name()->c_str() : "");
            dtFree(data);
            dtFreeNavMesh(navMesh);
            return nullptr;
        }
    }
    return navMesh;
}

} // namespace Build
} // namespace Navigation
