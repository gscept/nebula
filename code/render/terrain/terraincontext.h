#pragma once
//------------------------------------------------------------------------------
/**
    Terrain rendering subsystem

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
    SizeT tileWidth, tileHeight;
    float vertexDensityX, vertexDensityY; // vertex density is vertices per meter
};

struct BiomeSetupSettings
{
    float slopeThreshold;
    float heightThreshold;
    float uvScaleFactor;
};

struct BiomeMaterial
{
    Resources::ResourceName albedo;
    Resources::ResourceName normal;
    Resources::ResourceName material;
};

//------------------------------------------------------------------------------
/**
    This type keeps track of subregions of a texture so we can update them when
    we need it. Can handle huge textures, max 16k for compliance with nvidia GPUs.
*/
struct TerrainTextureSource
{
    Ptr<IO::Stream> stream;
    gliml::context source;
    float scaleFactorX, scaleFactorY;
    CoreGraphics::TextureId tex;
    Util::FixedArray<Util::FixedArray<Util::FixedArray<uint>>> pageReferenceCount;

    /// setup source
    void Setup(const Resources::ResourceName& path, bool manualRegister = false);
};

//------------------------------------------------------------------------------
/**
    This type keeps track of individual resident mips and is used within a terrain
    tile to render a repeating pattern. Can handle big textures, max 4k.
*/
struct TerrainMaterialSource
{
    Ptr<IO::Stream> stream;
    gliml::context source;
    CoreGraphics::TextureId tex;
    Util::FixedArray<Util::FixedArray<uint>> mipReferenceCount;

    /// setup source
    void Setup(const Resources::ResourceName& path);
    /// update mip
    void UpdateMip(float oldDistance, float newDistance, CoreGraphics::SubmissionContextId sub);
};

struct TerrainMaterial
{
    TerrainMaterialSource albedo;
    TerrainMaterialSource normals;
    TerrainMaterialSource material;
    TerrainTextureSource mask;

    /// update mip status
    void UpdateMip(float oldDistance, float newDistance, CoreGraphics::SubmissionContextId sub);
};

class TerrainContext : public Graphics::GraphicsContext
{
    _DeclareContext();
public:

    /// constructor
    TerrainContext();
    /// destructor
    virtual ~TerrainContext();

    /// create terrain context
    static void Create(const CoreGraphics::WindowId wnd);
    /// destroy terrain context
    static void Discard();

    /// setup new terrain surface from texture and settings
    static void SetupTerrain(
        const Graphics::GraphicsEntityId entity, 
        const Resources::ResourceName& heightMap, 
        const Resources::ResourceName& decisionMap,
        const Resources::ResourceName& albedoMap,
        const TerrainSetupSettings& settings);

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
        CoreGraphics::TextureId heightMap;
        CoreGraphics::TextureId normalMap;
        CoreGraphics::TextureId decisionMap;
        CoreGraphics::TextureId lowResAlbedoMap;

        CoreGraphics::ConstantBufferId terrainConstants;
        CoreGraphics::ResourceTableId terrainResourceTable;
        
        CoreGraphics::ResourceTableId patchTable;

        CoreGraphics::BufferId vbo;
        CoreGraphics::BufferId ibo;

        TerrainTextureSource albedoSource;
        TerrainTextureSource heightSource;
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

    static Util::Queue<Jobs::JobId> runningJobs;
    static Jobs::JobSyncId jobHostSync;
};

} // namespace Terrain
