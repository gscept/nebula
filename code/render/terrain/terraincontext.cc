//------------------------------------------------------------------------------
//  terraincontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "terraincontext.h"
#include "graphics/graphicsserver.h"
#include "frame/framesubgraph.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "dynui/imguicontext.h"
#include "jobs2/jobs2.h"
#include "imgui.h"
#include "renderutil/drawfullscreenquad.h"
#include "core/cvar.h"

#include "lighting/lightcontext.h"

#include "occupancyquadtree.h"
#include "texturetilecache.h"

#include "resources/resourceserver.h"
#include "gliml.h"

#include "terrain/terrain.h"

N_DECLARE_COUNTER(N_TERRAIN_TOTAL_AVAILABLE_DATA, Terrain Total Data Size);

namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
TerrainContext::TerrainBiomeAllocator TerrainContext::terrainBiomeAllocator;

Threading::Event subtexturesFinishedEvent, sectionCullFinishedEvent;
Threading::AtomicCounter subtexturesDoneCounter = 0, sectionCullDoneCounter = 0;

const uint IndirectionTextureSize = 2048;
const uint SubTextureWorldSize = 64;
const uint SubTextureMaxTiles = 256;
const uint IndirectionNumMips = Math::log2(SubTextureMaxTiles) + 1;
const float SubTextureRange = 300.0f; // 300 meter range
const float SubTextureSwapDistance = 10.0f;
const float SubTextureFadeStart = 200.0f;
const uint SubTextureMaxPixels = 65536;
const uint SubTextureMaxUpdates = 1024;

const uint LowresFallbackSize = 4096;
const uint LowresFallbackMips = Math::log2(LowresFallbackSize) + 1;

const uint PhysicalTextureTileSize = 256;
const uint PhysicalTextureTileHalfPadding = 4;
const uint PhysicalTextureTilePadding = PhysicalTextureTileHalfPadding * 2;
const uint PhysicalTextureNumTiles = 32;
const uint PhysicalTextureTilePaddedSize = PhysicalTextureTileSize + PhysicalTextureTilePadding;
const uint PhysicalTextureSize = (PhysicalTextureTileSize) * PhysicalTextureNumTiles;
const uint PhysicalTexturePaddedSize = (PhysicalTextureTilePaddedSize) * PhysicalTextureNumTiles;

const uint TerrainShadowMapSize = 1024;


__ImplementContext(TerrainContext, TerrainContext::terrainAllocator);

struct
{
    TerrainSetupSettings settings;
    CoreGraphics::ShaderId terrainShader;
    CoreGraphics::ShaderProgramId terrainShadowProgram;
    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId systemConstants;
    CoreGraphics::VertexLayoutId vlo;

    CoreGraphics::WindowId wnd;

    CoreGraphics::BufferId biomeBuffer;
    Terrain::MaterialLayers biomeMaterials;
    Util::Array<CoreGraphics::TextureId> biomeTextures;
    CoreGraphics::TextureId biomeMasks[Terrain::MAX_BIOMES];
    CoreGraphics::TextureId biomeWeights[Terrain::MAX_BIOMES];
    Threading::AtomicCounter biomeLoaded[Terrain::MAX_BIOMES];
    uint biomeLowresGenerated[Terrain::MAX_BIOMES];
    IndexT biomeCounter;

    Graphics::GraphicsEntityId sun;

    CoreGraphics::TextureId terrainShadowMap;
    Math::vec4 cachedSunDirection;
    bool updateShadowMap;
    CoreGraphics::ResourceTableId terrainShadowResourceTable;

    CoreGraphics::TextureId terrainPosBuffer;
    Memory::ArenaAllocator<sizeof(Frame::FrameCode) * 4> frameOpAllocator;
    
    float mipLoadDistance = 1500.0f;
    float mipRenderPadding = 150.0f;
    SizeT layers;

    bool debugRender;
    bool renderToggle;

} terrainState;

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

struct SubTextureCompressed
{
    Math::float2 worldCoordinate;
    uint32 : 32;
    uint32 indirectionX : 12;
    uint32 indirectionY : 12;
    uint mip : 8;
};

struct
{
    CoreGraphics::ShaderProgramId                                   terrainPrepassProgram;
    CoreGraphics::PipelineId                                        terrainPrepassPipeline;
    CoreGraphics::ShaderProgramId                                   terrainPageClearUpdateBufferProgram;
    CoreGraphics::ShaderProgramId                                   terrainResolveProgram;
    CoreGraphics::PipelineId                                        terrainResolvePipeline;
    CoreGraphics::ShaderProgramId                                   terrainTileUpdateProgram;
    CoreGraphics::ShaderProgramId                                   terrainTileFallbackProgram;

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

    CoreGraphics::TextureId                                         physicalAlbedoCache;
    CoreGraphics::TextureId                                         physicalNormalCache;
    CoreGraphics::TextureId                                         physicalMaterialCache;
    CoreGraphics::TextureId                                         lowresAlbedo;
    CoreGraphics::TextureId                                         lowresNormal;
    CoreGraphics::TextureId                                         lowresMaterial;
    bool                                                            updateLowres = false;

    CoreGraphics::ResourceTableSet                                  virtualTerrainSystemResourceTable;
    CoreGraphics::ResourceTableId                                   virtualTerrainRuntimeResourceTable;
    Util::FixedArray<CoreGraphics::ResourceTableId>                 virtualTerrainDynamicResourceTable;

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

    CoreGraphics::PassId                                            tileUpdatePass;
    CoreGraphics::PassId                                            tileFallbackPass;

    Util::Array<Terrain::TerrainTileUpdateUniforms>                 pageUniforms;
    Util::Array<Math::uint2>                                        tileOffsets;
    Util::Array<PhysicalTileUpdate>                                 pageUpdatesThisFrame;

    Util::Array<CoreGraphics::BufferCopy>                           indirectionBufferUpdatesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureUpdatesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureFromCopiesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureToCopiesThisFrame;
    Util::Array<CoreGraphics::BufferCopy>                           indirectionBufferClearsThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureClearsThisFrame;
    uint numPixels;

} terrainVirtualTileState;

struct TerrainVert
{
    Math::float3 position;
    Math::float2 uv;
    Math::float2 tileUv;
};

struct TerrainTri
{
    IndexT a, b, c;
};

struct TerrainQuad
{
    IndexT a, b, c, d;
};

enum BiomeLoadBits
{
    AlbedoLoaded = 0x1
    , NormalLoaded = 0x2
    , MaterialLoaded = 0x4
    , MaskLoaded = 0x8
    , WeightsLoaded = 0x10
};

//------------------------------------------------------------------------------
/**
*/
void
PackSubTexture(const SubTexture& subTex, TerrainSubTexture& compressed)
{
    // First 12 bits are offset in x,
    // next 12 bits is offset in y,
    // then 4 bits for mips
    // lastly 4 bits for mip bias
    compressed.packed0 = (subTex.indirectionOffset.x & 0xFFF)
        | ((subTex.indirectionOffset.y & 0xFFF) << 12)
        | ((subTex.maxMip & 0xF) << 24)
        | ((subTex.mipBias & 0xF) << 28)
        ;
}

//------------------------------------------------------------------------------
/**
*/
TerrainContext::TerrainContext()
{
}

//------------------------------------------------------------------------------
/**
*/
TerrainContext::~TerrainContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::Create(const TerrainSetupSettings& settings)
{
    __CreateContext();
    using namespace CoreGraphics;

    Core::CVarCreate(Core::CVarType::CVar_Int, "r_terrain_debug", "0", "Show debug interface for the terrain system [0,1]");

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    terrainState.settings = settings;
    terrainState.updateShadowMap = true;
    terrainState.cachedSunDirection = Math::vec4(0);

    // create vertex buffer
    Util::Array<VertexComponent> vertexComponents = 
    {
        VertexComponent{ 0, VertexComponent::Format::Float3 },
        VertexComponent{ 1, VertexComponent::Format::Float2 },
        VertexComponent{ 2, VertexComponent::Format::Float2 },
    };
    terrainState.vlo = CreateVertexLayout({ vertexComponents });

    terrainState.terrainShader = ShaderGet("shd:terrain/terrain.fxb");
    terrainState.resourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);
    terrainState.terrainShadowProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainShadows"));

    CoreGraphics::BufferCreateInfo sysBufInfo;
    sysBufInfo.name = "VirtualSystemBuffer"_atm;
    sysBufInfo.size = 1;
    sysBufInfo.elementSize = sizeof(Terrain::TerrainSystemUniforms);
    sysBufInfo.mode = CoreGraphics::DeviceAndHost;
    sysBufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    terrainState.systemConstants = CoreGraphics::CreateBuffer(sysBufInfo);

    CoreGraphics::BufferCreateInfo biomeBufferInfo;
    biomeBufferInfo.name = "BiomeBuffer"_atm;
    biomeBufferInfo.size = 1;
    biomeBufferInfo.elementSize = sizeof(Terrain::MaterialLayers);
    biomeBufferInfo.mode = CoreGraphics::DeviceAndHost;
    biomeBufferInfo.usageFlags = CoreGraphics::ConstantBuffer;
    terrainState.biomeBuffer = CoreGraphics::CreateBuffer(biomeBufferInfo);

    CoreGraphics::TextureCreateInfo shadowMapInfo;
    shadowMapInfo.name = "TerrainShadowMap"_atm;
    shadowMapInfo.width = TerrainShadowMapSize;
    shadowMapInfo.height = TerrainShadowMapSize;
    shadowMapInfo.format = CoreGraphics::PixelFormat::R16F;
    shadowMapInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
    terrainState.terrainShadowMap = CoreGraphics::CreateTexture(shadowMapInfo);

    Lighting::LightContext::SetupTerrainShadows(terrainState.terrainShadowMap, settings.worldSizeX);

    ResourceTableSetConstantBuffer(terrainState.resourceTable, { terrainState.biomeBuffer, Terrain::Table_System::MaterialLayers::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetConstantBuffer(terrainState.resourceTable, { terrainState.systemConstants, Terrain::Table_System::TerrainSystemUniforms::SLOT, 0, NEBULA_WHOLE_BUFFER_SIZE, 0});
    ResourceTableSetRWTexture(terrainState.resourceTable, { terrainState.terrainShadowMap, Terrain::Table_System::TerrainShadowMap_SLOT, 0, CoreGraphics::InvalidSamplerId });
    ResourceTableCommitChanges(terrainState.resourceTable);

    //------------------------------------------------------------------------------
    /**
        Adaptive terrain virtual texturing setup

        Passes:
        1. Clear page update buffer.
        2. Rasterize geometry and update page buffer.
        3. Read back on CPU and generate terrain tile jobs
        4. Splat terrain tiles into to physical texture cache.
        5. Render full-screen pass to put terrain pixels on the screen.

        Why don't we just render the terrain directly and sample our input textures? 
        Well, it would require a lot of work for the terrain splatting shader, 
        especially if we want to have features like roads and procedural decals. 
        This is called an optimization. LOL. 
    */
    //------------------------------------------------------------------------------

    terrainVirtualTileState.terrainPageClearUpdateBufferProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainPageClearUpdateBuffer"));
    terrainVirtualTileState.terrainPrepassProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainPrepass"));
    terrainVirtualTileState.terrainResolveProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainResolve"));
    terrainVirtualTileState.terrainTileUpdateProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainTileUpdate"));
    terrainVirtualTileState.terrainTileFallbackProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainLowresFallback"));

    terrainVirtualTileState.virtualTerrainSystemResourceTable = ShaderCreateResourceTableSet(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);
    terrainVirtualTileState.virtualTerrainRuntimeResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
    terrainVirtualTileState.virtualTerrainDynamicResourceTable.Resize(CoreGraphics::GetNumBufferedFrames());
    for (auto& table : terrainVirtualTileState.virtualTerrainDynamicResourceTable)
        table = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "VirtualRuntimeBuffer"_atm;
    bufInfo.size = 1;
    bufInfo.elementSize = sizeof(Terrain::TerrainRuntimeUniforms);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    terrainVirtualTileState.runtimeConstants = CoreGraphics::CreateBuffer(bufInfo);

    uint subTextureCount = (terrainState.settings.worldSizeX / SubTextureWorldSize) * (terrainState.settings.worldSizeZ / SubTextureWorldSize);

    bufInfo.name = "SubTextureBuffer"_atm;
    bufInfo.size = subTextureCount;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTexture);
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    terrainVirtualTileState.subTextureBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "SubTextureStagingBuffer"_atm;
    bufInfo.size = subTextureCount;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTexture);
    bufInfo.mode = BufferAccessMode::HostLocal;
    bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
    terrainVirtualTileState.subtextureStagingBuffers = std::move(BufferSet(bufInfo));

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "IndirectionTexture"_atm;
    texInfo.width = IndirectionTextureSize;
    texInfo.height = IndirectionTextureSize;
    texInfo.format = CoreGraphics::PixelFormat::R32;
    texInfo.usage = CoreGraphics::SampleTexture | CoreGraphics::TransferTextureDestination | CoreGraphics::TransferTextureSource;
    texInfo.mips = IndirectionNumMips;
    texInfo.clear = true;
    texInfo.clearColorU4 = Math::uint4{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    terrainVirtualTileState.indirectionTexture = CoreGraphics::CreateTexture(texInfo);

    texInfo.name = "IndirectionTextureCopy"_atm;
    terrainVirtualTileState.indirectionTextureCopy = CoreGraphics::CreateTexture(texInfo);

    uint offset = 0;

    // setup offset and mip size variables
    terrainVirtualTileState.indirectionMipOffsets.Resize(IndirectionNumMips);
    terrainVirtualTileState.indirectionMipSizes.Resize(IndirectionNumMips);
    for (uint i = 0; i < IndirectionNumMips; i++)
    {
        uint width = IndirectionTextureSize >> i;
        uint height = IndirectionTextureSize >> i;

        terrainVirtualTileState.indirectionMipSizes[i] = width;
        terrainVirtualTileState.indirectionMipOffsets[i] = offset;
        offset += width * height;
    }

    terrainVirtualTileState.indirectionBuffer.Resize(offset);
    terrainVirtualTileState.indirectionBuffer.Fill(IndirectionEntry{ 0xFFFFFFFF });

    bufInfo.name = "TerrainUploadBuffer"_atm;
    bufInfo.elementSize = sizeof(IndirectionEntry);
    bufInfo.size = IndirectionTextureSize * IndirectionTextureSize;
    bufInfo.mode = BufferAccessMode::HostLocal;
    bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    terrainVirtualTileState.indirectionUploadBuffers = bufInfo;
    terrainVirtualTileState.indirectionUploadOffsets.Resize(CoreGraphics::GetNumBufferedFrames());
    terrainVirtualTileState.indirectionUploadOffsets.Fill(0x0);

    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer();

    // clear the buffer, the subsequent fills are to clear the buffers

    bufInfo.name = "PageStatusBuffer"_atm;
    bufInfo.elementSize = sizeof(uint);
    bufInfo.size = offset;
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    terrainVirtualTileState.pageStatusBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "PageUpdateListBuffer"_atm;
    bufInfo.elementSize = sizeof(Terrain::PageUpdateList);
    bufInfo.size = 1;
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    terrainVirtualTileState.pageUpdateListBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "PageUpdateReadbackBuffer"_atm;
    bufInfo.elementSize = sizeof(Terrain::PageUpdateList);
    bufInfo.size = 1;
    bufInfo.usageFlags = CoreGraphics::TransferBufferDestination;
    bufInfo.mode = BufferAccessMode::HostCached;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;

    terrainVirtualTileState.pageUpdateReadbackBuffers = std::move(BufferSet(bufInfo));
    for (IndexT i = 0; i < terrainVirtualTileState.pageUpdateReadbackBuffers.buffers.Size(); i++)
    {
        CoreGraphics::BufferFill(cmdBuf, terrainVirtualTileState.pageUpdateReadbackBuffers.buffers[i], 0x0);
    }

    // we're done
    CoreGraphics::UnlockGraphicsSetupCommandBuffer();

    CoreGraphics::TextureCreateInfo albedoCacheInfo;
    albedoCacheInfo.name = "AlbedoPhysicalCache"_atm;
    albedoCacheInfo.width = PhysicalTexturePaddedSize;
    albedoCacheInfo.height = PhysicalTexturePaddedSize;
    albedoCacheInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    albedoCacheInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.physicalAlbedoCache = CoreGraphics::CreateTexture(albedoCacheInfo);

    CoreGraphics::TextureCreateInfo normalCacheInfo;
    normalCacheInfo.name = "NormalPhysicalCache"_atm;
    normalCacheInfo.width = PhysicalTexturePaddedSize;
    normalCacheInfo.height = PhysicalTexturePaddedSize;
    normalCacheInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    normalCacheInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.physicalNormalCache = CoreGraphics::CreateTexture(normalCacheInfo);

    CoreGraphics::TextureCreateInfo materialCacheInfo;
    materialCacheInfo.name = "MaterialPhysicalCache"_atm;
    materialCacheInfo.width = PhysicalTexturePaddedSize;
    materialCacheInfo.height = PhysicalTexturePaddedSize;
    materialCacheInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    materialCacheInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.physicalMaterialCache = CoreGraphics::CreateTexture(materialCacheInfo);

    CoreGraphics::TextureCreateInfo lowResAlbedoInfo;
    lowResAlbedoInfo.name = "TerrainAlbedoLowres"_atm;
    lowResAlbedoInfo.mips = LowresFallbackMips;
    lowResAlbedoInfo.width = LowresFallbackSize;
    lowResAlbedoInfo.height = LowresFallbackSize;
    lowResAlbedoInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResAlbedoInfo.usage = CoreGraphics::TextureUsage::RenderTexture | CoreGraphics::TextureUsage::TransferTextureDestination | CoreGraphics::TextureUsage::TransferTextureSource;
    terrainVirtualTileState.lowresAlbedo = CoreGraphics::CreateTexture(lowResAlbedoInfo);

    CoreGraphics::TextureCreateInfo lowResNormalInfo;
    lowResNormalInfo.name = "TerrainNormalLowres"_atm;
    lowResNormalInfo.mips = LowresFallbackMips;
    lowResNormalInfo.width = LowresFallbackSize;
    lowResNormalInfo.height = LowresFallbackSize;
    lowResNormalInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResNormalInfo.usage = CoreGraphics::TextureUsage::RenderTexture | CoreGraphics::TextureUsage::TransferTextureDestination | CoreGraphics::TextureUsage::TransferTextureSource;
    terrainVirtualTileState.lowresNormal = CoreGraphics::CreateTexture(lowResNormalInfo);

    CoreGraphics::TextureCreateInfo lowResMaterialInfo;
    lowResMaterialInfo.name = "TerrainMaterialLowres"_atm;
    lowResMaterialInfo.mips = LowresFallbackMips;
    lowResMaterialInfo.width = LowresFallbackSize;
    lowResMaterialInfo.height = LowresFallbackSize;
    lowResMaterialInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResMaterialInfo.usage = CoreGraphics::TextureUsage::RenderTexture | CoreGraphics::TextureUsage::TransferTextureDestination | CoreGraphics::TextureUsage::TransferTextureSource;
    terrainVirtualTileState.lowresMaterial = CoreGraphics::CreateTexture(lowResMaterialInfo);

    // setup virtual sub textures buffer
    terrainVirtualTileState.subTextures.Resize(subTextureCount);
    terrainVirtualTileState.gpuSubTextures.Resize(subTextureCount);

    uint subTextureCountX = terrainState.settings.worldSizeX / SubTextureWorldSize;
    uint subTextureCountZ = terrainState.settings.worldSizeZ / SubTextureWorldSize;
    for (uint z = 0; z < subTextureCountZ; z++)
    {
        for (uint x = 0; x < subTextureCountX; x++)
        {
            uint index = x + z * subTextureCountX;
            terrainVirtualTileState.subTextures[index].numTiles = 0;
            terrainVirtualTileState.subTextures[index].maxMip = 0;
            terrainVirtualTileState.subTextures[index].indirectionOffset.x = UINT32_MAX;
            terrainVirtualTileState.subTextures[index].indirectionOffset.y = UINT32_MAX;
            terrainVirtualTileState.subTextures[index].mipBias = 0;

            // position is calculated as the center of each cell, offset by half of the world size (so we are oriented around 0)
            float xPos = x * SubTextureWorldSize - terrainState.settings.worldSizeX / 2.0f;
            float zPos = z * SubTextureWorldSize - terrainState.settings.worldSizeZ / 2.0f;
            terrainVirtualTileState.subTextures[index].worldCoordinate.x = xPos;
            terrainVirtualTileState.subTextures[index].worldCoordinate.y = zPos;
            terrainVirtualTileState.gpuSubTextures[index].worldCoordinate[0] = xPos;
            terrainVirtualTileState.gpuSubTextures[index].worldCoordinate[1] = zPos;
            PackSubTexture(terrainVirtualTileState.subTextures[index], terrainVirtualTileState.gpuSubTextures[index]);
        }
    }

    // setup indirection occupancy, the indirection texture is 2048, and the maximum allocation size is 256 indirection pixels
    terrainVirtualTileState.indirectionOccupancy.Setup(IndirectionTextureSize, 256, 1);

    // setup the physical texture occupancy, the texture is 8192x8192 pixels, and the page size is 256, so effectively make this a quad tree that ends at individual pages
    //terrainVirtualTileState.physicalTextureTileOccupancy.Setup(PhysicalTexturePaddedSize, PhysicalTexturePaddedSize, PhysicalTextureTilePaddedSize);
    terrainVirtualTileState.physicalTextureTileCache.Setup(PhysicalTextureTilePaddedSize, PhysicalTexturePaddedSize);

    terrainVirtualTileState.virtualTerrainSystemResourceTable.ForEach([](const ResourceTableId table, const IndexT i)
    {
        ResourceTableSetConstantBuffer(table,
                                    {
                                        terrainState.biomeBuffer,
                                        Terrain::Table_System::MaterialLayers::SLOT,
                                        NEBULA_WHOLE_BUFFER_SIZE,
                                    }
        );

        ResourceTableSetConstantBuffer(table,
            {
                terrainState.systemConstants,
                Terrain::Table_System::TerrainSystemUniforms::SLOT,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                terrainVirtualTileState.subTextureBuffer,
                Terrain::Table_System::TerrainSubTexturesBuffer::SLOT,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                terrainVirtualTileState.pageUpdateListBuffer,
                Terrain::Table_System::PageUpdateListBuffer::SLOT,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                terrainVirtualTileState.pageStatusBuffer,
                Terrain::Table_System::PageStatusBuffer::SLOT,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableCommitChanges(table);
    });
    

    ResourceTableSetConstantBuffer(terrainVirtualTileState.virtualTerrainRuntimeResourceTable,
        {
            terrainVirtualTileState.runtimeConstants,
            Terrain::Table_Batch::TerrainRuntimeUniforms::SLOT,
            Terrain::Table_Batch::TerrainRuntimeUniforms::SIZE,
        });
    ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainRuntimeResourceTable);

    IndexT bufferIndex = 0;
    for (const auto table : terrainVirtualTileState.virtualTerrainDynamicResourceTable)
    {
        ResourceTableSetConstantBuffer(table,
        {
            CoreGraphics::GetGraphicsConstantBuffer(bufferIndex++),
            Terrain::Table_DynamicOffset::TerrainTileUpdateUniforms::SLOT,
            0,
            Terrain::Table_DynamicOffset::TerrainTileUpdateUniforms::SIZE,
            0,
            false, true
        });
        ResourceTableCommitChanges(table);
    }

    // create pass for updating the physical cache tiles
    CoreGraphics::PassCreateInfo tileUpdatePassCreate;
    tileUpdatePassCreate.name = "TerrainVirtualTileUpdate";
    tileUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Albedo Cache View", terrainVirtualTileState.physicalAlbedoCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalAlbedoCache) }));
    tileUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Normal Cache View", terrainVirtualTileState.physicalNormalCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalNormalCache) }));
    tileUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Material Cache View", terrainVirtualTileState.physicalMaterialCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalMaterialCache) }));
    AttachmentFlagBits bits = AttachmentFlagBits::Load | AttachmentFlagBits::Store;
    tileUpdatePassCreate.attachmentFlags.Append(bits, bits, bits);
    tileUpdatePassCreate.attachmentDepthStencil.Append(false, false, false);
    tileUpdatePassCreate.attachmentClears.Append(Math::vec4(0,0,0,0), Math::vec4(0,0,0,0), Math::vec4(0,0,0,0));
    
    CoreGraphics::Subpass subpass;
    subpass.attachments.Append(0, 1, 2);
    subpass.numScissors = 3;
    subpass.numViewports = 3;

    tileUpdatePassCreate.subpasses.Append(subpass);
    terrainVirtualTileState.tileUpdatePass = CoreGraphics::CreatePass(tileUpdatePassCreate);

    // create pass for updating the fallback textures
    CoreGraphics::PassCreateInfo lowresUpdatePassCreate;
    lowresUpdatePassCreate.name = "TerrainVirtualLowresUpdate";
    lowresUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Lowres Albedo Cache View", terrainVirtualTileState.lowresAlbedo, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresAlbedo) }));
    lowresUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Lowres Normal Cache View", terrainVirtualTileState.lowresNormal, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresNormal) }));
    lowresUpdatePassCreate.attachments.Append(CreateTextureView({ "Terrain Lowres Material Cache View", terrainVirtualTileState.lowresMaterial, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresMaterial) }));
    bits = AttachmentFlagBits::Store | AttachmentFlagBits::Clear;
    lowresUpdatePassCreate.attachmentFlags.Append(bits, bits, bits);
    lowresUpdatePassCreate.attachmentDepthStencil.Append(false, false, false);
    lowresUpdatePassCreate.attachmentClears.Append(Math::vec4(0,0,0,0), Math::vec4(0,0,0,0), Math::vec4(0,0,0,0));

    lowresUpdatePassCreate.subpasses.Append(subpass);
    terrainVirtualTileState.tileFallbackPass = CoreGraphics::CreatePass(lowresUpdatePassCreate);

    Frame::FrameCode* terrainPrepare = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    terrainPrepare->domain = CoreGraphics::BarrierDomain::Global;
    terrainPrepare->bufferDeps.Add(terrainVirtualTileState.pageStatusBuffer,
                                {
                                    "Page Status Buffer",
                                    CoreGraphics::PipelineStage::TransferWrite,
                                    CoreGraphics::BufferSubresourceInfo()
                                });
    terrainPrepare->bufferDeps.Add(terrainVirtualTileState.subTextureBuffer,
                                {
                                    "Sub Texture Buffer",
                                    CoreGraphics::PipelineStage::TransferWrite,
                                    CoreGraphics::BufferSubresourceInfo()
                                });
    terrainPrepare->textureDeps.Add(terrainVirtualTileState.indirectionTexture,
                                {
                                    "Indirection Texture",
                                    CoreGraphics::PipelineStage::TransferWrite,
                                    CoreGraphics::TextureSubresourceInfo::Color(terrainVirtualTileState.indirectionTexture)
                                });
    terrainPrepare->textureDeps.Add(terrainVirtualTileState.indirectionTextureCopy,
                                {
                                    "Indirection Texture Copy",
                                    CoreGraphics::PipelineStage::TransferRead,
                                    CoreGraphics::TextureSubresourceInfo::Color(terrainVirtualTileState.indirectionTextureCopy)
                                });
    terrainPrepare->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Size() > 0
            || terrainVirtualTileState.indirectionBufferClearsThisFrame.Size() > 0
            || terrainVirtualTileState.indirectionTextureFromCopiesThisFrame.Size() > 0)
        {
            CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Shuffle Indirection Regions");

            // Perform clears
            if (terrainVirtualTileState.indirectionBufferClearsThisFrame.Size() > 0)
            {
                CmdBarrier(cmdBuf,
                    PipelineStage::HostWrite,
                    PipelineStage::TransferRead,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrierInfo
                        {
                            terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex],
                            CoreGraphics::BufferSubresourceInfo()
                        },

                    });

                // perform the moves and clears of indirection pixels
                CmdCopy(cmdBuf,
                    terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex], terrainVirtualTileState.indirectionBufferClearsThisFrame,
                    terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureClearsThisFrame);

                CmdBarrier(cmdBuf,
                    PipelineStage::TransferRead,
                    PipelineStage::HostWrite,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrierInfo
                        {
                            terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex],
                            CoreGraphics::BufferSubresourceInfo()
                        },
                    });
            }

            // Perform mip shifts
            if (terrainVirtualTileState.indirectionTextureFromCopiesThisFrame.Size() > 0)
            {
                CmdCopy(cmdBuf,
                    terrainVirtualTileState.indirectionTextureCopy, terrainVirtualTileState.indirectionTextureFromCopiesThisFrame,
                    terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureToCopiesThisFrame);
            }

            // Perform CPU side updates
            if (terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Size() > 0)
            {
                CmdBarrier(cmdBuf,
                    PipelineStage::HostWrite,
                    PipelineStage::TransferRead,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrierInfo
                        {
                            terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex],
                            CoreGraphics::BufferSubresourceInfo()
                        },

                    });

                // update the new pixels
                CmdCopy(cmdBuf,
                    terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex], terrainVirtualTileState.indirectionBufferUpdatesThisFrame,
                    terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureUpdatesThisFrame);

                CmdBarrier(cmdBuf,
                    PipelineStage::TransferRead,
                    PipelineStage::HostWrite,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrierInfo
                        {
                            terrainVirtualTileState.indirectionUploadBuffers.buffers[bufferIndex],
                            CoreGraphics::BufferSubresourceInfo()
                        },
                    });
            }

            // Clear pending updates
            terrainVirtualTileState.indirectionTextureFromCopiesThisFrame.Clear();
            terrainVirtualTileState.indirectionTextureToCopiesThisFrame.Clear();

            terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Clear();
            terrainVirtualTileState.indirectionTextureUpdatesThisFrame.Clear();

            terrainVirtualTileState.indirectionBufferClearsThisFrame.Clear();
            terrainVirtualTileState.indirectionTextureClearsThisFrame.Clear();
            CmdEndMarker(cmdBuf);
        }

        // If we have subtexture updates, make sure to copy them too
        if (terrainVirtualTileState.virtualSubtextureBufferUpdate)
        {
            CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Copy SubTextures from Host Buffer");
            CmdBarrier(cmdBuf,
                PipelineStage::HostWrite,
                PipelineStage::TransferRead,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrierInfo
                    {
                        terrainVirtualTileState.subtextureStagingBuffers.buffers[bufferIndex],
                        CoreGraphics::BufferSubresourceInfo()
                    },
                });

            BufferCopy from, to;
            from.offset = 0;
            to.offset = 0;
            CmdCopy(cmdBuf
                , terrainVirtualTileState.subtextureStagingBuffers.buffers[bufferIndex], { from }
                , terrainVirtualTileState.subTextureBuffer, { to }
                , BufferGetByteSize(terrainVirtualTileState.subTextureBuffer));

            CmdBarrier(cmdBuf,
                PipelineStage::TransferRead,
                PipelineStage::HostWrite,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrierInfo
                    {
                        terrainVirtualTileState.subtextureStagingBuffers.buffers[bufferIndex],
                        CoreGraphics::BufferSubresourceInfo()
                    },
                });

            terrainVirtualTileState.virtualSubtextureBufferUpdate = false;
            CmdEndMarker(cmdBuf);
        }
    };

    Frame::FrameCode* indirectionCopy = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    indirectionCopy->domain = CoreGraphics::BarrierDomain::Global;
    indirectionCopy->textureDeps.Add(terrainVirtualTileState.indirectionTexture,
                            {
                                "Indirection Texture",
                                CoreGraphics::PipelineStage::TransferRead,
                                CoreGraphics::TextureSubresourceInfo::Color(terrainVirtualTileState.indirectionTexture)
                            });
    indirectionCopy->textureDeps.Add(terrainVirtualTileState.indirectionTextureCopy,
                            {
                                "Indirection Texture Copy",
                                CoreGraphics::PipelineStage::TransferWrite,
                                CoreGraphics::TextureSubresourceInfo::Color(terrainVirtualTileState.indirectionTextureCopy)
                            });
    indirectionCopy->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Copy Indirection Mips");

        // Copy this frame of indirection to copy
        TextureDimensions dims = TextureGetDimensions(terrainVirtualTileState.indirectionTexture);
        for (IndexT mip = 0; mip < TextureGetNumMips(terrainVirtualTileState.indirectionTexture); mip++)
        {
            TextureCopy cp;
            cp.mip = mip;
            cp.layer = 0;
            cp.region = Math::rectangle(0, 0, dims.width >> mip, dims.height >> mip);
            CmdCopy(cmdBuf,
                    terrainVirtualTileState.indirectionTexture, { cp }, terrainVirtualTileState.indirectionTextureCopy, { cp });
        }

        CmdEndMarker(cmdBuf);
    };

    Frame::AddSubgraph("Terrain Prepare", { terrainPrepare, indirectionCopy });

    Frame::FrameCode* terrainPrepass = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    terrainPrepass->domain = CoreGraphics::BarrierDomain::Pass;
    terrainPrepass->bufferDeps.Add(terrainVirtualTileState.pageUpdateListBuffer,
                    {
                        "Page Update List",
                        CoreGraphics::PipelineStage::PixelShaderWrite,
                        CoreGraphics::BufferSubresourceInfo()
                    });
    terrainPrepass->bufferDeps.Add(terrainVirtualTileState.subTextureBuffer,
                    {
                        "Sub Texture Buffer",
                        CoreGraphics::PipelineStage::PixelShaderRead,
                        CoreGraphics::BufferSubresourceInfo()
                    });
    terrainPrepass->buildFunc = [](const CoreGraphics::PassId pass, uint subpass)
    {
        terrainVirtualTileState.terrainPrepassPipeline = CreatePipeline(
            {
                .shader = terrainVirtualTileState.terrainPrepassProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = InputAssemblyKey{ { .topo = CoreGraphics::PrimitiveTopology::PatchList, .primRestart = false } }
            });
    };
    terrainPrepass->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (terrainState.renderToggle == false)
            return;

        // setup shader state, set shader before we set the vertex layout
        CmdSetGraphicsPipeline(cmdBuf, terrainVirtualTileState.terrainPrepassPipeline);
        CmdSetVertexLayout(cmdBuf, terrainState.vlo);
        CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::PatchList);

        // set shared resources
        CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainSystemResourceTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

        // Draw terrains
        Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

        SizeT numQuadsX = terrainState.settings.quadsPerTileX;
        SizeT numQuadsY = terrainState.settings.quadsPerTileY;
        SizeT numVertsX = numQuadsX + 1;
        SizeT numVertsY = numQuadsY + 1;
        SizeT numTris = numVertsX * numVertsY;

        CoreGraphics::PrimitiveGroup group;
        group.SetBaseIndex(0);
        group.SetBaseVertex(0);
        group.SetNumIndices(numTris * 4);
        group.SetNumVertices(0);

        // go through and render terrain instances
        for (IndexT i = 0; i < runtimes.Size(); i++)
        {
            TerrainRuntimeInfo& rt = runtimes[i];
            CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            CmdSetVertexBuffer(cmdBuf, 0, rt.vbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index32, rt.ibo, 0);
            CmdDraw(cmdBuf, terrainVirtualTileState.numPatchesThisFrame, group);
        }
    };
    Frame::AddSubgraph("Terrain Prepass", { terrainPrepass });

    Frame::FrameCode* terrainShadows = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    terrainShadows->domain = CoreGraphics::BarrierDomain::Global;
    terrainShadows->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (!terrainState.renderToggle)
            return;

        if (!terrainState.updateShadowMap)
            return;

        terrainState.updateShadowMap = false;

        // Setup shader state, set shader before we set the vertex layout
        CmdSetShaderProgram(cmdBuf, terrainState.terrainShadowProgram);

        // Set shared resources
        CmdSetResourceTable(cmdBuf, terrainState.resourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
        CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

        // Dispatch
        uint x = Math::divandroundup(TerrainShadowMapSize, 64);
        CmdDispatch(cmdBuf, x, TerrainShadowMapSize, 1);
    };
    Frame::AddSubgraph("Sun Terrain Shadows", { terrainShadows });

    Frame::FrameCode* pageExtract = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    pageExtract->domain = CoreGraphics::BarrierDomain::Global;
    pageExtract->bufferDeps.Add(terrainVirtualTileState.pageUpdateListBuffer,
                {
                    "Page Update List",
                    CoreGraphics::PipelineStage::TransferRead,
                    CoreGraphics::BufferSubresourceInfo()
                });
    pageExtract->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Copy Pages to Readback");
        CoreGraphics::BufferCopy from, to;
        from.offset = 0;
        to.offset = 0;
        CmdCopy(
            cmdBuf,
            terrainVirtualTileState.pageUpdateListBuffer, { from }, terrainVirtualTileState.pageUpdateReadbackBuffers.buffers[bufferIndex], { to },
            sizeof(Terrain::PageUpdateList)
        );
        CmdEndMarker(cmdBuf);
    };

    Frame::FrameCode* pageClear = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    pageClear->domain = CoreGraphics::BarrierDomain::Global;
    pageClear->bufferDeps.Add(terrainVirtualTileState.pageUpdateListBuffer,
                        {
                            "Page Update List",
                            CoreGraphics::PipelineStage::ComputeShaderWrite,
                            CoreGraphics::BufferSubresourceInfo()
                        });
    pageClear->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_COMPUTE, "Clear Page Status Buffer");

        CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainPageClearUpdateBufferProgram);
        CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainSystemResourceTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

        // run a single compute shader to clear the number of page entries
        uint numDispatches = Math::divandroundup(Terrain::MAX_PAGE_UPDATES, 64);
        CmdDispatch(cmdBuf, numDispatches, 1, 1);

        CmdEndMarker(cmdBuf);
    };

    Frame::FrameCode* cacheUpdate = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    cacheUpdate->domain = CoreGraphics::BarrierDomain::Global;
    cacheUpdate->textureDeps.Add(terrainVirtualTileState.physicalAlbedoCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::ColorWrite,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalAlbedoCache)
                            });
    cacheUpdate->textureDeps.Add(terrainVirtualTileState.physicalMaterialCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::ColorWrite,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalMaterialCache)
                            });
    cacheUpdate->textureDeps.Add(terrainVirtualTileState.physicalNormalCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::ColorWrite,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalNormalCache)
                            });
    cacheUpdate->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        if (terrainVirtualTileState.updateLowres)
        {
            terrainVirtualTileState.updateLowres = false;

            CmdBeginMarker(cmdBuf, NEBULA_MARKER_GRAPHICS, "Update Lowres caches");

            CmdBeginPass(cmdBuf, terrainVirtualTileState.tileFallbackPass);

            // Setup state for update
            CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainTileFallbackProgram);
            CmdSetResourceTable(cmdBuf, PassGetResourceTable(terrainVirtualTileState.tileFallbackPass), NEBULA_PASS_GROUP, GraphicsPipeline, nullptr);
            CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainSystemResourceTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
            CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            Math::rectangle<int> rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = TextureGetDimensions(terrainVirtualTileState.lowresAlbedo).width;
            rect.bottom = TextureGetDimensions(terrainVirtualTileState.lowresAlbedo).height;
            CmdSetViewport(cmdBuf, rect, 0);
            CmdSetViewport(cmdBuf, rect, 1);
            CmdSetViewport(cmdBuf, rect, 2);
            CmdSetScissorRect(cmdBuf, rect, 0);
            CmdSetScissorRect(cmdBuf, rect, 1);
            CmdSetScissorRect(cmdBuf, rect, 2);

            // Update textures
            RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
            CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());

            CmdEndPass(cmdBuf);

            // Transition rendered to mips to be shader read
            CoreGraphics::CmdBarrier(
                cmdBuf,
                CoreGraphics::PipelineStage::ColorWrite,
                CoreGraphics::PipelineStage::AllShadersRead,
                CoreGraphics::BarrierDomain::Global,
                {
                    {
                        terrainVirtualTileState.lowresAlbedo,
                        CoreGraphics::TextureSubresourceInfo(),
                    },
                    {
                        terrainVirtualTileState.lowresMaterial,
                        CoreGraphics::TextureSubresourceInfo(),
                    },
                    {
                        terrainVirtualTileState.lowresNormal,
                        CoreGraphics::TextureSubresourceInfo(),
                    }
                },
                nullptr);

            // Now generate mipmaps
            CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainVirtualTileState.lowresAlbedo);
            CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainVirtualTileState.lowresMaterial);
            CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainVirtualTileState.lowresNormal);

            CmdEndMarker(cmdBuf);
        }

        if (terrainVirtualTileState.pageUpdatesThisFrame.Size() > 0)
        {
            CmdBeginMarker(cmdBuf, NEBULA_MARKER_GRAPHICS, "Update Texture Caches");

            // start the pass
            CmdBeginPass(cmdBuf, terrainVirtualTileState.tileUpdatePass);

            CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainTileUpdateProgram);

            // apply the mesh for all quads
            RenderUtil::DrawFullScreenQuad::ApplyMesh(cmdBuf);
            CmdSetResourceTable(cmdBuf, PassGetResourceTable(terrainVirtualTileState.tileUpdatePass), NEBULA_PASS_GROUP, GraphicsPipeline, nullptr);
            CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainSystemResourceTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

            // go through pending page updates and render into the physical texture caches
            for (IndexT i = 0; i < runtimes.Size(); i++)
            {
                TerrainRuntimeInfo& rt = runtimes[i];
                CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                for (int j = 0; j < terrainVirtualTileState.pageUpdatesThisFrame.Size(); j++)
                {
                    PhysicalTileUpdate& pageUpdate = terrainVirtualTileState.pageUpdatesThisFrame[j];
                    CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainDynamicResourceTable[bufferIndex], NEBULA_DYNAMIC_OFFSET_GROUP, GraphicsPipeline, 1, pageUpdate.constantBufferOffsets);

                    // update viewport rectangle
                    Math::rectangle<int> rect;
                    rect.left = pageUpdate.tileOffset[0];
                    rect.top = pageUpdate.tileOffset[1];
                    rect.right = rect.left + PhysicalTextureTilePaddedSize;
                    rect.bottom = rect.top + PhysicalTextureTilePaddedSize;
                    CmdSetViewport(cmdBuf, rect, 0);
                    CmdSetViewport(cmdBuf, rect, 1);
                    CmdSetViewport(cmdBuf, rect, 2);
                    CmdSetScissorRect(cmdBuf, rect, 0);
                    CmdSetScissorRect(cmdBuf, rect, 1);
                    CmdSetScissorRect(cmdBuf, rect, 2);

                    // draw tile
                    CmdDraw(cmdBuf, RenderUtil::DrawFullScreenQuad::GetPrimitiveGroup());
                }
                terrainVirtualTileState.pageUpdatesThisFrame.Clear();
            }

            CmdEndPass(cmdBuf);

            CmdEndMarker(cmdBuf);
        }
    };
    Frame::AddSubgraph("Terrain Update Caches", { pageExtract, cacheUpdate, pageClear });

    Frame::FrameCode* resolve = terrainState.frameOpAllocator.Alloc<Frame::FrameCode>();
    resolve->domain = CoreGraphics::BarrierDomain::Pass;
    resolve->textureDeps.Add(terrainVirtualTileState.physicalAlbedoCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::PixelShaderRead,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalAlbedoCache)
                            });
    resolve->textureDeps.Add(terrainVirtualTileState.physicalMaterialCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::PixelShaderRead,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalMaterialCache)
                            });
    resolve->textureDeps.Add(terrainVirtualTileState.physicalNormalCache,
                            {
                                "Terrain Albedo Cache",
                                CoreGraphics::PipelineStage::PixelShaderRead,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.physicalNormalCache)
                            });
    resolve->textureDeps.Add(terrainVirtualTileState.indirectionTexture,
                            {
                                "Indirection Texture",
                                CoreGraphics::PipelineStage::PixelShaderRead,
                                TextureSubresourceInfo::Color(terrainVirtualTileState.indirectionTexture)
                            });
    resolve->buildFunc = [](const CoreGraphics::PassId pass, uint subpass)
    {
        terrainVirtualTileState.terrainResolvePipeline = CoreGraphics::CreatePipeline(
            {
                .shader = terrainVirtualTileState.terrainResolveProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = InputAssemblyKey{ { .topo = CoreGraphics::PrimitiveTopology::PatchList, .primRestart = false } }
            });
    };
    resolve->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetGraphicsPipeline(cmdBuf, terrainVirtualTileState.terrainResolvePipeline);
        CmdSetVertexLayout(cmdBuf, terrainState.vlo);
        CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::PatchList);

        // set shared resources
        CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainSystemResourceTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

        if (terrainState.renderToggle == false)
            return;

        // Draw terrains
        Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

        SizeT numQuadsX = terrainState.settings.quadsPerTileX;
        SizeT numQuadsY = terrainState.settings.quadsPerTileY;
        SizeT numVertsX = numQuadsX + 1;
        SizeT numVertsY = numQuadsY + 1;
        SizeT numTris = numVertsX * numVertsY;

        CoreGraphics::PrimitiveGroup group;
        group.SetBaseIndex(0);
        group.SetBaseVertex(0);
        group.SetNumIndices(numTris * 4);
        group.SetNumVertices(0);

        // go through and render terrain instances
        for (IndexT i = 0; i < runtimes.Size(); i++)
        {
            TerrainRuntimeInfo& rt = runtimes[i];
            CmdSetResourceTable(cmdBuf, terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            CmdSetVertexBuffer(cmdBuf, 0, rt.vbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index32, rt.ibo, 0);
            CmdDraw(cmdBuf, terrainVirtualTileState.numPatchesThisFrame, group);
        }
    };
    Frame::AddSubgraph("Terrain Resolve", { resolve });

    // create vlo
    terrainState.layers = 1;
    terrainState.debugRender = false;
    terrainState.renderToggle = true;
    terrainState.biomeCounter = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::Discard()
{
    DestroyBuffer(terrainVirtualTileState.pageStatusBuffer);
    DestroyBuffer(terrainVirtualTileState.subTextureBuffer);
    terrainVirtualTileState.subtextureStagingBuffers.~BufferSet();
    DestroyBuffer(terrainVirtualTileState.pageUpdateListBuffer);
    terrainVirtualTileState.pageUpdateReadbackBuffers.~BufferSet();
    DestroyPass(terrainVirtualTileState.tileFallbackPass);
    DestroyPass(terrainVirtualTileState.tileUpdatePass);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetupTerrain(
    const Graphics::GraphicsEntityId entity,
    const Resources::ResourceName& heightMap, 
    const Resources::ResourceName& decisionMap)
{
    n_assert(terrainState.settings.worldSizeX > 0);
    n_assert(terrainState.settings.worldSizeZ > 0);
    using namespace CoreGraphics;
    const Graphics::ContextEntityId cid = GetContextId(entity);
    TerrainLoadInfo& loadInfo = terrainAllocator.Get<Terrain_LoadInfo>(cid.id);
    TerrainRuntimeInfo& runtimeInfo = terrainAllocator.Get<Terrain_RuntimeInfo>(cid.id);

	runtimeInfo.loadBits = 0x0;
    runtimeInfo.lowresGenerated = false;
    runtimeInfo.decisionMap = Resources::CreateResource(decisionMap, "terrain"_atm, [&runtimeInfo](Resources::ResourceId id)
	{
		runtimeInfo.decisionMap = id;
        runtimeInfo.lowresGenerated = false;
        runtimeInfo.loadBits |= TerrainRuntimeInfo::DecisionMapLoaded;
	}, nullptr, false, false);

    runtimeInfo.heightMap = Resources::CreateResource(heightMap, "terrain"_atm, [&runtimeInfo](Resources::ResourceId id)
	{
        runtimeInfo.heightMap = id;
        runtimeInfo.lowresGenerated = false;
		runtimeInfo.loadBits |= TerrainRuntimeInfo::HeightMapLoaded;
	}, nullptr, false, false);

    runtimeInfo.worldWidth = terrainState.settings.worldSizeX;
    runtimeInfo.worldHeight = terrainState.settings.worldSizeZ;
    runtimeInfo.maxHeight = terrainState.settings.maxHeight;
    runtimeInfo.minHeight = terrainState.settings.minHeight;

    // divide world dimensions into 
    runtimeInfo.numTilesX = terrainState.settings.worldSizeX / terrainState.settings.tileWidth;
    runtimeInfo.numTilesY = terrainState.settings.worldSizeZ / terrainState.settings.tileHeight;
    runtimeInfo.tileWidth = terrainState.settings.tileWidth;
    runtimeInfo.tileHeight = terrainState.settings.tileHeight;
    SizeT height = terrainState.settings.maxHeight - terrainState.settings.minHeight;
    SizeT numQuadsX = terrainState.settings.quadsPerTileX;
    SizeT numQuadsY = terrainState.settings.quadsPerTileY;
    SizeT numVertsX = numQuadsX + 1;
    SizeT numVertsY = numQuadsY + 1;
    SizeT vertDistanceX = terrainState.settings.tileWidth  / terrainState.settings.quadsPerTileX;
    SizeT vertDistanceY = terrainState.settings.tileHeight / terrainState.settings.quadsPerTileY;
    SizeT numBufferedFrame = CoreGraphics::GetNumBufferedFrames();

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "TerrainPerPatchData"_atm;
    bufInfo.size = runtimeInfo.numTilesX * runtimeInfo.numTilesY;
    bufInfo.elementSize = sizeof(Terrain::TerrainPatch);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
    terrainVirtualTileState.patchConstants = CoreGraphics::BufferSet(bufInfo);

    terrainVirtualTileState.virtualTerrainSystemResourceTable.ForEach([](const ResourceTableId table, IndexT i)
    {
        ResourceTableSetRWBuffer(
            table
            , ResourceTableBuffer(terrainVirtualTileState.patchConstants.buffers[i], Terrain::Table_System::TerrainPatchData::SLOT)
            );

        ResourceTableCommitChanges(table);
    });
    

    // allocate a tile vertex buffer
    Util::FixedArray<TerrainVert> verts(numVertsX * numVertsY);

    // allocate terrain index buffer, every fourth pixel will generate two triangles 
    SizeT numTris = numVertsX * numVertsY;
    Util::FixedArray<TerrainQuad> quads(numQuadsX * numQuadsY);
    Util::FixedArray<Terrain::TerrainPatch> patchData(runtimeInfo.numTilesY * runtimeInfo.numTilesX);

    // setup sections
    uint patchCounter = 0;
    for (uint y = 0; y < runtimeInfo.numTilesY; y++)
    {
        for (uint x = 0; x < runtimeInfo.numTilesX; x++)
        {
            Math::bbox box;
            box.set(
                Math::point(
                    x * terrainState.settings.tileWidth - terrainState.settings.worldSizeX / 2 + terrainState.settings.tileWidth / 2,
                    terrainState.settings.minHeight,
                    y * terrainState.settings.tileHeight - terrainState.settings.worldSizeZ / 2 + terrainState.settings.tileHeight / 2
                ),
                Math::vector(terrainState.settings.tileWidth / 2, height, terrainState.settings.tileHeight / 2));
            runtimeInfo.sectionBoxes.Append(box);
            runtimeInfo.sectorVisible.Append(true);
            runtimeInfo.sectorLod.Append(1000.0f);
            runtimeInfo.sectorUpdateTextureTile.Append(false);
            runtimeInfo.sectorUniformOffsets.Append(Util::FixedArray<uint>(2, 0));
            runtimeInfo.sectorTileOffsets.Append(Util::FixedArray<uint>(2, 0));
            runtimeInfo.sectorAllocatedTile.Append(Math::uint3{ UINT32_MAX, UINT32_MAX, UINT32_MAX });
            runtimeInfo.sectorTextureTileSize.Append(Math::uint2{ 0, 0 });
            runtimeInfo.sectorUv.Append({ x * terrainState.settings.tileWidth / terrainState.settings.worldSizeX, y * terrainState.settings.tileHeight / terrainState.settings.worldSizeZ });

            Terrain::TerrainPatch patch;
            patch.PosOffset[0] = box.pmin.x;
            patch.PosOffset[1] = box.pmin.z;
            patch.UvOffset[0] = runtimeInfo.sectorUv.Back().x;
            patch.UvOffset[1] = runtimeInfo.sectorUv.Back().y;
            patchData[patchCounter++] = patch;
        }
    }
    //CoreGraphics::BufferUpdate(terrainVirtualTileState.patchConstants, patchData.Begin(), patchData.ByteSize(), 0);

    // walk through and set up sections, oriented around origo, so half of the sections are in the negative
    uint quad = 0;
    for (IndexT y = 0; y < numQuadsY; y++)
    {
        for (IndexT x = 0; x < numQuadsX; x++)
        {
            struct Vertex
            {
                Math::vec4 position;
                Math::vec2 uv;
            };
            Vertex v1, v2, v3, v4;

            // set terrain vertices, uv should be a fraction of world size
            v1.position.set(x * vertDistanceX, 0, y * vertDistanceY, 1);
            v1.uv = Math::vec2(x * vertDistanceX / float(terrainState.settings.tileWidth), y * vertDistanceY / float(terrainState.settings.tileHeight));

            v2.position.set((x + 1) * vertDistanceX, 0, y * vertDistanceY, 1);
            v2.uv = Math::vec2((x + 1) * vertDistanceX / float(terrainState.settings.tileWidth), y * vertDistanceY / float(terrainState.settings.tileHeight));

            v3.position.set(x * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
            v3.uv = Math::vec2(x * vertDistanceX / float(terrainState.settings.tileWidth), (y + 1) * vertDistanceY / float(terrainState.settings.tileHeight));

            v4.position.set((x + 1) * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
            v4.uv = Math::vec2((x + 1) * vertDistanceX / float(terrainState.settings.tileWidth), (y + 1) * vertDistanceY / float(terrainState.settings.tileHeight));

            // calculate tile local index, and offsets
            IndexT locX = x;
            IndexT locY = y;
            IndexT nextX = x + 1;
            IndexT nextY = y + 1;

            // get buffer data so we can update it
            IndexT vidx1, vidx2, vidx3, vidx4;
            vidx1 = locX + locY * numVertsX;
            vidx2 = nextX + locY * numVertsX;
            vidx3 = locX + nextY * numVertsX;
            vidx4 = nextX + nextY * numVertsX;

            TerrainVert& vt1 = verts[vidx1];
            v1.position.storeu3(vt1.position.v);
            vt1.uv.x = v1.uv.x;
            vt1.uv.y = v1.uv.y;
            vt1.tileUv.x = x / (float)(numQuadsX);
            vt1.tileUv.y = y / (float)(numQuadsY);

            TerrainVert& vt2 = verts[vidx2];
            v2.position.storeu3(vt2.position.v);
            vt2.uv.x = v2.uv.x;
            vt2.uv.y = v2.uv.y;
            vt2.tileUv.x = x / (float)(numQuadsX + 1.0f / numQuadsX);
            vt2.tileUv.y = y / (float)(numQuadsY);

            TerrainVert& vt3 = verts[vidx3];
            v3.position.storeu3(vt3.position.v);
            vt3.uv.x = v3.uv.x;
            vt3.uv.y = v3.uv.y;
            vt3.tileUv.x = x / (float)(numQuadsX);
            vt3.tileUv.y = y / (float)(numQuadsY + 1.0f / numQuadsY);

            TerrainVert& vt4 = verts[vidx4];
            v4.position.storeu3(vt4.position.v);
            vt4.uv.x = v4.uv.x;
            vt4.uv.y = v4.uv.y;
            vt4.tileUv.x = x / (float)(numQuadsX + 1.0f / numQuadsX);
            vt4.tileUv.y = y / (float)(numQuadsY + 1.0f / numQuadsY);

            // setup triangle tris
            TerrainQuad& q1 = quads[quad++];
            q1.a = vidx1;
            q1.b = vidx2;
            q1.c = vidx3;
            q1.d = vidx4;
        }
    }

    // create vbo
    BufferCreateInfo vboInfo;
    vboInfo.name = "terrain_vbo"_atm;
    vboInfo.size = numVertsX * numVertsY;
    vboInfo.elementSize = CoreGraphics::VertexLayoutGetSize(terrainState.vlo);
    vboInfo.mode = CoreGraphics::DeviceLocal;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = verts.Begin();
    vboInfo.dataSize = verts.ByteSize();
    runtimeInfo.vbo = CreateBuffer(vboInfo);

    // create ibo
    BufferCreateInfo iboInfo;
    iboInfo.name = "terrain_ibo"_atm;
    iboInfo.size = numTris * 4;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
    iboInfo.mode = CoreGraphics::DeviceLocal;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = quads.Begin();
    iboInfo.dataSize = quads.ByteSize();
    runtimeInfo.ibo = CreateBuffer(iboInfo);
}

//------------------------------------------------------------------------------
/**
*/
TerrainBiomeId 
TerrainContext::CreateBiome(const BiomeSettings& settings)
{
    Ids::Id32 ret = terrainBiomeAllocator.Alloc();
    terrainBiomeAllocator.Set<TerrainBiome_Settings>(ret, settings);

    Util::Array<BiomeMaterial> mats =
    {
        settings.materials[BiomeSettings::BiomeMaterialLayer::Flat],
        settings.materials[BiomeSettings::BiomeMaterialLayer::Slope],
        settings.materials[BiomeSettings::BiomeMaterialLayer::Height],
        settings.materials[BiomeSettings::BiomeMaterialLayer::HeightSlope]
    };
    IndexT biomeIndex = terrainState.biomeCounter;
    for (int i = 0; i < mats.Size(); i++)
    {
        terrainState.biomeLoaded[i] = 0x0;
        terrainState.biomeLowresGenerated[i] = false;
        Resources::CreateResource(mats[i].albedo.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            terrainState.biomeMaterials.MaterialAlbedos[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            Threading::Interlocked::Or(&terrainState.biomeLoaded[biomeIndex], BiomeLoadBits::AlbedoLoaded);
        }, nullptr, false, false);

        Resources::CreateResource(mats[i].normal.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            terrainState.biomeMaterials.MaterialNormals[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            Threading::Interlocked::Or(&terrainState.biomeLoaded[biomeIndex], BiomeLoadBits::NormalLoaded);
        }, nullptr, false, false);

        Resources::CreateResource(mats[i].material.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            terrainState.biomeMaterials.MaterialPBRs[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            Threading::Interlocked::Or(&terrainState.biomeLoaded[biomeIndex], BiomeLoadBits::NormalLoaded);
        }, nullptr, false, false);
    }

    Resources::CreateResource(settings.biomeMask, "terrain", [biomeIndex](Resources::ResourceId id)
    {
        terrainState.biomeMaterials.MaterialMasks[biomeIndex / 4][biomeIndex % 4] = CoreGraphics::TextureGetBindlessHandle(id);
        terrainState.biomeTextures.Append(id);
        terrainState.biomeMasks[terrainState.biomeCounter] = id;
        CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
        terrainState.biomeLowresGenerated[biomeIndex] = false;
        Threading::Interlocked::Or(&terrainState.biomeLoaded[biomeIndex], BiomeLoadBits::MaskLoaded);
    }, nullptr, false, false);

    if (settings.biomeParameters.useMaterialWeights)
    {
        Resources::CreateResource(settings.biomeParameters.weights, "terrain", [biomeIndex](Resources::ResourceId id)
        {
            terrainState.biomeMaterials.MaterialWeights[biomeIndex / 4][biomeIndex % 4] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            terrainState.biomeWeights[terrainState.biomeCounter] = id;
            CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            terrainState.biomeLoaded[biomeIndex] |= BiomeLoadBits::WeightsLoaded;
        }, nullptr, false, false);
    }
    else
    {
        terrainState.biomeLoaded[biomeIndex] |= BiomeLoadBits::WeightsLoaded;
    }

    //CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
    //terrainVirtualTileState.updateLowres = true;

    terrainBiomeAllocator.Set<TerrainBiome_Index>(ret, terrainState.biomeCounter);
    terrainState.biomeCounter++;
    return TerrainBiomeId(ret);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetBiomeSlopeThreshold(TerrainBiomeId id, float threshold)
{
    terrainBiomeAllocator.Get<TerrainBiome_Settings>(id.id).biomeParameters.slopeThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetBiomeHeightThreshold(TerrainBiomeId id, float threshold)
{
    terrainBiomeAllocator.Get<TerrainBiome_Settings>(id.id).biomeParameters.heightThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetSun(const Graphics::GraphicsEntityId sun)
{
    terrainState.sun = sun;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::CullPatches(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(TerrainRunJobs, Terrain);
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(view->GetCamera()));
    terrainVirtualTileState.indirectionUploadOffsets[ctx.bufferIndex] = 0;

    n_assert(subtexturesDoneCounter == 0);
    subtexturesDoneCounter = 1;
    struct SubTextureUpdateCtx
    {
        SubTextureUpdateJobUniforms uniforms;
        Math::mat4 camera;
        Terrain::SubTexture* subtextures;
        Threading::AtomicCounter* numOutputs;
        Terrain::SubTextureUpdateJobOutput* outputs;
    };
    SubTextureUpdateCtx subtexCtx;
    subtexCtx.uniforms.maxMip = IndirectionNumMips - 1;
    subtexCtx.uniforms.physicalTileSize = PhysicalTextureTileSize;
    subtexCtx.uniforms.subTextureWorldSize = SubTextureWorldSize;

    subtexCtx.camera = cameraTransform;
    subtexCtx.subtextures = terrainVirtualTileState.subTextures.Begin();
    terrainVirtualTileState.subTextureNumOutputs = 0;
    subtexCtx.numOutputs = &terrainVirtualTileState.subTextureNumOutputs;
    subtexCtx.outputs = terrainVirtualTileState.subTextureJobOutputs;
    Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
    {
        N_SCOPE(TerrainSubTextureUpdateJob, Terrain);

        auto context = static_cast<SubTextureUpdateCtx*>(ctx);

        // Iterate over work group
        for (IndexT i = 0; i < groupSize; i++)
        {
            // Get item index
            IndexT index = i + invocationOffset;
            if (index >= totalJobs)
                return;

            const Terrain::SubTexture& subTexture = context->subtextures[index];

            // mask out y coordinate by multiplying result with, 1, 0 ,1
            Math::vec4 min = Math::vec4(subTexture.worldCoordinate.x, 0, subTexture.worldCoordinate.y, 0);
            Math::vec4 max = min + Math::vec4(context->uniforms.subTextureWorldSize, 0.0f, context->uniforms.subTextureWorldSize, 0.0f);
            Math::vec4 cameraXZ = context->camera.position * Math::vec4(1, 0, 1, 0);
            Math::vec4 nearestPoint = Math::minimize(Math::maximize(cameraXZ, min), max);
            float distance = length(nearestPoint - cameraXZ);

            // if we are outside the virtual area, just default the resolution to 0
            uint resolution = 0;
            uint lod = 0;
            uint t = 0;
            if (distance > SubTextureRange)
                goto skipResolution;

            // at every regular distance interval, increase t
            t = Math::max(1.0f, (distance / SubTextureSwapDistance));

            // calculate lod logarithmically, such that it goes geometrically slower to progress to higher lods
            lod = Math::min((uint)Math::log2(t), context->uniforms.maxMip);

            // calculate the resolution by offseting the max resolution with the lod
            resolution = SubTextureMaxPixels >> lod;

skipResolution:

            // calculate the amount of tiles, which is the final lodded resolution divided by the size of a tile
            // the max being maxResolution and the smallest being 1
            uint tiles = resolution / context->uniforms.physicalTileSize;

            // only care about subtextures with at least 4 tiles
            //tiles = tiles >= 4 ? tiles : 0;

            SubTextureUpdateState state = SubTextureUpdateState::NoChange;

            // set state
            if (tiles >= 1)
            {
                if (subTexture.numTiles == 0)
                    state = SubTextureUpdateState::Created;
                else if (tiles > subTexture.numTiles)
                    state = SubTextureUpdateState::Grew;
                else if (tiles < subTexture.numTiles)
                    state = SubTextureUpdateState::Shrank;
            }
            else
            {
                // if tiles is now below the limit but it used to be bigger, delete 
                if (subTexture.numTiles > 0)
                    state = SubTextureUpdateState::Deleted;
            }

            if (state != SubTextureUpdateState::NoChange)
            {
                // If change, produce output
                int outputIndex = Threading::Interlocked::Add(context->numOutputs, 1);
                n_assert(outputIndex < SubTextureMaxUpdates);

                SubTextureUpdateJobOutput& output = context->outputs[outputIndex];
                output.index = index;
                output.oldMaxMip = subTexture.maxMip;
                output.oldTiles = subTexture.numTiles;
                output.oldCoord = subTexture.indirectionOffset;

                output.newMaxMip = tiles > 0 ? Math::log2(tiles) : 0;
                output.newTiles = tiles;

                output.mipBias = Math::max(0u, output.newMaxMip - output.oldMaxMip);

                // default is that the subtexture did not change
                output.updateState = state;
            }

        }
    }, terrainVirtualTileState.subTextures.Size(), 256, subtexCtx, {}, &subtexturesDoneCounter, &subtexturesFinishedEvent);

    n_assert(sectionCullDoneCounter == 0);
    sectionCullDoneCounter = 1;  
    const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
    struct TileCullCtx
    {
        Math::vec4 m_col_x[4];
        Math::vec4 m_col_y[4];
        Math::vec4 m_col_z[4];
        Math::vec4 m_col_w[4];
        Math::bbox* boundingBoxes;
        bool* visibilities;
        Threading::AtomicCounter* instanceCounter;
        Terrain::TerrainPatch* patchData;
    };
    TileCullCtx cullCtx;
    cullCtx.m_col_x[0] = Math::splat_x(viewProj.r[0]);
    cullCtx.m_col_x[1] = Math::splat_x(viewProj.r[1]);
    cullCtx.m_col_x[2] = Math::splat_x(viewProj.r[2]);
    cullCtx.m_col_x[3] = Math::splat_x(viewProj.r[3]);

    cullCtx.m_col_y[0] = Math::splat_y(viewProj.r[0]);
    cullCtx.m_col_y[1] = Math::splat_y(viewProj.r[1]);
    cullCtx.m_col_y[2] = Math::splat_y(viewProj.r[2]);
    cullCtx.m_col_y[3] = Math::splat_y(viewProj.r[3]);

    cullCtx.m_col_z[0] = Math::splat_z(viewProj.r[0]);
    cullCtx.m_col_z[1] = Math::splat_z(viewProj.r[1]);
    cullCtx.m_col_z[2] = Math::splat_z(viewProj.r[2]);
    cullCtx.m_col_z[3] = Math::splat_z(viewProj.r[3]);

    cullCtx.m_col_w[0] = Math::splat_w(viewProj.r[0]);
    cullCtx.m_col_w[1] = Math::splat_w(viewProj.r[1]);
    cullCtx.m_col_w[2] = Math::splat_w(viewProj.r[2]);
    cullCtx.m_col_w[3] = Math::splat_w(viewProj.r[3]);

    terrainVirtualTileState.numPatchesThisFrame = 0;
    cullCtx.instanceCounter = &terrainVirtualTileState.numPatchesThisFrame;
    cullCtx.patchData = (Terrain::TerrainPatch*)CoreGraphics::BufferMap(terrainVirtualTileState.patchConstants.buffers[ctx.bufferIndex]);

    sectionCullDoneCounter = runtimes.Size();
    for (IndexT i = 0; i < runtimes.Size(); i++)
    {
        TerrainRuntimeInfo& rt = runtimes[i];

        cullCtx.boundingBoxes = rt.sectionBoxes.Begin();
        cullCtx.visibilities = rt.sectorVisible.Begin();
        Jobs2::JobDispatch([](SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset, void* ctx)
        {
            N_SCOPE(BruteforceViewFrustumCulling, Visibility);
            auto context = static_cast<TileCullCtx*>(ctx);

            // Iterate over work group
            for (IndexT i = 0; i < groupSize; i++)
            {
                // Get item index
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                const Math::ClipStatus::Type clip = context->boundingBoxes[index].clipstatus(context->m_col_x, context->m_col_y, context->m_col_z, context->m_col_w);
                if (clip != Math::ClipStatus::Outside)
                {
                    uint offset = Threading::Interlocked::Add(context->instanceCounter, 1);

                    Terrain::TerrainPatch patch;
                    patch.PosOffset[0] = context->boundingBoxes[index].pmin.x;
                    patch.PosOffset[1] = context->boundingBoxes[index].pmin.z;
                    patch.UvOffset[0] = 0.0f;
                    patch.UvOffset[1] = 0.0f;
                    context->patchData[offset] = patch;
                    context->visibilities[index] = true;
                }
                else
                {
                    context->visibilities[index] = false;
                }
            }
        }, rt.sectionBoxes.Size(), 256, cullCtx, {}, & sectionCullDoneCounter, & sectionCullFinishedEvent);
    }
    CoreGraphics::BufferUnmap(terrainVirtualTileState.patchConstants.buffers[ctx.bufferIndex]);
    if (runtimes.IsEmpty())
        sectionCullFinishedEvent.Signal();
}

//------------------------------------------------------------------------------
/**
    Unpack from packed ushort vectors to full size
*/
void
UnpackPageDataEntry(uint* packed, uint& status, uint& subTextureIndex, uint& mip, uint& maxMip, uint& subTextureTileX, uint& subTextureTileY)
{
    status = packed[0] & 0x3;
    subTextureIndex = packed[0] >> 2;
    mip = packed[1] & 0xF;
    maxMip = (packed[1] >> 4) & 0xF;
    subTextureTileX = (packed[1] >> 8) & 0xFF;
    subTextureTileY = (packed[1] >> 16) & 0xFF;

    // Delete these
    n_assert(subTextureTileX == packed[2]);
    n_assert(subTextureTileY == packed[3]);
}

//------------------------------------------------------------------------------
/**
*/
template<typename T>
uint
Upload(T* data, uint size, uint alignment)
{
    uint& offset = terrainVirtualTileState.indirectionUploadOffsets[CoreGraphics::GetBufferedFrameIndex()];
    uint uploadOffset = Math::align(offset, alignment);
    CoreGraphics::BufferUpdateArray(terrainVirtualTileState.indirectionUploadBuffers.Buffer(), data, size, offset);
    offset = uploadOffset + size * sizeof(T);
    return uploadOffset;
}

//------------------------------------------------------------------------------
/**
*/
void
IndirectionUpdate(
    uint mip
    , uint physicalOffsetX
    , uint physicalOffsetY
    , uint indirectionOffsetX
    , uint indirectionOffsetY
    , uint subTextureTileX
    , uint subTextureTileY
)
{
    IndirectionEntry entry;
    entry.mip = mip;
    entry.physicalOffsetX = physicalOffsetX;
    entry.physicalOffsetY = physicalOffsetY;

    // Calculate indirection pixel in subtexture
    uint indirectionPixelX = (indirectionOffsetX >> mip) + subTextureTileX;
    uint indirectionPixelY = (indirectionOffsetY >> mip) + subTextureTileY;

    terrainVirtualTileState.indirectionEntryUpdates.Append(entry);
    terrainVirtualTileState.indirectionTextureCopies.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(indirectionPixelX, indirectionPixelY, indirectionPixelX + 1, indirectionPixelY + 1), (uint)mip, 0 });

    uint mipOffset = terrainVirtualTileState.indirectionMipOffsets[mip];
    uint size = terrainVirtualTileState.indirectionMipSizes[mip];
    terrainVirtualTileState.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * size] = entry;
}

//------------------------------------------------------------------------------
/**
*/
void
IndirectionErase(
    uint mip
    , uint indirectionOffsetX
    , uint indirectionOffsetY
    , uint subTextureTileX
    , uint subTextureTileY
)
{
    IndirectionEntry entry{ 0xFFFFFFFF };

    // Calculate indirection pixel in subtexture
    uint indirectionPixelX = (indirectionOffsetX >> mip) + subTextureTileX;
    uint indirectionPixelY = (indirectionOffsetY >> mip) + subTextureTileY;

    terrainVirtualTileState.indirectionEntryUpdates.Append(entry);
    terrainVirtualTileState.indirectionTextureCopies.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(indirectionPixelX, indirectionPixelY, indirectionPixelX + 1, indirectionPixelY + 1), (uint)mip, 0 });

    uint mipOffset = terrainVirtualTileState.indirectionMipOffsets[mip];
    uint size = terrainVirtualTileState.indirectionMipSizes[mip];
    terrainVirtualTileState.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * size] = entry;
}
//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is bigger by mapping mips 0..X to 1..X in the new region
*/
void
IndirectionMoveGrow(
    uint oldMaxMip
    , uint oldTiles
    , const Math::uint2& oldCoord
    , uint newMaxMip
    , uint newTiles
    , const Math::uint2& newCoord)
{
    // Calculate difference in mips, which is our mip offset
    uint mipDiff = newMaxMip - oldMaxMip;

    // If subtexture grew, copy mipchain of old section to the new section, shifted by the difference in mips
    // Lod is the mip level for the new tile
    for (uint i = 0; i <= oldMaxMip; i++)
    {
        // We need to remap the entirety of the old subtexture indirection texels to the new area.
        // For 64x64 -> 256x256 shift, it would mean mapping mips
        // [0, 1, 2, 3, ...] -> [2, 3, 4, 5, ...] 
        uint newMip = i + mipDiff;
        uint width = oldTiles >> i;
        n_assert(newTiles >> newMip == width);
        uint dataSize = width * sizeof(IndirectionEntry);

        // newCoord and oldCoord are in their respective mip 0 basis, so we have to shift them
        // to get the proper coordinates
        Math::uint2 mippedNewCoord{ newCoord.x >> newMip, newCoord.y >> newMip };
        Math::uint2 mippedOldCoord{ oldCoord.x >> i, oldCoord.y >> i };

        terrainVirtualTileState.indirectionTextureFromCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), i, 0 });
        terrainVirtualTileState.indirectionTextureToCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), newMip, 0 });

        // Update CPU buffer
        uint fromMipOffset = terrainVirtualTileState.indirectionMipOffsets[i];
        uint fromSize = terrainVirtualTileState.indirectionMipSizes[i];
        uint fromStart = mippedOldCoord.x + mippedOldCoord.y * fromSize;
        uint toMipOffset = terrainVirtualTileState.indirectionMipOffsets[newMip];
        uint toSize = terrainVirtualTileState.indirectionMipSizes[newMip];
        uint toStart = mippedNewCoord.x + mippedNewCoord.y * toSize;

        for (uint j = 0; j < width; j++)
        {
            uint fromRowOffset = fromSize * j;
            uint toRowOffset = toSize * j;
            memmove(terrainVirtualTileState.indirectionBuffer.Begin() + toMipOffset + toRowOffset + toStart, terrainVirtualTileState.indirectionBuffer.Begin() + fromMipOffset + fromRowOffset + fromStart, dataSize);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is smaller, mapping mips 0..X to 0..X-1 in the new region
*/
void
IndirectionMoveShrink(
    uint oldMaxMip
    , uint oldTiles
    , const Math::uint2& oldCoord
    , uint newMaxMip
    , uint newTiles
    , const Math::uint2& newCoord)
{
    // If we shrink, the old mips should be higher, so this should be our mip shift offset
    uint mipDiff = oldMaxMip - newMaxMip;

    // Since we shrink, we need to go through our new mips and copy old data over
    for (uint i = 0; i <= newMaxMip; i++)
    {
        // Remapping from the old bigger subtexture to the new smaller requires us copy from the source shifted to the next mips
        // For a 256x256 -> 64x64 shift this would mean copying mips
        // [2, 3, 4, 5, ...] -> [0, 1, 2, 3, ...]
        uint oldMip = mipDiff + i;
        uint width = newTiles >> i;
        n_assert(oldTiles >> oldMip == width);
        uint dataSize = width * sizeof(IndirectionEntry);

        // newCoord and oldCoord are in their respective mip 0 basis, so we have to shift them
        // to get the proper coordinates
        Math::uint2 mippedNewCoord{ newCoord.x >> i, newCoord.y >> i };
        Math::uint2 mippedOldCoord{ oldCoord.x >> oldMip, oldCoord.y >> oldMip };

        terrainVirtualTileState.indirectionTextureFromCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), oldMip, 0 });
        terrainVirtualTileState.indirectionTextureToCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), i, 0 });

        // Update CPU buffer
        uint fromMipOffset = terrainVirtualTileState.indirectionMipOffsets[oldMip];
        uint fromSize = terrainVirtualTileState.indirectionMipSizes[oldMip];
        uint fromStart = mippedOldCoord.x + mippedOldCoord.y * fromSize;
        uint toMipOffset = terrainVirtualTileState.indirectionMipOffsets[i];
        uint toSize = terrainVirtualTileState.indirectionMipSizes[i];
        uint toStart = mippedNewCoord.x + mippedNewCoord.y * toSize;

        for (uint j = 0; j < width; j++)
        {
            uint fromRowOffset = fromSize * j;
            uint toRowOffset = toSize * j;
            memmove(terrainVirtualTileState.indirectionBuffer.Begin() + toMipOffset + toRowOffset + toStart, terrainVirtualTileState.indirectionBuffer.Begin() + fromMipOffset + fromRowOffset + fromStart, dataSize);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Clear old region
*/
void
IndirectionClear(
    uint mips
    , uint tiles
    , const Math::uint2& coord)
{
    uint width = tiles;
    uint dataSize = width * width * sizeof(IndirectionEntry);

    // Grab the largest mip and fill with 0 pixels
    Util::FixedArray<IndirectionEntry> pixels(width * width);
    pixels.Fill(IndirectionEntry{ 0xFFFFFFFF });

    // Upload to GPU, the lowest mip will have enough values to cover all mips
    uint offset = Upload(pixels.Begin(), pixels.Size(), 4);

    // go through old region and reset indirection pixels
    for (uint i = 0; i <= mips; i++)
    {
        Math::uint2 mippedCoord{ coord.x >> i, coord.y >> i };
        uint width = tiles >> i;

        terrainVirtualTileState.indirectionBufferClearsThisFrame.Append(CoreGraphics::BufferCopy{ static_cast<uint>(offset) });
        terrainVirtualTileState.indirectionTextureClearsThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedCoord.x, mippedCoord.y, mippedCoord.x + width, mippedCoord.y + width), i, 0 });

        // Update CPU buffer
        uint mipOffset = terrainVirtualTileState.indirectionMipOffsets[i];
        uint rowSize = terrainVirtualTileState.indirectionMipSizes[i];
        uint rowStart = mippedCoord.x + mippedCoord.y * rowSize;

        for (uint j = 0; j < width; j++)
        {
            uint rowOffset = rowSize * j;
            memcpy(terrainVirtualTileState.indirectionBuffer.Begin() + mipOffset + rowOffset + rowStart, pixels.Begin(), width * sizeof(IndirectionEntry));
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::UpdateLOD(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(TerrainUpdateVirtualTexturing, Terrain);
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(view->GetCamera()));
    const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

    Math::mat4 sunTransform = Lighting::LightContext::GetTransform(terrainState.sun);
    if (terrainState.cachedSunDirection != sunTransform.z_axis)
    {
        terrainState.updateShadowMap = true;
        terrainState.cachedSunDirection = sunTransform.z_axis;
    }

    CoreGraphics::TextureId posBuffer = view->GetFrameScript()->GetTexture("TerrainPosBuffer");

    Terrain::TerrainSystemUniforms systemUniforms;
    systemUniforms.TerrainPosBuffer = CoreGraphics::TextureGetBindlessHandle(posBuffer);
    systemUniforms.MinLODDistance = 0.0f;
    systemUniforms.MaxLODDistance = terrainState.mipLoadDistance;
    systemUniforms.VirtualLodDistance = terrainState.mipLoadDistance;
    systemUniforms.MinTessellation = 1.0f;
    systemUniforms.MaxTessellation = 32.0f;
    systemUniforms.Debug = terrainState.debugRender;
    systemUniforms.NumBiomes = terrainState.biomeCounter;
    systemUniforms.AlbedoPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.physicalAlbedoCache);
    systemUniforms.NormalPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.physicalNormalCache);
    systemUniforms.MaterialPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.physicalMaterialCache);
    systemUniforms.AlbedoLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.lowresAlbedo);
    systemUniforms.NormalLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.lowresNormal);
    systemUniforms.MaterialLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.lowresMaterial);
    systemUniforms.IndirectionBuffer = CoreGraphics::TextureGetBindlessHandle(terrainVirtualTileState.indirectionTexture);

    for (IndexT j = 0; j < terrainState.biomeCounter; j++)
    {
        BiomeParameters settings = terrainBiomeAllocator.Get<TerrainBiome_Settings>(j).biomeParameters;
        if (!terrainState.biomeLowresGenerated && AllBits(terrainState.biomeLoaded[j], BiomeLoadBits::AlbedoLoaded | BiomeLoadBits::NormalLoaded | BiomeLoadBits::MaterialLoaded | BiomeLoadBits::MaskLoaded | BiomeLoadBits::WeightsLoaded))
        {
            terrainVirtualTileState.updateLowres = true;
            terrainState.biomeLowresGenerated[j] = true;
        }
        
        CoreGraphics::TextureId biomeMask = terrainState.biomeMasks[j];
		if (biomeMask != CoreGraphics::InvalidTextureId)
		{
			systemUniforms.BiomeRules[j][3] = CoreGraphics::TextureGetNumMips(terrainState.biomeMasks[j]);
		}
        systemUniforms.BiomeRules[j][0] = settings.slopeThreshold;
        systemUniforms.BiomeRules[j][1] = settings.heightThreshold;
        systemUniforms.BiomeRules[j][2] = settings.uvScaleFactor;
    }
    BufferUpdate(terrainState.systemConstants, systemUniforms, 0);

    struct PendingDelete
    {
        uint oldMaxMip;
        uint oldTiles;
        Math::uint2 oldCoord;
    };

    // Wait for subtextures job to finish this frame
    subtexturesFinishedEvent.Wait();

    for (IndexT i = 0; i < terrainVirtualTileState.subTextureNumOutputs; i++)
    {
        const Terrain::SubTextureUpdateJobOutput& output = terrainVirtualTileState.subTextureJobOutputs[i];
        Terrain::SubTexture& subTex = terrainVirtualTileState.subTextures[output.index];
        Terrain::TerrainSubTexture& compressedSubTex = terrainVirtualTileState.gpuSubTextures[output.index];

        switch (output.updateState)
        {
            case SubTextureUpdateState::Grew:
            case SubTextureUpdateState::Shrank:
                terrainVirtualTileState.indirectionOccupancy.Deallocate(output.oldCoord, output.oldTiles);
                [[fallthrough]];
            case SubTextureUpdateState::Created:
            {
                Math::uint2 newCoord = terrainVirtualTileState.indirectionOccupancy.Allocate(output.newTiles);
                if (newCoord.x == 0xFFFFFFFF || newCoord.y == 0xFFFFFFFF)
                    break;
                //n_assert(newCoord.x != 0xFFFFFFFF && newCoord.y != 0xFFFFFFFF);

                // update subtexture
                subTex.numTiles = output.newTiles;
                subTex.indirectionOffset = newCoord;
                subTex.maxMip = output.newMaxMip;
                terrainVirtualTileState.virtualSubtextureBufferUpdate = true;
                switch (output.updateState)
                {
                    case SubTextureUpdateState::Grew:
                        IndirectionClear(output.newMaxMip - output.oldMaxMip, output.newTiles, newCoord);
                        IndirectionMoveGrow(output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, newCoord);
                        break;
                    case SubTextureUpdateState::Shrank:
                        IndirectionMoveShrink(output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, newCoord);
                        break;
                    case SubTextureUpdateState::Created:
                        IndirectionClear(output.newMaxMip, output.newTiles, newCoord);
                        break;
                }
                PackSubTexture(subTex, compressedSubTex);
                break;
            }
            case SubTextureUpdateState::Deleted:
                terrainVirtualTileState.indirectionOccupancy.Deallocate(output.oldCoord, output.oldTiles);
                subTex.numTiles = 0;
                subTex.indirectionOffset.x = 0xFFFFFFFF;
                subTex.indirectionOffset.y = 0xFFFFFFFF;
                subTex.maxMip = 0;
                terrainVirtualTileState.virtualSubtextureBufferUpdate = true;
                PackSubTexture(subTex, compressedSubTex);
                break;
            default:
                break;
        }
    }

    // Handle readback from the GPU
    CoreGraphics::BufferInvalidate(terrainVirtualTileState.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);
    Terrain::PageUpdateList* updateList = (Terrain::PageUpdateList*)CoreGraphics::BufferMap(terrainVirtualTileState.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);
    terrainVirtualTileState.numPixels = updateList->NumEntries;
    n_assert(terrainVirtualTileState.numPixels < Terrain::MAX_PAGE_UPDATES);
    for (uint i = 0; i < updateList->NumEntries; i++)
    {
        uint status, subTextureIndex, mip, maxMip, subTextureTileX, subTextureTileY;
        UnpackPageDataEntry(updateList->Entry[i], status, subTextureIndex, mip, maxMip, subTextureTileX, subTextureTileY);

        // the update state is either 1 if the page is allocated, or 2 if it used to be allocated but has since been deallocated
        uint updateState = status;
        SubTexture& subTexture = terrainVirtualTileState.subTextures[subTextureIndex];

        if (subTexture.numTiles == 0)
            continue;

        n_assert(mip < 0xF);
        n_assert(subTextureTileX < PhysicalTextureTileSize);
        n_assert(subTextureTileY < PhysicalTextureTileSize);

        // If the subtexture has changed size, just ignore this update
        if (subTexture.maxMip != maxMip)
            continue;

        TileCacheEntry cacheEntry;
        cacheEntry.entry.tileX = subTextureTileX;
        cacheEntry.entry.tileY = subTextureTileY;
        cacheEntry.entry.tiles = subTexture.numTiles >> mip;
        cacheEntry.entry.subTextureIndex = subTextureIndex;

        // Get new or cached coords from the virtual texture cache
        TextureTileCache::CacheResult result = terrainVirtualTileState.physicalTextureTileCache.Cache(cacheEntry);
        if (result.didCache)
        {
            IndirectionUpdate(mip, result.cached.x, result.cached.y, subTexture.indirectionOffset.x, subTexture.indirectionOffset.y, subTextureTileX, subTextureTileY);

            // Calculate some metrics, the meters per tile, meters per pixel and then the padded meters per tile value
            float metersPerTile = SubTextureWorldSize / float(subTexture.numTiles >> mip);
            float metersPerPixel = metersPerTile / float(PhysicalTextureTilePaddedSize);
            float metersPerTilePadded = metersPerTile + PhysicalTextureTilePadding * metersPerPixel;

            Terrain::TerrainTileUpdateUniforms tileUpdateUniforms;
            tileUpdateUniforms.MetersPerTile = metersPerTilePadded;

            // Calculate world offsets, it's the offset of the subtexture, plus the tile scaled down to pixel level, and with the padding subtracted
            tileUpdateUniforms.SparseTileWorldOffset[0] = subTexture.worldCoordinate.x + subTextureTileX * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
            tileUpdateUniforms.SparseTileWorldOffset[1] = subTexture.worldCoordinate.y + subTextureTileY * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
                
            terrainVirtualTileState.pageUniforms.Append(tileUpdateUniforms);
            terrainVirtualTileState.tileOffsets.Append({ result.cached.x, result.cached.y });
        }
        else
        {
            // Calculate indirection pixel in subtexture
            uint indirectionPixelX = (subTexture.indirectionOffset.x >> mip) + subTextureTileX;
            uint indirectionPixelY = (subTexture.indirectionOffset.y >> mip) + subTextureTileY;
            uint mipOffset = terrainVirtualTileState.indirectionMipOffsets[mip];
            uint mipSize = terrainVirtualTileState.indirectionMipSizes[mip];

            // If the cache has the tile but the indirection has been cleared, issue an update for the indirection only
            const IndirectionEntry& entry = terrainVirtualTileState.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * mipSize];
            if (entry.mip == 0xF)
                IndirectionUpdate(mip, result.cached.x, result.cached.y, subTexture.indirectionOffset.x, subTexture.indirectionOffset.y, subTextureTileX, subTextureTileY);
        }

        // If evicted, clear the tile in the indirection texture
        /*
        if (result.evicted != InvalidTileCacheEntry)
        {
            const TerrainSubTexture& evictSubtexture = terrainVirtualTileState.subTextures[result.evicted.entry.subTextureIndex];

            // Calculate the mip, which is relative to the max number of mips in the current
            // SubTexture and whatever was cached before
            int evictMip = Math::log2(evictSubtexture.tiles) - Math::log2(result.evicted.entry.tiles);

            // If the mip is positive, it means the pixel still exists and can therefore be discarded
            if (evictMip >= 0)
            {
                // Calculate indirection pixel in subtexture
                uint indirectionPixelX = (subTexture.indirectionOffset[0] >> evictMip) + result.evicted.entry.tileX;
                uint indirectionPixelY = (subTexture.indirectionOffset[1] >> evictMip) + result.evicted.entry.tileY;
                uint mipOffset = terrainVirtualTileState.indirectionMipOffsets[evictMip];
                uint mipSize = terrainVirtualTileState.indirectionMipSizes[evictMip];

                // Now simply just clear that pixel
                IndirectionErase(evictMip, indirectionPixelX, indirectionPixelY, result.evicted.entry.tileX, result.evicted.entry.tileY);
            }
        }
        */
    }
    CoreGraphics::BufferUnmap(terrainVirtualTileState.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);

    IndexT i;

    // To avoid too many changes per frame, limit the amount of updates to 64
    static const int NumPagesPerFrame = 256;

    // Setup constants for tile updates
    SizeT numPagesThisFrame = Math::min(NumPagesPerFrame, terrainVirtualTileState.pageUniforms.Size());
    for (i = 0; i < numPagesThisFrame; i++)
    {
        PhysicalTileUpdate pageUpdate;
        pageUpdate.constantBufferOffsets[0] = CoreGraphics::SetConstants(terrainVirtualTileState.pageUniforms[i]);
        pageUpdate.constantBufferOffsets[1] = 0;
        pageUpdate.tileOffset[0] = terrainVirtualTileState.tileOffsets[i].x;
        pageUpdate.tileOffset[1] = terrainVirtualTileState.tileOffsets[i].y;
        terrainVirtualTileState.pageUpdatesThisFrame.Append(pageUpdate);
    }
    if (i > 0)
    {
        terrainVirtualTileState.pageUniforms.EraseRange(0, numPagesThisFrame - 1);
        terrainVirtualTileState.tileOffsets.EraseRange(0, numPagesThisFrame - 1);
    }

    // Update buffers for indirection pixel uploads

    numPagesThisFrame = Math::min(NumPagesPerFrame, terrainVirtualTileState.indirectionEntryUpdates.Size());
    uint offset = Upload(terrainVirtualTileState.indirectionEntryUpdates.Begin(), terrainVirtualTileState.indirectionEntryUpdates.Size(), 4);
    for (i = 0; i < numPagesThisFrame; i++)
    {
        // setup indirection update
        terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Append(CoreGraphics::BufferCopy{ static_cast<uint>(offset + i * sizeof(Terrain::IndirectionEntry)) });
        terrainVirtualTileState.indirectionTextureUpdatesThisFrame.Append(terrainVirtualTileState.indirectionTextureCopies[i]);
    }
    if (i > 0)
    {
        terrainVirtualTileState.indirectionEntryUpdates.EraseRange(0, numPagesThisFrame);
        terrainVirtualTileState.indirectionTextureCopies.EraseRange(0, numPagesThisFrame);
    }

    // Flush upload buffer
    CoreGraphics::BufferFlush(terrainVirtualTileState.indirectionUploadBuffers.buffers[ctx.bufferIndex], 0, terrainVirtualTileState.indirectionUploadOffsets[ctx.bufferIndex]);

    // Wait for jobs to finish
    sectionCullFinishedEvent.Wait();

    if (terrainVirtualTileState.virtualSubtextureBufferUpdate)
    {
        auto bla = reinterpret_cast<SubTextureCompressed*>(terrainVirtualTileState.gpuSubTextures.Begin());
        BufferUpdateArray(
            terrainVirtualTileState.subtextureStagingBuffers.buffers[ctx.bufferIndex],
            terrainVirtualTileState.gpuSubTextures.Begin(),
            terrainVirtualTileState.gpuSubTextures.Size());
    }

    // Setup uniforms for terrain patches
    for (IndexT i = 0; i < runtimes.Size(); i++)
    {
        TerrainRuntimeInfo& rt = runtimes[i];

        if (!rt.lowresGenerated && AllBits(rt.loadBits, TerrainRuntimeInfo::HeightMapLoaded))
        {
            terrainVirtualTileState.updateLowres = true;
            rt.lowresGenerated = true;
        }

        TerrainRuntimeUniforms uniforms;
        Math::mat4().store(uniforms.Transform);
        uniforms.DecisionMap = TextureGetBindlessHandle(CoreGraphics::TextureId(rt.decisionMap));
        uniforms.HeightMap = TextureGetBindlessHandle(CoreGraphics::TextureId(rt.heightMap));

        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(rt.heightMap);
        CoreGraphics::TextureDimensions dataBufferDims = CoreGraphics::TextureGetDimensions(posBuffer);
        uniforms.VirtualTerrainTextureSize[0] = dims.width;
        uniforms.VirtualTerrainTextureSize[1] = dims.height;
        uniforms.VirtualTerrainTextureSize[2] = 1.0f / dims.width;
        uniforms.VirtualTerrainTextureSize[3] = 1.0f / dims.height;
        uniforms.MaxHeight = rt.maxHeight;
        uniforms.MinHeight = rt.minHeight;
        uniforms.WorldSizeX = rt.worldWidth;
        uniforms.WorldSizeZ = rt.worldHeight;
        uniforms.DataBufferSize[0] = dataBufferDims.width;
        uniforms.DataBufferSize[1] = dataBufferDims.height;
        uniforms.NumTilesX = rt.numTilesX;
        uniforms.NumTilesY = rt.numTilesY;
        uniforms.TileWidth = rt.tileWidth;
        uniforms.TileHeight = rt.tileHeight;
        uniforms.VirtualTerrainPageSize[0] = 64.0f;
        uniforms.VirtualTerrainPageSize[1] = 64.0f;
        uniforms.VirtualTerrainSubTextureSize[0] = SubTextureWorldSize;
        uniforms.VirtualTerrainSubTextureSize[1] = SubTextureWorldSize;
        uniforms.VirtualTerrainNumSubTextures[0] = terrainState.settings.worldSizeX / SubTextureWorldSize;
        uniforms.VirtualTerrainNumSubTextures[1] = terrainState.settings.worldSizeZ / SubTextureWorldSize;
        uniforms.PhysicalInvPaddedTextureSize = 1.0f / PhysicalTexturePaddedSize;
        uniforms.PhysicalTileSize = PhysicalTextureTileSize;
        uniforms.PhysicalTilePaddedSize = PhysicalTextureTilePaddedSize;
        uniforms.PhysicalTilePadding = PhysicalTextureTileHalfPadding;

        CoreGraphics::TextureDimensions indirectionDims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);
        uniforms.VirtualTerrainNumPages[0] = indirectionDims.width;
        uniforms.VirtualTerrainNumPages[1] = indirectionDims.height;
        uniforms.VirtualTerrainNumMips = IndirectionNumMips;

        uniforms.LowresResolution[0] = uniforms.LowresResolution[1] = LowresFallbackSize;
        uniforms.LowresNumMips = LowresFallbackMips - 1;

        uniforms.IndirectionResolution[0] = uniforms.IndirectionResolution[1] = IndirectionTextureSize;
        uniforms.IndirectionNumMips = IndirectionNumMips - 1;

        uniforms.LowresFadeStart = SubTextureFadeStart * SubTextureFadeStart;
        uniforms.LowresFadeDistance = 1.0f / ((SubTextureRange - SubTextureFadeStart) * (SubTextureRange - SubTextureFadeStart));

        for (SizeT j = 0; j < terrainVirtualTileState.indirectionMipOffsets.Size(); j++)
        {
            uniforms.VirtualPageBufferMipOffsets[j / 4][j % 4] = terrainVirtualTileState.indirectionMipOffsets[j];
            uniforms.VirtualPageBufferMipSizes[j / 4][j % 4] = terrainVirtualTileState.indirectionMipSizes[j];
        }
        uniforms.VirtualPageBufferNumPages = CoreGraphics::BufferGetSize(terrainVirtualTileState.pageStatusBuffer);

        BufferUpdate(terrainVirtualTileState.runtimeConstants, uniforms, 0);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::RenderUI(const Graphics::FrameContext& ctx)
{
    if (Core::CVarReadInt(Core::CVarGet("r_terrain_debug")) > 0)
    {
        if (ImGui::Begin("Terrain Debug 2"))
        {
            ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
            ImGui::Checkbox("Debug Render", &terrainState.debugRender);
            ImGui::Checkbox("Render", &terrainState.renderToggle);
            ImGui::LabelText("Updates", "Number of updates %d", terrainVirtualTileState.numPixels);

            {
                ImGui::Text("Indirection texture occupancy quadtree");
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 start = ImGui::GetCursorScreenPos();
                ImVec2 fullSize = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
                drawList->PushClipRect(
                    ImVec2{ start.x, start.y },
                    ImVec2{ Math::max(start.x + fullSize.x, start.x + 512.0f), Math::min(start.y + fullSize.y, start.y + 512.0f) }, true);

                terrainVirtualTileState.indirectionOccupancy.DebugRender(drawList, start, 0.25f);
                drawList->PopClipRect();

                // set back cursor so we can draw our box
                ImGui::SetCursorScreenPos(start);
                ImGui::InvisibleButton("Indirection texture occupancy quadtree", ImVec2(512.0f, 512.0f));
            }

            {
                /*
                ImGui::Text("Physical texture occupancy quadtree");
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 start = ImGui::GetCursorScreenPos();
                ImVec2 fullSize = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
                drawList->PushClipRect(
                    ImVec2{ start.x, start.y },
                    ImVec2{ Math::max(start.x + fullSize.x, start.x + 512.0f), Math::min(start.y + fullSize.y, start.y + 512.0f) }, true);

                terrainVirtualTileState.physicalTextureTileOccupancy.DebugRender(drawList, start, 0.0625f);
                drawList->PopClipRect();

                // set back cursor so we can draw our box
                ImGui::SetCursorScreenPos(start);
                ImGui::InvisibleButton("Physical texture occupancy quadtree", ImVec2(512.0f, 512.0f));
                */
            }

            {

                ImGui::NewLine();
                ImGui::Separator();
                ImGui::Text("Physical Normal Cache");
                CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.physicalNormalCache);

                ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                static Dynui::ImguiTextureId textureInfo;
                textureInfo.nebulaHandle = terrainVirtualTileState.physicalNormalCache;
                textureInfo.mip = 0;
                textureInfo.layer = 0;

                imageSize.x = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
                float ratio = (float)dims.height / (float)dims.width;
                imageSize.y = imageSize.x * ratio;

                ImGui::Image((void*)&textureInfo, imageSize);
            }

            {

                ImGui::NewLine();
                ImGui::Separator();
                ImGui::Text("Terrain Shadow Map");
                static int mip = 0;
                ImGui::InputInt("Mip", &mip, 1, 1);

                CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainState.terrainShadowMap);

                ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                static Dynui::ImguiTextureId textureInfo;
                textureInfo.nebulaHandle = terrainState.terrainShadowMap;
                textureInfo.mip = mip;
                textureInfo.layer = 0;


                imageSize.x = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
                float ratio = (float)dims.height / (float)dims.width;
                imageSize.y = imageSize.x * ratio;

                ImGui::Image((void*)&textureInfo, imageSize);
            }
        }

        ImGui::End();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::ClearCache()
{
    terrainVirtualTileState.physicalTextureTileCache.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::OnRenderDebug(uint32_t flags)
{
    using namespace CoreGraphics;
    ShapeRenderer* shapeRenderer = ShapeRenderer::Instance();
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
    for (IndexT i = 0; i < runtimes.Size(); i++)
    {
        TerrainRuntimeInfo& rt = runtimes[i];
        for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
        {
            RenderShape shape(RenderShape::Box, RenderShape::Wireframe, rt.sectionBoxes[j].to_mat4(), Math::vec4(0, 1, 0, 1));
            shapeRenderer->AddShape(shape);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
TerrainContext::Alloc()
{
    return terrainAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::Dealloc(Graphics::ContextEntityId id)
{
    terrainAllocator.Dealloc(id.id);
}

} // namespace Terrain
