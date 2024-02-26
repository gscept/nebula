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

#include "jobs/jobs.h"

#include "io/ioserver.h"
#include "coregraphics/load/glimltypes.h"

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

struct BiomeParameters
{
    float slopeThreshold;
    float heightThreshold;
    float uvScaleFactor;
    bool useMaterialWeights;
    Resources::ResourceName weights;
};

struct BiomeMaterial
{
    Resources::ResourceName albedo = "tex:system/white.dds";
    Resources::ResourceName normal = "tex:system/nobump.dds";
    Resources::ResourceName material = "tex:system/default_material.dds";
};

struct BiomeMaterialBuilder
{
    /// Set albedo
    BiomeMaterialBuilder& Albedo(const Resources::ResourceName& name)
    {
        this->material.albedo = name.IsValid() ? name : "tex:system/white.dds";
        return *this;
    }

    /// Set normal
    BiomeMaterialBuilder& Normal(const Resources::ResourceName& name)
    {
        this->material.normal = name.IsValid() ? name : "tex:system/nobump.dds";
        return *this;
    }

    /// Set material
    BiomeMaterialBuilder& Material(const Resources::ResourceName& name)
    {
        this->material.material = name.IsValid() ? name : "tex:system/default_material.dds";
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
    enum BiomeMaterialLayer : uint8
    {
        Flat,         // Material to use on flat surfaces
        Slope,        // Material to use on slanted surfaces
        Height,       // Material to use for surfaces high up
        HeightSlope,  // Material to sue for high up on slanted surface

        NumLayers
    };
    BiomeParameters biomeParameters;
    BiomeMaterial materials[BiomeMaterialLayer::NumLayers];
    Resources::ResourceName biomeMask;
};

struct BiomeSettingsBuilder
{
private:
    enum BuilderBits : uint8
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

    uint8 bits = NoBits;
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

enum class SubTextureUpdateState : uint8
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

class TerrainContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    /// constructor
    TerrainContext();
    /// destructor
    virtual ~TerrainContext();

    /// create terrain context
    static void Create(const TerrainSetupSettings& settings);
    /// destroy terrain context
    static void Discard();

    /// setup new terrain surface from texture and settings
    static void SetupTerrain(
        const Graphics::GraphicsEntityId entity, 
        const Resources::ResourceName& heightMap, 
        const Resources::ResourceName& decisionMap,
        bool enableRayTracing);

    /// setup a new biome
    static TerrainBiomeId CreateBiome(const BiomeSettings& settings);

    /// set biome slope threshold
    static void SetBiomeSlopeThreshold(TerrainBiomeId id, float threshold);
    /// set biome height threshold
    static void SetBiomeHeightThreshold(TerrainBiomeId id, float threshold);

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

#ifndef PUBLIC_DEBUG    
    /// debug rendering
    static void OnRenderDebug(uint32_t flags);
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
        Resources::ResourceId heightMap;
        Resources::ResourceId decisionMap;
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

    enum
    {
        Terrain_LoadInfo,
        Terrain_RuntimeInfo
    };

    typedef Ids::IdAllocator<
        TerrainLoadInfo,
        TerrainRuntimeInfo
    > TerrainAllocator;
    static TerrainAllocator terrainAllocator;

    enum
    {
        TerrainBiome_Settings,
        TerrainBiome_Index,
        TerrainBiome_MaskTexture,
    };

    typedef Ids::IdAllocator<
        BiomeSettings,
        uint32_t
    > TerrainBiomeAllocator;
    static TerrainBiomeAllocator terrainBiomeAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Terrain
