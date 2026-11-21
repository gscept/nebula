#pragma once
//------------------------------------------------------------------------------
/**
    Navigation::Build navmesh assets

    Ties the navmesh builder to the asset formats.

    Navmeshes are addressed with URNs of the nav type, folder:file:nav. The work file,
    work:folder/file.nanav, holds a Navigation::NavMesh description. Baking it writes
    nav:folder/file.navmesh, a Navigation::BakedNavMesh with the Detour tiles.

    A bake is skipped when the export holds the hash of the same inputs: the work file,
    the area table and the geometry of every source as the geometry loader reads it, so
    editing a mesh, heightmap or area a navmesh uses bakes it again.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "navbuild/navmeshbuilder.h"
#include "navbuild/navareas.h"
#include "io/uri.h"
// declares the packing of the math types, which the generated headers use
#include "nflatbuffer/nebula_flat.h"
#include "flat/navigation/navmesh.h"
#include "flat/navigation/bakednavmesh.h"

namespace Navigation
{
namespace Build
{

/// part of every input hash, bump it when the builder output changes for the same input
static constexpr uint32_t BuilderVersion = 1;

enum class BakeResult
{
    Baked,
    UpToDate,
    Failed
};

/// register the work and export extensions of the nav URN type
void RegisterPathMappings();

/// fill a build input from a description, loading every source through the geometry loader and
/// resolving area names with the area table
bool SetupBuildInput(const NavMeshT& description, const AreaTable& areas, BuildInput& out);
/// hash of the work file data, the area table and the loaded source geometry
uint64_t InputHash(const void* workData, SizeT workSize, const AreaTable& areas, const BuildInput& input);
/// read the input hash stored in a baked navmesh, returns false if the file is missing or not a baked navmesh
bool ReadInputHash(const IO::URI& exportFile, uint64_t& hash);
/// bake a work file to an export file, registers the default geometry loader if none is registered.
/// Areas are resolved with the compiled area table, AreaTableFile, or the built in areas if there is none
BakeResult BakeNavMesh(const IO::URI& workFile, const IO::URI& exportFile, bool force);

/// check that a buffer holds a baked navmesh
bool IsBakedNavMesh(const void* data, SizeT size);
/// create a Detour navmesh from baked data, the tile data is copied. Free with dtFreeNavMesh
dtNavMesh* CreateDetourNavMesh(const BakedNavMesh* baked);

} // namespace Build
} // namespace Navigation
