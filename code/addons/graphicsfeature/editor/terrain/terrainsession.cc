//------------------------------------------------------------------------------
//  @file terrainsession.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "graphicsfeature/editor/terrain/terrainsession.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/components/terrain.h"
#include "graphicsfeature/terrain/terrainfile.h"
#include "neweditor/model/scene.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/commands/setcomponentcommand.h"
#include "editorassets/livebatcher.h"
#include "toolkitutil/asset/assetimporter.h"
#include "toolkitutil/logger.h"
#include "terrain/terraincontext.h"
#include "vegetation/vegetationcontext.h"
#include "coregraphics/image.h"
#include "coregraphics/barrier.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/commandbuffer.h"
#include "resources/resourceserver.h"
#include "io/ioserver.h"
#include "game/world.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::TerrainSession, 'TRSS', Core::RefCounted);

namespace
{

/// the maps are one texel a meter, this keeps a typo in the world size from asking for gigabytes
constexpr int MinMapSize = 64;
constexpr int MaxMapSize = 8192;

//------------------------------------------------------------------------------
/**
    One stroke, the texels it changed before and after
*/
class TerrainStrokeCommand : public Editor::EditCommand
{
    __DeclareClass(TerrainStrokeCommand)
public:
    const char* Name() const override { return this->map == TerrainSession::Map::Height ? "Sculpt terrain" : "Paint terrain"; }
    bool Execute() override
    {
        if (this->session.isvalid() && this->session->IsOpen())
            this->session->WriteRegion(this->map, this->index, this->region, this->after);
        return true;
    }
    bool Unexecute() override
    {
        if (this->session.isvalid() && this->session->IsOpen())
            this->session->WriteRegion(this->map, this->index, this->region, this->before);
        return true;
    }

    Ptr<TerrainSession> session;
    TerrainSession::Map map = TerrainSession::Map::Height;
    IndexT index = 0;
    Math::rectangle<int> region;
    Util::FixedArray<float> before;
    Util::FixedArray<float> after;
};
__ImplementClass(GraphicsFeature::TerrainStrokeCommand, 'TRSC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
*/
float
Sample(const Util::FixedArray<float>& texels, int width, int height, float u, float v)
{
    // texel centers are at i + 0.5, the way the renderer samples
    u = Math::clamp(u - 0.5f, 0.0f, float(width - 1));
    v = Math::clamp(v - 0.5f, 0.0f, float(height - 1));
    const int x0 = (int)u, z0 = (int)v;
    const int x1 = Math::min(x0 + 1, width - 1), z1 = Math::min(z0 + 1, height - 1);
    const float fx = u - x0, fz = v - z0;
    const float a = Math::lerp(texels[z0 * width + x0], texels[z0 * width + x1], fx);
    const float b = Math::lerp(texels[z1 * width + x0], texels[z1 * width + x1], fx);
    return Math::lerp(a, b, fz);
}

//------------------------------------------------------------------------------
/**
*/
void
Resample(const Util::FixedArray<float>& from, int fromWidth, int fromHeight, Util::FixedArray<float>& to, int toWidth, int toHeight)
{
    Util::FixedArray<float> result(toWidth * toHeight);
    const float sx = fromWidth / float(toWidth), sz = fromHeight / float(toHeight);
    for (int z = 0; z < toHeight; z++)
        for (int x = 0; x < toWidth; x++)
            result[z * toWidth + x] = Sample(from, fromWidth, fromHeight, (x + 0.5f) * sx, (z + 0.5f) * sz);
    to = std::move(result);
}

//------------------------------------------------------------------------------
/**
    The renderer's falloff of a brush, full inside the hardness, then smoothly down to the edge
*/
float
Falloff(float distance, float hardness)
{
    if (distance >= 1.0f)
        return 0.0f;
    const float inner = Math::clamp(hardness, 0.0f, 0.999f);
    if (distance <= inner)
        return 1.0f;
    const float t = (distance - inner) / (1.0f - inner);
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

//------------------------------------------------------------------------------
/**
*/
Math::rectangle<int>
Union(const Math::rectangle<int>& a, const Math::rectangle<int>& b)
{
    return Math::rectangle<int>(Math::min(a.left, b.left), Math::min(a.top, b.top), Math::max(a.right, b.right), Math::max(a.bottom, b.bottom));
}

//------------------------------------------------------------------------------
/**
    The value of the first channel of a texel, for the formats a map can be read from
*/
bool
DecodeTexel(CoreGraphics::PixelFormat::Code format, const ubyte* texel, float& out)
{
    switch (format)
    {
    case CoreGraphics::PixelFormat::R8:
    case CoreGraphics::PixelFormat::R8G8B8A8:
    case CoreGraphics::PixelFormat::R8G8B8X8:
        out = texel[0] / 255.0f;
        return true;
    case CoreGraphics::PixelFormat::B8G8R8A8:
        out = texel[2] / 255.0f;
        return true;
    case CoreGraphics::PixelFormat::R16:
    case CoreGraphics::PixelFormat::R16G16:
    case CoreGraphics::PixelFormat::R16G16B16A16:
        out = (*(const uint16_t*)texel) / 65535.0f;
        return true;
    case CoreGraphics::PixelFormat::R32F:
    case CoreGraphics::PixelFormat::R32G32F:
    case CoreGraphics::PixelFormat::R32G32B32A32F:
        out = *(const float*)texel;
        return true;
    default:
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    The first channel of the image a texture was imported from, resampled to the map size. False
    when the texture has no single image to read
*/
bool
LoadSourceImage(const IO::Path& texture, Util::FixedArray<float>& out, int width, int height)
{
    Util::Array<IO::URI> files;
    Util::String error;
    if (!ToolkitUtil::GetTextureSource(texture, files, error) || files.Size() != 1 || !IO::FileExists(files[0]))
        return false;
    CoreGraphics::ImageCreateInfoFile info;
    info.path = files[0];
    info.convertTo32Bit = false;
    CoreGraphics::ImageId image = CoreGraphics::CreateImage(info);
    const ubyte* data = CoreGraphics::ImageGetBuffer(image);
    const CoreGraphics::ImageDimensions dims = CoreGraphics::ImageGetDimensions(image);
    if (data == nullptr || dims.width <= 0 || dims.height <= 0)
    {
        CoreGraphics::DestroyImage(image);
        return false;
    }
    // the image is read as four channels whatever the file has
    const CoreGraphics::ImageChannelPrimitive primitive = CoreGraphics::ImageGetChannelPrimitive(image);
    Util::FixedArray<float> source(dims.width * dims.height);
    for (SizeT i = 0; i < source.Size(); i++)
    {
        switch (primitive)
        {
        case CoreGraphics::Bit16UInt: source[i] = ((const uint16_t*)data)[i * 4] / 65535.0f; break;
        case CoreGraphics::Bit32Float: source[i] = ((const float*)data)[i * 4]; break;
        default: source[i] = data[i * 4] / 255.0f; break;
        }
    }
    CoreGraphics::DestroyImage(image);
    Resample(source, dims.width, dims.height, out, width, height);
    return true;
}

} // namespace

//------------------------------------------------------------------------------
/**
*/
TerrainSession::TerrainSession() :
    scene(nullptr),
    entity(Game::Entity::Invalid()),
    graphicsEntity(Graphics::InvalidGraphicsEntityId),
    width(0),
    height(0),
    open(false),
    dirty(false),
    terrainChanged(false)
{
}

//------------------------------------------------------------------------------
/**
*/
TerrainSession::~TerrainSession()
{
    // the scene can be gone by now
    this->Close(false);
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::Open(Editor::Scene* scene, Game::Entity entity)
{
    n_assert(!this->open);
    const Game::ComponentId component = Game::GetComponentId<Terrain>();
    const bool has = scene->HasInstance(entity) && scene->GetAuthoringWorld()->HasComponent(entity, component);
    const Terrain* terrain = has ? (const Terrain*)scene->GetComponentData(entity, component) : nullptr;
    if (terrain == nullptr)
        return false;

    this->scene = scene;
    this->entity = entity;
    this->entityGuid = scene->GetEditable(entity).guid;
    this->file = terrain->resource.IsValid() ? terrain->resource.Value() : "";

    // the work file is what was saved last, the export can lag behind it
    if (!ReadTerrainFile(terrain->resource, this->resource, true) && !ReadTerrainFile(terrain->resource, this->resource, false))
        DefaultTerrainResource(this->resource);
    for (auto& biome : this->resource.biomes)
    {
        while (biome->materials.size() < 4)
            biome->materials.push_back(std::make_unique<Render::BiomeMaterialT>());
    }

    this->width = Math::clamp((int)this->resource.world_size_x, MinMapSize, MaxMapSize);
    this->height = Math::clamp((int)this->resource.world_size_z, MinMapSize, MaxMapSize);
    this->resource.world_size_x = (float)this->width;
    this->resource.world_size_z = (float)this->height;

    this->LoadMap(this->heights, this->resource.height_map, 0.0f, true, "Terrain editor heights");
    this->biomes.Clear();
    for (IndexT i = 0; i < (IndexT)this->resource.biomes.size(); i++)
    {
        this->biomes.Append(EditMap());
        this->LoadMap(this->biomes.Back(), this->resource.biomes[i]->mask, 1.0f, false, "Terrain editor biome mask");
    }
    this->plants.Clear();
    for (IndexT i = 0; i < (IndexT)this->resource.vegetation.size(); i++)
    {
        this->plants.Append(EditMap());
        this->LoadMap(this->plants.Back(), this->resource.vegetation[i]->mask, 0.0f, false, "Terrain editor plant mask");
    }

    this->graphicsEntity = Graphics::InvalidGraphicsEntityId;
    this->terrainChanged = true;
    this->dirty = false;
    this->open = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::Close(bool restore)
{
    if (!this->open)
        return;
    this->stroke = StrokeState();

    // the renderer can still have the textures bound this frame, they are only destroyed delayed
    DestroyMapTexture(this->heights);
    for (EditMap& map : this->biomes)
        DestroyMapTexture(map);
    for (EditMap& map : this->plants)
        DestroyMapTexture(map);
    this->heights = EditMap();
    this->biomes.Clear();
    this->plants.Clear();

    // the terrain keeps drawing with what it had, set it up from its file again so it doesn't use
    // textures that are gone
    if (this->graphicsEntity != Graphics::InvalidGraphicsEntityId && ::Terrain::TerrainContext::IsEntityRegistered(this->graphicsEntity))
    {
        this->graphicsEntity = Graphics::InvalidGraphicsEntityId;
        if (restore && this->scene != nullptr && this->scene->HasInstance(this->entity) && this->scene->GetAuthoringWorld()->HasComponent(this->entity, Game::GetComponentId<Terrain>()))
        {
            const Game::ComponentId component = Game::GetComponentId<Terrain>();
            this->scene->RestoreComponent(this->entity, component);
        }
    }
    this->scene = nullptr;
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::IsOpen() const
{
    return this->open;
}

//------------------------------------------------------------------------------
/**
*/
Editor::Scene*
TerrainSession::GetScene() const
{
    return this->scene;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
TerrainSession::GetEntity() const
{
    return this->entity;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Guid&
TerrainSession::GetEntityGuid() const
{
    return this->entityGuid;
}

//------------------------------------------------------------------------------
/**
*/
Graphics::GraphicsEntityId
TerrainSession::GetGraphicsEntity() const
{
    return this->graphicsEntity;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
TerrainSession::GetFile() const
{
    return this->file;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
TerrainSession::SuggestFile() const
{
    Util::String name = this->scene != nullptr ? this->scene->GetEditable(this->entity).name : "terrain";
    name.ReplaceChars(" :/\\", '_');
    if (name.IsEmpty())
        name = "terrain";
    return Util::Format("terrain:%s:ter", name.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
Render::TerrainResourceT&
TerrainSession::GetResource()
{
    return this->resource;
}

//------------------------------------------------------------------------------
/**
*/
int
TerrainSession::GetMapWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
int
TerrainSession::GetMapHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::Update()
{
    if (!this->open)
        return false;
    if (!this->scene->HasInstance(this->entity))
        return false;
    const Game::ComponentId component = Game::GetComponentId<Terrain>();
    if (!this->scene->GetAuthoringWorld()->HasComponent(this->entity, component))
        return false;
    const Terrain* authored = (const Terrain*)this->scene->GetComponentData(this->entity, component);
    // the inspector can point the component at another file, that is another terrain
    const Util::String named = authored->resource.IsValid() ? authored->resource.Value() : "";
    if (named != this->file)
        return false;

    Game::World* simWorld = this->scene->GetSimWorld();
    const Game::Entity simEntity = this->scene->ToGameEntity(this->entity);
    if (simWorld == nullptr || simEntity == Game::Entity::Invalid() || !simWorld->IsValid(simEntity) || !simWorld->HasComponent<Terrain>(simEntity))
    {
        this->graphicsEntity = Graphics::InvalidGraphicsEntityId;
        return true;
    }
    const Graphics::GraphicsEntityId gfx = simWorld->GetComponent<Terrain>(simEntity).graphicsEntityId;
    if (gfx == Graphics::InvalidGraphicsEntityId || !::Terrain::TerrainContext::IsEntityRegistered(gfx))
    {
        this->graphicsEntity = Graphics::InvalidGraphicsEntityId;
        return true;
    }

    // a terrain the sim world set up again is built from what the session holds
    if (gfx != this->graphicsEntity || this->terrainChanged)
    {
        this->graphicsEntity = gfx;
        this->terrainChanged = false;
        this->Rebuild();
    }

    bool invalidate = this->heights.hasDirty;
    Upload(this->heights, true);
    for (EditMap& map : this->biomes)
    {
        invalidate |= map.hasDirty;
        Upload(map, false);
    }
    for (EditMap& map : this->plants)
        Upload(map, false);

    this->AttachMaps();
    if (invalidate)
        ::Terrain::TerrainContext::InvalidateTerrain(this->graphicsEntity);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::AttachMaps()
{
    const Graphics::GraphicsEntityId gfx = this->graphicsEntity;
    ::Terrain::TerrainContext::SetHeightmap(gfx, this->heights.texture);
    for (IndexT i = 0; i < this->biomes.Size(); i++)
        ::Terrain::TerrainContext::SetBiomeMask(gfx, ::Terrain::TerrainBiomeId(i), this->biomes[i].texture);
    if (Vegetation::VegetationContext::IsEntityRegistered(gfx))
    {
        for (IndexT i = 0; i < this->plants.Size(); i++)
            Vegetation::VegetationContext::SetMask(gfx, i, this->plants[i].texture);
        Vegetation::VegetationContext::Invalidate(gfx);
    }
}

//------------------------------------------------------------------------------
/**
    The maps the renderer would load from the file are the session's textures instead. It is given
    system textures to load, which are there already, and the session's after
*/
void
TerrainSession::Rebuild()
{
    const Graphics::GraphicsEntityId gfx = this->graphicsEntity;
    const Render::TerrainResourceT& res = this->resource;

    ::Terrain::TerrainCreateInfo createInfo;
    createInfo.minHeight = res.min_height;
    createInfo.maxHeight = Math::max(res.max_height, res.min_height + 0.01f);
    createInfo.quadsPerTileX = res.quads_per_tile_x;
    createInfo.quadsPerTileY = res.quads_per_tile_z;
    createInfo.tileWidth = res.tile_width;
    createInfo.tileHeight = res.tile_height;
    createInfo.width = (float)this->width;
    createInfo.height = (float)this->height;
    createInfo.heightMap = IO::Path::Parse("system:black:tex");
    createInfo.decisionMap = TerrainTexture(res.decision_map, "system:white:tex");
    createInfo.enableRayTracing = false;
    ::Terrain::TerrainContext::SetupTerrain(gfx, createInfo);

    for (IndexT i = 0; i < (IndexT)res.biomes.size(); i++)
    {
        const Render::BiomeT& biome = *res.biomes[i];
        ::Terrain::BiomeSettings settings;
        settings.biomeMask = IO::Path::Parse("system:white:tex");
        for (int layer = 0; layer < 4 && layer < (int)biome.materials.size(); layer++)
        {
            settings.materials[layer].albedo = TerrainTexture(biome.materials[layer]->albedo, "system:white:tex");
            settings.materials[layer].normal = TerrainTexture(biome.materials[layer]->normals, "system:nobump:tex");
            settings.materials[layer].material = TerrainTexture(biome.materials[layer]->material, "system:default_material:tex");
        }
        settings.biomeParameters.slopeThreshold = biome.slope_threshold;
        settings.biomeParameters.heightThreshold = biome.height_threshold;
        settings.biomeParameters.uvScaleFactor = biome.uv_scale_factor;
        ::Terrain::TerrainContext::CreateBiome(gfx, settings);
    }

    // a context entity is only let go at the end of the frame, so the one there is reused
    if (Vegetation::VegetationContext::IsEntityRegistered(gfx))
        Vegetation::VegetationContext::ClearMeshes(gfx);
    else
    {
        Vegetation::VegetationContext::RegisterEntity(gfx);
        Vegetation::VegetationContext::Setup(gfx);
    }
    for (IndexT i = 0; i < (IndexT)res.vegetation.size(); i++)
    {
        const Render::VegetationMeshT& plant = *res.vegetation[i];
        Vegetation::VegetationMeshSetup setup;
        setup.mask = IO::Path::Parse("system:black:tex");
        setup.model = TerrainModel(plant.model, "system:box:mdl");
        setup.slopeThreshold = plant.slope_threshold;
        setup.heightThreshold = plant.height_threshold;
        setup.density = plant.density;
        Vegetation::VegetationContext::SetupMesh(gfx, setup);
    }

    this->AttachMaps();
    ::Terrain::TerrainContext::InvalidateTerrain(gfx);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::ApplyStructure()
{
    const int newWidth = Math::clamp((int)this->resource.world_size_x, MinMapSize, MaxMapSize);
    const int newHeight = Math::clamp((int)this->resource.world_size_z, MinMapSize, MaxMapSize);
    this->resource.world_size_x = (float)newWidth;
    this->resource.world_size_z = (float)newHeight;
    if (newWidth != this->width || newHeight != this->height)
        this->Resize(newWidth, newHeight);
    this->terrainChanged = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::ApplyBiome(IndexT biome)
{
    this->dirty = true;
    if (this->graphicsEntity == Graphics::InvalidGraphicsEntityId || biome >= (IndexT)this->resource.biomes.size())
        return;
    const Render::BiomeT& res = *this->resource.biomes[biome];
    ::Terrain::TerrainContext::SetBiomeRules(this->graphicsEntity, ::Terrain::TerrainBiomeId(biome), res.slope_threshold, res.height_threshold, res.uv_scale_factor);
    for (int layer = 0; layer < 4 && layer < (int)res.materials.size(); layer++)
    {
        ::Terrain::TerrainContext::SetBiomeLayer(this->graphicsEntity, ::Terrain::TerrainBiomeId(biome), ::Terrain::BiomeSettings::BiomeMaterialLayer(layer),
            TerrainTexture(res.materials[layer]->albedo, "system:white:tex"),
            TerrainTexture(res.materials[layer]->normals, "system:nobump:tex"),
            TerrainTexture(res.materials[layer]->material, "system:default_material:tex"));
    }
    ::Terrain::TerrainContext::InvalidateTerrain(this->graphicsEntity);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::ApplyPlant(IndexT plant)
{
    this->dirty = true;
    if (this->graphicsEntity == Graphics::InvalidGraphicsEntityId || plant >= (IndexT)this->resource.vegetation.size())
        return;
    if (!Vegetation::VegetationContext::IsEntityRegistered(this->graphicsEntity))
        return;
    const Render::VegetationMeshT& res = *this->resource.vegetation[plant];
    Vegetation::VegetationContext::SetDensity(this->graphicsEntity, plant, res.density);
    // the model and the thresholds are read when a mesh is set up
    this->terrainChanged = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::AddBiome()
{
    if (this->resource.biomes.size() >= ::Terrain::MAX_BIOMES)
        return;
    auto biome = std::make_unique<Render::BiomeT>();
    biome->name = Util::Format("Biome %d", (int)this->resource.biomes.size() + 1);
    biome->mask = "";
    biome->slope_threshold = 0.5f;
    biome->height_threshold = 0.5f;
    biome->uv_scale_factor = 1.0f;
    for (int i = 0; i < 4; i++)
    {
        auto material = std::make_unique<Render::BiomeMaterialT>();
        material->albedo = "system:white:tex";
        material->normals = "system:nobump:tex";
        material->material = "system:default_material:tex";
        biome->materials.push_back(std::move(material));
    }
    this->resource.biomes.push_back(std::move(biome));

    // a new biome covers nothing until it is painted
    EditMap map;
    map.texels.Resize(this->width * this->height);
    map.texels.Fill(0.0f);
    this->CreateMapTexture(map, false, "Terrain editor biome mask");
    this->biomes.Append(std::move(map));
    this->terrainChanged = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::RemoveBiome(IndexT biome)
{
    if (biome >= this->biomes.Size())
        return;
    DestroyMapTexture(this->biomes[biome]);
    this->biomes.EraseIndex(biome);
    this->resource.biomes.erase(this->resource.biomes.begin() + biome);
    this->terrainChanged = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::AddPlant()
{
    if (this->resource.vegetation.size() >= Vegetation::MAX_MESH_INFOS)
        return;
    auto plant = std::make_unique<Render::VegetationMeshT>();
    plant->name = Util::Format("Plant %d", (int)this->resource.vegetation.size() + 1);
    plant->mask = "";
    plant->model = "system:box:mdl";
    plant->slope_threshold = 0.5f;
    plant->height_threshold = 1.0f;
    plant->density = 1.0f;
    this->resource.vegetation.push_back(std::move(plant));

    EditMap map;
    map.texels.Resize(this->width * this->height);
    map.texels.Fill(0.0f);
    this->CreateMapTexture(map, false, "Terrain editor plant mask");
    this->plants.Append(std::move(map));
    this->terrainChanged = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::RemovePlant(IndexT plant)
{
    if (plant >= this->plants.Size())
        return;
    DestroyMapTexture(this->plants[plant]);
    this->plants.EraseIndex(plant);
    this->resource.vegetation.erase(this->resource.vegetation.begin() + plant);
    this->terrainChanged = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
TerrainSession::EditMap*
TerrainSession::GetMap(Map map, IndexT index)
{
    switch (map)
    {
    case Map::Height: return &this->heights;
    case Map::Biome: return index < this->biomes.Size() ? &this->biomes[index] : nullptr;
    case Map::Plant: return index < this->plants.Size() ? &this->plants[index] : nullptr;
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const TerrainSession::EditMap*
TerrainSession::GetMap(Map map, IndexT index) const
{
    return const_cast<TerrainSession*>(this)->GetMap(map, index);
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::TextureId
TerrainSession::GetMapTexture(Map map, IndexT index) const
{
    const EditMap* edit = this->GetMap(map, index);
    return edit != nullptr ? edit->texture : CoreGraphics::InvalidTextureId;
}

//------------------------------------------------------------------------------
/**
    The heightmap is 16 bits, as the renderer reads it, the masks are 8
*/
void
TerrainSession::CreateMapTexture(EditMap& map, bool height, const char* name)
{
    DestroyMapTexture(map);
    const SizeT count = this->width * this->height;
    Util::FixedArray<uint8_t> bytes(count * (height ? 2 : 1));
    if (height)
    {
        uint16_t* out = (uint16_t*)bytes.Begin();
        for (SizeT i = 0; i < count; i++)
            out[i] = (uint16_t)(Math::clamp(map.texels[i], 0.0f, 1.0f) * 65535.0f + 0.5f);
    }
    else
    {
        for (SizeT i = 0; i < count; i++)
            bytes[i] = (uint8_t)(Math::clamp(map.texels[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }

    CoreGraphics::TextureCreateInfo info;
    info.name = name;
    info.width = this->width;
    info.height = this->height;
    info.format = height ? CoreGraphics::PixelFormat::R16 : CoreGraphics::PixelFormat::R8;
    info.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination | CoreGraphics::TextureUsage::TransferSource;
    info.data = bytes.Begin();
    info.dataSize = bytes.ByteSize();
    map.texture = CoreGraphics::CreateTexture(info);
    map.hasDirty = false;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::DestroyMapTexture(EditMap& map)
{
    if (map.texture != CoreGraphics::InvalidTextureId)
        CoreGraphics::DestroyTexture(map.texture);
    map.texture = CoreGraphics::InvalidTextureId;
}

//------------------------------------------------------------------------------
/**
    A texture that can't be read on the CPU, a compressed one, is the fallback, except the system's
    black and white which are what their names say
*/
void
TerrainSession::LoadMap(EditMap& map, const Util::String& texture, float fallback, bool height, const char* name)
{
    map.texels.Resize(this->width * this->height);
    map.texels.Fill(fallback);

    const IO::Path path = TerrainTexture(texture, height ? "system:black:tex" : (fallback > 0.5f ? "system:white:tex" : "system:black:tex"));
    const Util::String urn = path.AsString();
    if (urn == "system:black:tex")
        map.texels.Fill(0.0f);
    else if (urn == "system:white:tex")
        map.texels.Fill(1.0f);
    else if (LoadSourceImage(path, map.texels, this->width, this->height))
    {
        // read from the image it was imported from, which is what a save writes
    }
    else
    {
        // the texture's upload may not have run yet when it is read back, so this is for textures
        // that have no source image
        Resources::ResourceId res = Resources::CreateResource(path, "terrain editor", nullptr, nullptr, { .immediate = true, .stream = false });
        const CoreGraphics::TextureId tex = CoreGraphics::TextureId(res);
        const CoreGraphics::PixelFormat::Code format = res != Resources::InvalidResourceId ? CoreGraphics::TextureGetPixelFormat(tex) : CoreGraphics::PixelFormat::InvalidPixelFormat;
        float probe;
        const ubyte zero[16] = {};
        const bool loaded = res != Resources::InvalidResourceId && Resources::ResourceServer::Instance()->GetState(res) == Resources::Resource::Loaded;
        if (!loaded || !DecodeTexel(format, zero, probe))
            n_warning("Terrain editor: '%s' can't be read back, it is compressed or missing. The map starts at %.0f\n", urn.AsCharPtr(), fallback);
        else
        {
            const CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(tex);
            CoreGraphics::ImageId image = CoreGraphics::CreateImage(tex, CoreGraphics::PipelineStage::AllShadersRead);
            const ubyte* data = CoreGraphics::ImageGetBuffer(image);
            const SizeT stride = CoreGraphics::PixelFormat::ToSize(format);
            Util::FixedArray<float> source(dims.width * dims.height);
            for (SizeT i = 0; i < source.Size(); i++)
                DecodeTexel(format, data + i * stride, source[i]);
            CoreGraphics::DestroyImage(image);
            Resample(source, dims.width, dims.height, map.texels, this->width, this->height);
        }
        if (res != Resources::InvalidResourceId)
            Resources::DiscardResource(res);
    }
    this->CreateMapTexture(map, height, name);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::Resize(int newWidth, int newHeight)
{
    Resample(this->heights.texels, this->width, this->height, this->heights.texels, newWidth, newHeight);
    for (EditMap& map : this->biomes)
        Resample(map.texels, this->width, this->height, map.texels, newWidth, newHeight);
    for (EditMap& map : this->plants)
        Resample(map.texels, this->width, this->height, map.texels, newWidth, newHeight);
    this->width = newWidth;
    this->height = newHeight;
    this->CreateMapTexture(this->heights, true, "Terrain editor heights");
    for (EditMap& map : this->biomes)
        this->CreateMapTexture(map, false, "Terrain editor biome mask");
    for (EditMap& map : this->plants)
        this->CreateMapTexture(map, false, "Terrain editor plant mask");
    // a stroke's texels don't fit the new size
    this->stroke = StrokeState();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::Touch(EditMap& map, const Math::rectangle<int>& region)
{
    map.dirty = map.hasDirty ? Union(map.dirty, region) : region;
    map.hasDirty = true;
}

//------------------------------------------------------------------------------
/**
    Only the texels that changed are sent, through the upload ring and a copy on the setup command
    buffer, which runs before the frame draws
*/
void
TerrainSession::Upload(EditMap& map, bool height)
{
    if (!map.hasDirty || map.texture == CoreGraphics::InvalidTextureId)
        return;
    const CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(map.texture);
    const int left = Math::clamp(map.dirty.left, 0, dims.width), right = Math::clamp(map.dirty.right, 0, dims.width);
    const int top = Math::clamp(map.dirty.top, 0, dims.height), bottom = Math::clamp(map.dirty.bottom, 0, dims.height);
    map.hasDirty = false;
    if (right <= left || bottom <= top)
        return;

    const int w = right - left, h = bottom - top;
    const SizeT texelSize = height ? 2 : 1;
    Util::FixedArray<uint8_t> bytes(w * h * texelSize);
    for (int z = 0; z < h; z++)
    {
        const float* row = map.texels.Begin() + (top + z) * dims.width + left;
        if (height)
        {
            uint16_t* out = (uint16_t*)bytes.Begin() + z * w;
            for (int x = 0; x < w; x++)
                out[x] = (uint16_t)(Math::clamp(row[x], 0.0f, 1.0f) * 65535.0f + 0.5f);
        }
        else
        {
            uint8_t* out = bytes.Begin() + z * w;
            for (int x = 0; x < w; x++)
                out[x] = (uint8_t)(Math::clamp(row[x], 0.0f, 1.0f) * 255.0f + 0.5f);
        }
    }

    auto [alloc, buffer] = CoreGraphics::UploadArray(bytes.Begin(), bytes.Size(), texelSize);
    if (buffer == CoreGraphics::InvalidBufferId)
    {
        // the ring is full this frame, try again on the next
        Touch(map, Math::rectangle<int>(left, top, right, bottom));
        return;
    }

    CoreGraphics::BufferCopy bufferCopy;
    bufferCopy.offset = alloc.offset;
    bufferCopy.rowLength = 0;
    bufferCopy.imageHeight = 0;
    CoreGraphics::TextureCopy textureCopy;
    textureCopy.layer = 0;
    textureCopy.mip = 0;
    textureCopy.bits = CoreGraphics::ImageBits::ColorBits;
    textureCopy.region = Math::rectangle<SizeT>(left, top, right, bottom);

    const CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer("Terrain editor upload");
    CoreGraphics::BarrierPush(cmdBuf, CoreGraphics::PipelineStage::AllShadersRead, CoreGraphics::PipelineStage::TransferWrite, CoreGraphics::BarrierDomain::Global,
        { CoreGraphics::TextureBarrierInfo{ .tex = map.texture, .subres = CoreGraphics::TextureSubresourceInfo::ColorNoMipNoLayer() } });
    CoreGraphics::CmdCopy(cmdBuf, buffer, { bufferCopy }, map.texture, { textureCopy });
    CoreGraphics::BarrierPop(cmdBuf);
    CoreGraphics::UnlockGraphicsSetupCommandBuffer(cmdBuf);
    CoreGraphics::EnqueueUploadsFlushAndFree({ alloc });
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::HeightAt(float x, float z, float& outHeight) const
{
    if (!this->open)
        return false;
    const float u = x + this->width * 0.5f, v = z + this->height * 0.5f;
    if (u < 0.0f || v < 0.0f || u > this->width || v > this->height)
        return false;
    const float h = Sample(this->heights.texels, this->width, this->height, u, v);
    outHeight = this->resource.min_height + h * (this->resource.max_height - this->resource.min_height);
    return true;
}

//------------------------------------------------------------------------------
/**
    Marched through the terrain's box in steps of half a texel, the step that crosses the ground is
    halved down to a hundredth of a meter
*/
bool
TerrainSession::Raycast(const Math::line& ray, Math::vec3& outHit) const
{
    if (!this->open)
        return false;
    const Math::vec3 origin = Math::vec3(ray.start().x, ray.start().y, ray.start().z);
    const Math::vec3 full = Math::vec3(ray.vec().x, ray.vec().y, ray.vec().z);
    const float length = Math::length(full);
    if (length < TINY)
        return false;
    const Math::vec3 dir = full * (1.0f / length);

    // clip to the terrain's box
    const float hw = this->width * 0.5f, hh = this->height * 0.5f;
    const Math::vec3 boxMin(-hw, this->resource.min_height - 1.0f, -hh), boxMax(hw, this->resource.max_height + 1.0f, hh);
    float tMin = 0.0f, tMax = length;
    for (int axis = 0; axis < 3; axis++)
    {
        const float o = axis == 0 ? origin.x : (axis == 1 ? origin.y : origin.z);
        const float d = axis == 0 ? dir.x : (axis == 1 ? dir.y : dir.z);
        const float lo = axis == 0 ? boxMin.x : (axis == 1 ? boxMin.y : boxMin.z);
        const float hi = axis == 0 ? boxMax.x : (axis == 1 ? boxMax.y : boxMax.z);
        if (Math::abs(d) < 1e-6f)
        {
            if (o < lo || o > hi)
                return false;
            continue;
        }
        float t0 = (lo - o) / d, t1 = (hi - o) / d;
        if (t0 > t1)
            std::swap(t0, t1);
        tMin = Math::max(tMin, t0);
        tMax = Math::min(tMax, t1);
        if (tMin > tMax)
            return false;
    }

    const float step = Math::max(0.5f, (tMax - tMin) / 8192.0f);
    auto above = [this, &origin, &dir](float t, bool& inside) -> float
    {
        const Math::vec3 p = origin + dir * t;
        float ground;
        inside = this->HeightAt(p.x, p.z, ground);
        return p.y - ground;
    };

    bool inside = false;
    float previousT = tMin;
    float previous = above(tMin, inside);
    if (inside && previous <= 0.0f)
    {
        // the ray starts under the ground
        outHit = origin + dir * tMin;
        return true;
    }
    for (float t = tMin + step; t <= tMax + step * 0.5f; t += step)
    {
        const float current = above(Math::min(t, tMax), inside);
        if (inside && current <= 0.0f)
        {
            float lo = previousT, hi = Math::min(t, tMax);
            for (int i = 0; i < 24 && hi - lo > 0.01f; i++)
            {
                const float mid = (lo + hi) * 0.5f;
                bool midInside;
                if (above(mid, midInside) > 0.0f)
                    lo = mid;
                else
                    hi = mid;
            }
            outHit = origin + dir * hi;
            return true;
        }
        previousT = t;
        previous = current;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    The whole map is kept as it was, the stroke only knows at its end which texels it touched
*/
void
TerrainSession::BeginStroke(Map map, IndexT index)
{
    const EditMap* edit = this->GetMap(map, index);
    if (edit == nullptr)
        return;
    this->stroke = StrokeState();
    this->stroke.active = true;
    this->stroke.map = map;
    this->stroke.index = index;
    this->stroke.before = edit->texels;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::IsStroking() const
{
    return this->stroke.active;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::Stroke(const TerrainBrush& brush, const Math::vec3& center, float seconds)
{
    if (!this->stroke.active)
        return;
    EditMap* map = this->GetMap(this->stroke.map, this->stroke.index);
    if (map == nullptr)
        return;

    const float cx = center.x + this->width * 0.5f, cz = center.z + this->height * 0.5f;
    const float radius = Math::max(brush.radius, 0.5f);
    const int x0 = Math::clamp((int)Math::floor(cx - radius), 0, this->width), x1 = Math::clamp((int)Math::ceil(cx + radius), 0, this->width);
    const int z0 = Math::clamp((int)Math::floor(cz - radius), 0, this->height), z1 = Math::clamp((int)Math::ceil(cz + radius), 0, this->height);
    if (x1 <= x0 || z1 <= z0)
        return;

    const float range = Math::max(this->resource.max_height - this->resource.min_height, 0.01f);
    // the share of the way to the target this frame, the same for any frame rate
    const float blend = 1.0f - Math::pow(1.0f - Math::clamp(brush.strength, 0.0f, 0.999f), seconds * 8.0f);
    if (brush.tool == TerrainBrush::Flatten && !this->stroke.hasFlattenTarget)
    {
        this->stroke.flattenTarget = Sample(map->texels, this->width, this->height, cx, cz);
        this->stroke.hasFlattenTarget = true;
    }

    // smoothing reads the texels as they were at the start of this frame
    Util::FixedArray<float> snapshot;
    const int sx0 = Math::max(x0 - 1, 0), sz0 = Math::max(z0 - 1, 0);
    const int sx1 = Math::min(x1 + 1, this->width), sz1 = Math::min(z1 + 1, this->height);
    const int sw = sx1 - sx0;
    if (brush.tool == TerrainBrush::Smooth)
    {
        snapshot.Resize(sw * (sz1 - sz0));
        for (int z = sz0; z < sz1; z++)
            memcpy(&snapshot[(z - sz0) * sw], &map->texels[z * this->width + sx0], sw * sizeof(float));
    }

    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            const float dx = x + 0.5f - cx, dz = z + 0.5f - cz;
            const float distance = brush.shape == TerrainBrush::Square ? Math::max(Math::abs(dx), Math::abs(dz)) : Math::sqrt(dx * dx + dz * dz);
            const float weight = Falloff(distance / radius, brush.hardness);
            if (weight <= 0.0f)
                continue;
            float& value = map->texels[z * this->width + x];
            switch (brush.tool)
            {
            case TerrainBrush::Raise:
                value += brush.rate * Math::max(brush.strength, 0.01f) * seconds * weight / range;
                break;
            case TerrainBrush::Lower:
                value -= brush.rate * Math::max(brush.strength, 0.01f) * seconds * weight / range;
                break;
            case TerrainBrush::Flatten:
                value = Math::lerp(value, this->stroke.flattenTarget, blend * weight);
                break;
            case TerrainBrush::Smooth:
            {
                float sum = 0.0f;
                int count = 0;
                for (int oz = -1; oz <= 1; oz++)
                {
                    for (int ox = -1; ox <= 1; ox++)
                    {
                        const int nx = x + ox, nz = z + oz;
                        if (nx < sx0 || nx >= sx1 || nz < sz0 || nz >= sz1)
                            continue;
                        sum += snapshot[(nz - sz0) * sw + (nx - sx0)];
                        count++;
                    }
                }
                value = Math::lerp(value, sum / count, blend * weight);
                break;
            }
            case TerrainBrush::Paint:
                value = Math::lerp(value, 1.0f, blend * weight);
                break;
            case TerrainBrush::Erase:
                value = Math::lerp(value, 0.0f, blend * weight);
                break;
            }
            value = Math::clamp(value, 0.0f, 1.0f);
        }
    }

    const Math::rectangle<int> region(x0, z0, x1, z1);
    Touch(*map, region);
    this->stroke.region = this->stroke.touched ? Union(this->stroke.region, region) : region;
    this->stroke.touched = true;
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::EndStroke()
{
    if (!this->stroke.active)
        return;
    const EditMap* map = this->GetMap(this->stroke.map, this->stroke.index);
    if (map != nullptr && this->stroke.touched && this->stroke.before.Size() == map->texels.Size())
    {
        const Math::rectangle<int>& r = this->stroke.region;
        const int w = r.width(), h = r.height();
        Ptr<TerrainStrokeCommand> command = TerrainStrokeCommand::Create();
        command->session = this;
        command->map = this->stroke.map;
        command->index = this->stroke.index;
        command->region = r;
        command->before.Resize(w * h);
        command->after.Resize(w * h);
        for (int z = 0; z < h; z++)
        {
            memcpy(&command->before[z * w], &this->stroke.before[(r.top + z) * this->width + r.left], w * sizeof(float));
            memcpy(&command->after[z * w], &map->texels[(r.top + z) * this->width + r.left], w * sizeof(float));
        }
        if (this->scene != nullptr)
            this->scene->GetCommandManager()->Execute(command);
    }
    this->stroke = StrokeState();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::WriteRegion(Map map, IndexT index, const Math::rectangle<int>& region, const Util::FixedArray<float>& texels)
{
    EditMap* edit = this->GetMap(map, index);
    const int w = region.width(), h = region.height();
    if (edit == nullptr || region.left < 0 || region.top < 0 || region.right > this->width || region.bottom > this->height || texels.Size() != w * h)
        return;
    for (int z = 0; z < h; z++)
        memcpy(&edit->texels[(region.top + z) * this->width + region.left], &texels[z * w], w * sizeof(float));
    Touch(*edit, region);
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::IsDirty() const
{
    return this->dirty;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainSession::MarkDirty()
{
    this->dirty = true;
}

//------------------------------------------------------------------------------
/**
    The image goes to the source folder the terrain's folder has, and is imported next to the
    terrain file without mips or compression, the renderer reads the values as they are
*/
bool
TerrainSession::SaveMap(const EditMap& map, bool height, const Util::String& folder, const Util::String& name, Util::String& outUrn, Util::String& outError)
{
    const IO::URI sourceFolder = ToolkitUtil::GetSourceFolder(IO::Path::Folder(folder));
    IO::CreateDirectory(sourceFolder);
    const IO::URI image = Util::Format("%s/%s.png", sourceFolder.AsString().AsCharPtr(), name.AsCharPtr());

    const SizeT count = this->width * this->height;
    Util::FixedArray<uint8_t> bytes(count * (height ? 2 : 1));
    if (height)
    {
        uint16_t* out = (uint16_t*)bytes.Begin();
        for (SizeT i = 0; i < count; i++)
            out[i] = (uint16_t)(Math::clamp(map.texels[i], 0.0f, 1.0f) * 65535.0f + 0.5f);
    }
    else
    {
        for (SizeT i = 0; i < count; i++)
            bytes[i] = (uint8_t)(Math::clamp(map.texels[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
    CoreGraphics::ImageCreateInfoData info;
    info.format = height ? CoreGraphics::PixelFormat::R16 : CoreGraphics::PixelFormat::R8;
    info.width = this->width;
    info.height = this->height;
    info.depth = 1;
    info.data = bytes.Begin();
    CoreGraphics::ImageId id = CoreGraphics::CreateImage(info);
    const bool written = CoreGraphics::ImageSaveToFile(id, CoreGraphics::ImageContainer::PNG, image);
    CoreGraphics::DestroyImage(id);
    if (!written)
    {
        outError = Util::Format("could not write %s", image.LocalPath().AsCharPtr());
        return false;
    }

    ToolkitUtil::TextureResourceT texture = ToolkitUtil::SetupTextureImportSettingsFromPath(image);
    texture.target_format = height ? ToolkitUtil::TexturePixelFormat_R16 : ToolkitUtil::TexturePixelFormat_R8G8B8A8;
    texture.color_space = ToolkitUtil::TextureColorSpace_Linear;
    texture.generate_mipmaps = false;
    const IO::URI workFolder = IO::Path::Folder(folder).WorkURI("assets");
    IO::CreateDirectory(workFolder);
    ToolkitUtil::Logger logger;
    if (!ToolkitUtil::ImportTexture(image, workFolder, texture, &logger))
    {
        outError = Util::Format("could not import %s", image.LocalPath().AsCharPtr());
        return false;
    }
    const IO::Path urn = IO::Path::File(folder, name, "tex");
    Editor::LiveBatcher::BatchFile(urn);
    outUrn = urn.AsString();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
TerrainSession::Save(const Util::String& target, Util::String& outError)
{
    if (!this->open)
    {
        outError = "no terrain is open";
        return false;
    }
    const Util::String fileName = this->file.IsEmpty() ? target : this->file;
    const IO::Path path = IO::Path::Parse(fileName);
    if (!path.IsFile() || path.GetType() != "ter" || path.GetFolder().IsEmpty())
    {
        outError = Util::Format("'%s' is not a terrain file, folder:file:ter", fileName.AsCharPtr());
        return false;
    }
    const Util::String& folder = path.GetFolder();
    const Util::String& base = path.GetFile();

    Util::String urn;
    if (!this->SaveMap(this->heights, true, folder, base + "_height", urn, outError))
        return false;
    this->resource.height_map = urn;
    for (IndexT i = 0; i < this->biomes.Size(); i++)
    {
        if (!this->SaveMap(this->biomes[i], false, folder, Util::Format("%s_biome%d", base.AsCharPtr(), i), urn, outError))
            return false;
        this->resource.biomes[i]->mask = urn;
    }
    for (IndexT i = 0; i < this->plants.Size(); i++)
    {
        if (!this->SaveMap(this->plants[i], false, folder, Util::Format("%s_plant%d", base.AsCharPtr(), i), urn, outError))
            return false;
        this->resource.vegetation[i]->mask = urn;
    }
    this->resource.world_size_x = (float)this->width;
    this->resource.world_size_z = (float)this->height;

    const Resources::ResourceName name = fileName;
    if (!WriteTerrainWorkFile(name, this->resource))
    {
        outError = Util::Format("could not write %s", path.WorkURI("assets").LocalPath().AsCharPtr());
        return false;
    }
    Editor::LiveBatcher::BatchFile(path);

    // a component that named no file names this one now, which is an undoable change of the scene
    if (this->file.IsEmpty() && this->scene != nullptr)
    {
        const Game::ComponentId component = Game::GetComponentId<Terrain>();
        const bool has = this->scene->HasInstance(this->entity) && this->scene->GetAuthoringWorld()->HasComponent(this->entity, component);
        const Terrain* current = has ? (const Terrain*)this->scene->GetComponentData(this->entity, component) : nullptr;
        if (current != nullptr)
        {
            Terrain value = *current;
            value.resource = name;
            this->file = fileName;
            Ptr<Editor::SetComponentCommand> command = Editor::SetComponentCommand::Create();
            command->scene = this->scene;
            command->entity = this->entity;
            command->component = component;
            command->newValue = Util::Blob(&value, sizeof(value));
            this->scene->GetCommandManager()->Execute(command);
        }
    }
    this->dirty = false;
    return true;
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
