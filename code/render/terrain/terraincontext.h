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
    Graphics::GraphicsEntityId sun;
};

struct BiomeSetupSettings
{
    float slopeThreshold;
    float heightThreshold;
    float uvScaleFactor;
    bool useMaterialWeights;
    Resources::ResourceName weights;
};

struct BiomeMaterial
{
    Resources::ResourceName albedo;
    Resources::ResourceName normal;
    Resources::ResourceName material;
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
        const Resources::ResourceName& decisionMap);

    /// setup a new biome
    static TerrainBiomeId CreateBiome(
        BiomeSetupSettings settings,
        BiomeMaterial flatMaterial,
        BiomeMaterial slopeMaterial,
        BiomeMaterial heightMaterial,
        BiomeMaterial heightSlopeMaterial,
        const Resources::ResourceName& mask
    );

    /// set biome slope threshold
    static void SetBiomeSlopeThreshold(TerrainBiomeId id, float threshold);
    /// set biome height threshold
    static void SetBiomeHeightThreshold(TerrainBiomeId id, float threshold);

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
        Resources::ResourceId normalMap;
        Resources::ResourceId decisionMap;

        CoreGraphics::ResourceTableId patchTable;

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
        BiomeSetupSettings,
        uint32_t
    > TerrainBiomeAllocator;
    static TerrainBiomeAllocator terrainBiomeAllocator;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};

} // namespace Terrain
