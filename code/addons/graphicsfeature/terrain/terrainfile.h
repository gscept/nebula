#pragma once
//------------------------------------------------------------------------------
/**
    @file terrainfile.h
    The terrain file a Terrain component names, folder:file:ter. The work file is
    work:assets/folder/file.nater and the batcher copies it to ter:folder/file.nater, which is what
    the game reads. It is a Render::TerrainResource flatbuffer.

    The textures it names are URNs, folder:file:tex. Files written before URNs were used name a
    texture as folder/file, which is still read.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/path.h"
#include "io/uri.h"
#include "resources/resourceid.h"
#include "util/string.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/render/terrainresource.h"

namespace GraphicsFeature
{

/// bind the ter type to its file extension, for the work and the export folder
void RegisterTerrainPathMappings();
/// the texture a terrain file names, the fallback (a URN) when it names none
IO::Path TerrainTexture(const Util::String& name, const char* fallback);
/// the model a vegetation mesh names, the fallback (a URN) when it names none
IO::Path TerrainModel(const Util::String& name, const char* fallback);
/// read a terrain file, the exported one or with fromWork the work file. False when it isn't there
/// or can't be read
bool ReadTerrainFile(const Resources::ResourceName& terrain, Render::TerrainResourceT& out, bool fromWork = false);
/// write the work file. False when it can't be written
bool WriteTerrainWorkFile(const Resources::ResourceName& terrain, const Render::TerrainResourceT& resource);
/// a terrain with nothing set, what a Terrain component without a file shows: flat, one biome
void DefaultTerrainResource(Render::TerrainResourceT& out);

} // namespace GraphicsFeature
