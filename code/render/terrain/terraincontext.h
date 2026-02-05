#pragma once
//------------------------------------------------------------------------------
/**
    Terrain rendering subsystem

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "resources/resourceid.h"
#include "math/bbox.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/texture.h"
#include "coregraphics/resourcetable.h"
#include "coregraphics/window.h"

#include "occupancyquadtree.h"
#include "texturetilecache.h"

#include "jobs/jobs.h"

#include "io/ioserver.h"
#include "coregraphics/load/glimltypes.h"

#include "gpulang/render/terrain/shaders/terrain_tile_write.h"
#include "gpulang/render/terrain/shaders/terrain.h"

namespace Terrain
{

ID_32_TYPE(TerrainBiomeId);

struct TerrainSetupSettings
{
    float minHeight, maxHeight;
    float worldSizeX, worldSizeZ;
    float tileWidth, tileHeight;
    float quadsPerTileX, quadsPerTileY; // vertex density is vertices per meter
};

struct TerrainCreateInfo
{
    float minHeight, maxHeight;         // Max height encoded in heightmap
    float width, height;                // Width and height of the terrain
    float tileWidth, tileHeight;        // Size of each tile, number of tiles becoming (width/tileWidth, height/tileHeight)
    float quadsPerTileX, quadsPerTileY; // Geometric density measured in quads per tile
    Resources::ResourceName heightMap;
    Resources::ResourceName decisionMap;
    bool enableRayTracing;
};

struct BiomeParameters
{
    float slopeThreshold = 0.5f;
    float heightThreshold = 0.5f;
    float uvScaleFactor = 1.0f;
    bool useMaterialWeights = false;
    Resources::ResourceName weights;
};

struct BiomeMaterial
{
    Resources::ResourceName albedo = "systex:white.dds";
    Resources::ResourceId albedoRes;
    Resources::ResourceName normal = "systex:nobump.dds";
    Resources::ResourceId normalRes;
    Resources::ResourceName material = "systex:default_material.dds";
    Resources::ResourceId materialRes;
};

struct BiomeMaterialBuilder
{
    /// Set albedo
    BiomeMaterialBuilder& Albedo(const Resources::ResourceName& name)
    {
        this->material.albedo = name.IsValid() ? name : "systex:white.dds";
        return *this;
    }

    /// Set normal
    BiomeMaterialBuilder& Normal(const Resources::ResourceName& name)
    {
        this->material.normal = name.IsValid() ? name : "systex:nobump.dds";
        return *this;
    }

    /// Set material
    BiomeMaterialBuilder& Material(const Resources::ResourceName& name)
    {
        this->material.material = name.IsValid() ? name : "systex:default_material.dds";
        return *this;
    }

    /// Finish
    BiomeMaterial Finish()
    {
        return this->material;
    }
private:
    BiomeMaterial material;
};

struct BiomeSettings
{
    enum BiomeMaterialLayer : uint8_t
    {
        Flat,         // Material to use on flat surfaces
        Slope,        // Material to use on slanted surfaces
        Height,       // Material to use for surfaces high up
        HeightSlope,  // Material to sue for high up on slanted surface

        NumLayers
    };
    BiomeParameters biomeParameters;
    BiomeMaterial materials[BiomeMaterialLayer::NumLayers];
    Resources::ResourceName biomeMask = "systex:white.dds";
};

struct BiomeSettingsBuilder
{
private:
    enum BuilderBits : uint8_t
    {
        NoBits = 0x0,
        SettingsBit = 0x1,
        FlatMaterialBit = 0x2,
        SlopeMaterialBit = 0x4,
        HeightMaterialBit = 0x8,
        HeightSlopeMaterialBit = 0x10,
        BiomeMask = 0x20,

        AllBits = (BiomeMask << 1) - 1
    };

    uint8_t bits = NoBits;
    BiomeSettings settings;

public:

    /// Builder for settings
    BiomeSettingsBuilder& Parameters(const BiomeParameters& settings)
    {
        this->bits |= BuilderBits::SettingsBit;
        this->settings.biomeParameters = settings;
        return *this;
    }

    /// Builder for flat material
    BiomeSettingsBuilder& FlatMaterial(const BiomeMaterial& material)
    {
        this->bits |= BuilderBits::FlatMaterialBit;
        this->settings.materials[BiomeSettings::BiomeMaterialLayer::Flat] = material;
        return *this;
    }

    /// Builder for slope material
    BiomeSettingsBuilder& SlopeMaterial(const BiomeMaterial& material)
    {
        this->bits |= BuilderBits::SlopeMaterialBit;
        this->settings.materials[BiomeSettings::BiomeMaterialLayer::Slope] = material;
        return *this;
    }

    /// Builder for height material
    BiomeSettingsBuilder& HeightMaterial(const BiomeMaterial& material)
    {
        this->bits |= BuilderBits::HeightMaterialBit;
        this->settings.materials[BiomeSettings::BiomeMaterialLayer::Height] = material;
        return *this;
    }

    /// Builder for height slope material
    BiomeSettingsBuilder& HeightSlopeMaterial(const BiomeMaterial& material)
    {
        this->bits |= BuilderBits::HeightSlopeMaterialBit;
        this->settings.materials[BiomeSettings::BiomeMaterialLayer::HeightSlope] = material;
        return *this;
    }

    /// Builder for biome mask
    BiomeSettingsBuilder& Mask(const Resources::ResourceName& mask)
    {
        this->bits |= BuilderBits::BiomeMask;
        this->settings.biomeMask = mask.IsValid() ? mask : "tex:system/white.dds";
        return *this;
    }

    /// Finish
    BiomeSettings Finish()
    {
        n_assert_msg(this->bits & AllBits, "BiomeSettinsBuilder: All fields must be set before calling finish");
        return this->settings;
    }
};

struct SubTextureUpdateJobUniforms
{
    uint subTextureWorldSize;
    uint maxMip;
    uint physicalTileSize;
};

enum class SubTextureUpdateState : uint8_t
{
    NoChange,           // subtexture remains the same
    Deleted,            // subtexture went to 0 tiles
    Created,            // subtexture was 0 tiles but grew
    Grew,               // subtexture grew to more tiles
    Shrank              // subtexture shrank
};

struct SubTextureUpdateJobOutput
{
    IndexT index;
    uint oldTiles, newTiles;
    uint oldMaxMip, newMaxMip;
    float mipBias;
    Math::uint2 oldCoord;
    SubTextureUpdateState updateState;
};

struct PhysicalTileUpdate
{
    uint constantBufferOffsets[2];
    uint tileOffset[2];
};

struct SubTexture
{
    Math::float2 worldCoordinate;
    Math::uint2 indirectionOffset;
    uint maxMip;
    uint mipBias;
    uint numTiles;
};
const uint SubTextureMaxUpdates = 1024;

struct SubTextureCompressed
{
    Math::float2 worldCoordinate;
    uint32_t : 32;
    uint32_t indirectionX : 12;
    uint32_t indirectionY : 12;
    uint mip : 8;
};


class TerrainContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// constructor
    TerrainContext();
    /// destructor
    virtual ~TerrainContext();

    /// create terrain context
    static void Create();
    /// destroy terrain context
    static void Discard();

    /// setup new terrain surface from texture and settings
    static void SetupTerrain(
        const Graphics::GraphicsEntityId entity, 
        const TerrainCreateInfo& createInfo
    );

    /// setup a new biome
    static TerrainBiomeId CreateBiome(const Graphics::GraphicsEntityId entity, const BiomeSettings& settings);
    /// Destroy biome
    static void DestroyBiome(const Graphics::GraphicsEntityId entity, TerrainBiomeId id);

    /// set biome slope threshold
    static void SetBiomeSlopeThreshold(const Graphics::GraphicsEntityId entity, TerrainBiomeId id, float threshold);
    /// set biome height threshold
    static void SetBiomeHeightThreshold(const Graphics::GraphicsEntityId entity, TerrainBiomeId id, float threshold);

    /// Set the sun entity for terrain shadows
    static void SetSun(const Graphics::GraphicsEntityId sun);

    /// cull terrain patches
    static void CullPatches(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// update sparse texture mips
    static void UpdateLOD(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// render IMGUI
    static void RenderUI(const Graphics::FrameContext& ctx);
    /// clear the tile cache (use when we need to force update the terrain)
    static void ClearCache();

    /// 
    static void SetVisible(bool visible);
    /// 
    static bool GetVisible();

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
#endif

#ifdef WITH_NEBULA_EDITOR
    /// Set heightmap to a system controller texture instead of a resource
    static void SetHeightmap(Graphics::GraphicsEntityId entity, CoreGraphics::TextureId heightmap);
    static void SetBiomeMask(Graphics::GraphicsEntityId entity, TerrainBiomeId biomeId, CoreGraphics::TextureId biomemask);
    static void SetBiomeLayer(Graphics::GraphicsEntityId entity, TerrainBiomeId biomeId, BiomeSettings::BiomeMaterialLayer layer, const Resources::ResourceName& albedo, const Resources::ResourceName& normal, const Resources::ResourceName& material);
    static void SetBiomeRules(Graphics::GraphicsEntityId entity, TerrainBiomeId biomeId, float slopeThreshold, float heightThreshold, float uvScalingFactor);
    static void InvalidateTerrain(Graphics::GraphicsEntityId entity);
#endif

private:

    struct TerrainLoadInfo
    {
        Resources::ResourceName texture;
        float minHeight, maxHeight;
    };

    struct TerrainRuntimeInfo
    {
        Util::Array<Math::bbox> sectionBoxes;
        Util::Array<CoreGraphics::PrimitiveGroup> sectorPrimGroups;
        Util::Array<bool> sectorVisible;
        Util::Array<Util::FixedArray<uint>> sectorUniformOffsets;
        Util::Array<Util::FixedArray<uint>> sectorTileOffsets;
        Util::Array<Math::uint2> sectorTextureTileSize;
        Util::Array<Math::float2> sectorUv;
        Util::Array<float> sectorLod;
        Util::Array<bool> sectorUpdateTextureTile;
        Util::Array<Math::uint3> sectorAllocatedTile;

        float worldWidth, worldHeight;
        float maxHeight, minHeight;
        uint numTilesX, numTilesY;
        uint tileWidth, tileHeight;
        Resources::ResourceId heightMapResource;
        CoreGraphics::TextureId heightMap;
        Resources::ResourceId decisionMapResource;
        CoreGraphics::TextureId decisionMap;
        enum TextureLoadBits
        {
            HeightMapLoaded = 0x1,
            DecisionMapLoaded = 0x2
        };
        uint loadBits;
        uint lowresGenerated;
        bool enableRayTracing;

        Util::FixedArray<CoreGraphics::ResourceTableId> patchTables;
        CoreGraphics::BufferId vbo;
        CoreGraphics::BufferId ibo;
    };

public:
    struct TerrainInstanceInfo
    {
        CoreGraphics::BufferId                                          systemConstants;

        bool                                                            virtualSubtextureBufferUpdate;
        CoreGraphics::BufferSet                                         subtextureStagingBuffers;
        CoreGraphics::BufferId                                          subTextureBuffer;

        CoreGraphics::BufferSet                                         pageUpdateReadbackBuffers;
        CoreGraphics::TextureId                                         indirectionTexture;
        CoreGraphics::TextureId                                         indirectionTextureCopy;

        CoreGraphics::BufferId                                          pageUpdateListBuffer;
        CoreGraphics::BufferId                                          pageStatusBuffer;

        Threading::AtomicCounter                                        numPatchesThisFrame;
        CoreGraphics::BufferSet                                         patchConstants;
        CoreGraphics::BufferId                                          runtimeConstants;
        CoreGraphics::BufferWithStaging                                 tileWriteBufferSet;

        CoreGraphics::TextureId                                         physicalAlbedoCacheBC;
        CoreGraphics::TextureViewId                                     physicalAlbedoCacheBCWrite;
        CoreGraphics::TextureId                                         physicalNormalCacheBC;
        CoreGraphics::TextureViewId                                     physicalNormalCacheBCWrite;
        CoreGraphics::TextureId                                         physicalMaterialCacheBC;
        CoreGraphics::TextureViewId                                     physicalMaterialCacheBCWrite;
        CoreGraphics::TextureId                                         lowresAlbedo;
        CoreGraphics::TextureId                                         lowresNormal;
        CoreGraphics::TextureId                                         lowresMaterial;
        bool                                                            updateLowres = false;

        CoreGraphics::ResourceTableSet                                  systemTable;
        CoreGraphics::ResourceTableId                                   runtimeTable;
        SizeT                                                           numPageBufferUpdateEntries;

        Util::FixedArray<SubTexture>                                    subTextures;
        Util::FixedArray<TerrainSubTexture>                             gpuSubTextures;
        Threading::AtomicCounter                                        subTextureNumOutputs;
        Terrain::SubTextureUpdateJobOutput                              subTextureJobOutputs[SubTextureMaxUpdates];
        OccupancyQuadTree                                               indirectionOccupancy;
        OccupancyQuadTree                                               physicalTextureTileOccupancy;
        TextureTileCache                                                physicalTextureTileCache;

        Util::FixedArray<uint>                                          indirectionMipOffsets;
        Util::FixedArray<uint>                                          indirectionMipSizes;
        Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureCopies;
        Util::Array<IndirectionEntry>                                   indirectionEntryUpdates;
        Util::FixedArray<IndirectionEntry>                              indirectionBuffer;
        CoreGraphics::BufferSet                                         indirectionUploadBuffers;
        Util::FixedArray<uint>                                          indirectionUploadOffsets;

        Util::Array<TerrainTileWrite::TileWrite>                        tileWrites;
        Util::Array<TerrainTileWrite::TileWrite>                        tileWritesThisFrame;

        Util::Array<CoreGraphics::BufferCopy, 4>                        indirectionBufferUpdatesThisFrame;
        Util::Array<CoreGraphics::TextureCopy, 4>                       indirectionTextureUpdatesThisFrame;
        Util::Array<CoreGraphics::TextureCopy, 4>                       indirectionTextureFromCopiesThisFrame;
        Util::Array<CoreGraphics::TextureCopy, 4>                       indirectionTextureToCopiesThisFrame;
        Util::Array<CoreGraphics::BufferCopy, 4>                        indirectionBufferClearsThisFrame;
        Util::Array<CoreGraphics::TextureCopy, 4>                       indirectionTextureClearsThisFrame;
        uint numPixels;

        CoreGraphics::TextureId shadowMap;

        TerrainCreateInfo                                               createInfo;

        Threading::Event                                                *subtexturesFinishedEvent, *sectionCullFinishedEvent;
        Threading::AtomicCounter                                        subtexturesDoneCounter = 0, sectionCullDoneCounter = 0;
        CoreGraphics::BarrierContext                                    barrierContext; 
        uint32_t                                                        pageUpdateListBarrierIndex, pageStatusBufferBarrierIndex, subtextureBufferBarrierIndex, indirectionBarrierIndex, indirectionCopyBarrierIndex, albedoCacheBarrierIndex, materialCacheBarrierIndex, normalCacheBarrierIndex;
        
        Util::Array<IndexT>                                             biomes;
        CoreGraphics::BufferId                                          biomeBuffer;
        Terrain::MaterialLayers::STRUCT                                 biomeMaterials;
        Util::Array<CoreGraphics::TextureId>                            biomeTextures;
        CoreGraphics::TextureId                                         biomeMasks[Terrain::MAX_BIOMES];
        Threading::AtomicCounter                                        biomeLoaded[Terrain::MAX_BIOMES][4];
        uint                                                            biomeLowresGenerated[Terrain::MAX_BIOMES];
        BiomeMaterial                                                   biomeResources[Terrain::MAX_BIOMES][BiomeSettings::BiomeMaterialLayer::NumLayers];
        CoreGraphics::TextureId                                         biomeWeights[Terrain::MAX_BIOMES];
    };
private:

    enum
    {
        Terrain_LoadInfo,
        Terrain_RuntimeInfo,
        Terrain_InstanceInfo
    };

    typedef Ids::IdAllocator<
        TerrainLoadInfo,
        TerrainRuntimeInfo,
        TerrainInstanceInfo
    > TerrainAllocator;
    static TerrainAllocator terrainAllocator;

    enum
    {
        TerrainBiome_Settings,
        TerrainBiome_MaskTexture,
    };

    typedef Ids::IdAllocator<
        BiomeSettings
    > TerrainBiomeAllocator;
    static TerrainBiomeAllocator terrainBiomeAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Terrain
