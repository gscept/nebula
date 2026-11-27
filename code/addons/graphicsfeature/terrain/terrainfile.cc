//------------------------------------------------------------------------------
//  @file terrainfile.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/terrain/terrainfile.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "nflatbuffer/flatbufferinterface.h"

namespace GraphicsFeature
{

//------------------------------------------------------------------------------
/**
*/
void
RegisterTerrainPathMappings()
{
    IO::Path::AddWorkMapping("ter", "nater");
    IO::Path::AddExportMapping("ter", "nater");
}

//------------------------------------------------------------------------------
/**
*/
static IO::Path
ResourcePath(const Util::String& name, const char* type, const char* fallback)
{
    if (name.IsEmpty())
        return IO::Path::Parse(fallback);
    if (name.FindCharIndex(':') != InvalidIndex)
        return IO::Path::Parse(name);
    // folder/file, from before URNs
    return IO::Path::FolderAndFile(type, name);
}

//------------------------------------------------------------------------------
/**
*/
IO::Path
TerrainTexture(const Util::String& name, const char* fallback)
{
    return ResourcePath(name, "tex", fallback);
}

//------------------------------------------------------------------------------
/**
*/
IO::Path
TerrainModel(const Util::String& name, const char* fallback)
{
    return ResourcePath(name, "mdl", fallback);
}

//------------------------------------------------------------------------------
/**
*/
bool
ReadTerrainFile(const Resources::ResourceName& terrain, Render::TerrainResourceT& out, bool fromWork)
{
    if (!terrain.IsValid())
        return false;
    const IO::Path path = IO::Path::Parse(terrain.Value());
    if (!path.IsFile() || path.GetType() != "ter")
    {
        n_warning("Terrain: '%s' is not a terrain, folder:file:ter\n", terrain.Value());
        return false;
    }
    const IO::URI uri = fromWork ? path.WorkURI("assets") : path.ExportURI();
    if (!IO::FileExists(uri))
        return false;

    Ptr<IO::Stream> file = IO::CreateStream(uri);
    file->SetAccessMode(IO::Stream::ReadAccess);
    if (!file->Open())
        return false;
    const void* buf = file->Map();
    const bool ok = buf != nullptr && file->GetSize() > 8;
    if (ok)
        Flat::FlatbufferInterface::DeserializeFlatbuffer<Render::TerrainResource>(out, (const uint8_t*)buf);
    file->Unmap();
    file->Close();
    return ok;
}

//------------------------------------------------------------------------------
/**
*/
bool
WriteTerrainWorkFile(const Resources::ResourceName& terrain, const Render::TerrainResourceT& resource)
{
    const IO::Path path = IO::Path::Parse(terrain.Value());
    if (!path.IsFile() || path.GetType() != "ter")
        return false;
    const IO::URI uri = path.WorkURI("assets");
    IO::CreateDirectory(uri.LocalPath().ExtractDirName());

    Util::Blob blob = Flat::FlatbufferInterface::SerializeFlatbuffer<Render::TerrainResource>(resource);
    Ptr<IO::Stream> file = IO::CreateStream(uri);
    file->SetAccessMode(IO::Stream::WriteAccess);
    if (!file->Open())
        return false;
    file->Write(blob.GetPtr(), blob.Size());
    file->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
DefaultTerrainResource(Render::TerrainResourceT& out)
{
    out = Render::TerrainResourceT();
    out.height_map = "system:black:tex";
    out.decision_map = "system:white:tex";
    auto biome = std::make_unique<Render::BiomeT>();
    biome->name = "Biome 1";
    biome->mask = "system:white:tex";
    for (int i = 0; i < 4; i++)
    {
        auto material = std::make_unique<Render::BiomeMaterialT>();
        material->albedo = "system:white:tex";
        material->normals = "system:nobump:tex";
        material->material = "system:default_material:tex";
        biome->materials.push_back(std::move(material));
    }
    out.biomes.push_back(std::move(biome));
}

} // namespace GraphicsFeature
