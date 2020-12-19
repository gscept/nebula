//------------------------------------------------------------------------------
//  terraincontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "terraincontext.h"
#include "coregraphics/image.h"
#include "graphics/graphicsserver.h"
#include "frame/frameplugin.h"
#include "graphics/cameracontext.h"
#include "graphics/view.h"
#include "dynui/imguicontext.h"
#include "imgui.h"
#include "renderutil/drawfullscreenquad.h"

#include "occupancyquadtree.h"
#include "texturetilecache.h"

#include "resources/resourceserver.h"
#include "gliml.h"

#include "terrain.h"

N_DECLARE_COUNTER(N_TERRAIN_TOTAL_AVAILABLE_DATA, Terrain Total Data Size);

namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
TerrainContext::TerrainBiomeAllocator TerrainContext::terrainBiomeAllocator;

extern void TerrainCullJob(const Jobs::JobFuncContext& ctx);
extern void TerrainSubTextureUpdateJob(const Jobs::JobFuncContext& ctx);
Util::Queue<Jobs::JobId> TerrainContext::runningJobs;
Jobs::JobSyncId TerrainContext::jobHostSync;


const uint IndirectionTextureSize = 2048;
const uint SubTextureWorldSize = 64;
const uint SubTextureMaxTiles = 256;
const uint IndirectionNumMips = Math::n_log2(SubTextureMaxTiles) + 1;

const uint LowresFallbackSize = 4096;
const uint LowresFallbackMips = Math::n_log2(LowresFallbackSize) + 1;

const uint PhysicalTextureTileSize = 256;
const uint PhysicalTextureTileHalfPadding = 4;
const uint PhysicalTextureTilePadding = PhysicalTextureTileHalfPadding * 2;
const uint PhysicalTextureNumTiles = 32;
const uint PhysicalTextureTilePaddedSize = PhysicalTextureTileSize + PhysicalTextureTilePadding;
const uint PhysicalTextureSize = (PhysicalTextureTileSize) * PhysicalTextureNumTiles;
const uint PhysicalTexturePaddedSize = (PhysicalTextureTilePaddedSize) * PhysicalTextureNumTiles;

const uint IndirectionUpdateBufferSize = 1024 * 1024 * sizeof(Terrain::IndirectionEntry);

_ImplementContext(TerrainContext, TerrainContext::terrainAllocator);


struct
{
    TerrainSetupSettings settings;
    CoreGraphics::ShaderId terrainShader;
    CoreGraphics::ShaderProgramId terrainZProgram;
    CoreGraphics::ShaderProgramId terrainPrepassProgram;
    CoreGraphics::ShaderProgramId terrainProgram;
    CoreGraphics::ShaderProgramId terrainScreenPass;
    CoreGraphics::ResourceTableId resourceTable;
    CoreGraphics::BufferId systemConstants;
    Util::Array<CoreGraphics::VertexComponent> components;
    CoreGraphics::VertexLayoutId vlo;

    CoreGraphics::WindowId wnd;

    CoreGraphics::TextureId biomeAlbedoArray[Terrain::MAX_BIOMES];
    CoreGraphics::TextureId biomeNormalArray[Terrain::MAX_BIOMES];
    CoreGraphics::TextureId biomeMaterialArray[Terrain::MAX_BIOMES];
    CoreGraphics::TextureId biomeMasks[Terrain::MAX_BIOMES];
    IndexT biomeCounter;

    CoreGraphics::TextureId terrainPosBuffer;
    
    IndexT albedoSlot;
    IndexT normalsSlot;
    IndexT pbrSlot;
    IndexT maskSlot;

    float mipLoadDistance = 1500.0f;
    float mipRenderPadding = 150.0f;
    SizeT layers;

    bool debugRender;
    bool renderToggle;

} terrainState;

struct PhysicalPageUpdate
{
    uint constantBufferOffsets[2];
    uint tileOffset[2];
};

struct
{
    CoreGraphics::ShaderProgramId                                   terrainPrepassProgram;
    CoreGraphics::ShaderProgramId                                   terrainPageClearUpdateBufferProgram;
    CoreGraphics::ShaderProgramId                                   terrainScreenspacePass;
    CoreGraphics::ShaderProgramId                                   terrainTileUpdateProgram;
    CoreGraphics::ShaderProgramId                                   terrainTileFallbackProgram;

    bool                                                            virtualSubtextureBufferUpdate;
    Util::FixedArray<CoreGraphics::BufferId>                        subtextureStagingBuffers;
    CoreGraphics::BufferId                                          subTextureBuffer;

    Util::FixedArray < CoreGraphics::BufferId>                      pageUpdateReadbackBuffers;
    CoreGraphics::TextureId                                         indirectionTexture;
    CoreGraphics::BufferId                                          pageUpdateListBuffer;
    CoreGraphics::BufferId                                          pageStatusBuffer;
    CoreGraphics::BufferId                                          pageStatusClearBuffer;

    Util::FixedArray<CoreGraphics::BufferId>                        indirectionUploadBuffers;

    CoreGraphics::BufferId                                          runtimeConstants;

    CoreGraphics::TextureId                                         physicalAlbedoCache;
    CoreGraphics::TextureId                                         physicalNormalCache;
    CoreGraphics::TextureId                                         physicalMaterialCache;
    CoreGraphics::TextureId                                         lowresAlbedo;
    CoreGraphics::TextureId                                         lowresNormal;
    CoreGraphics::TextureId                                         lowresMaterial;

    CoreGraphics::EventId                                           biomeUpdatedEvent;
    bool                                                            updateLowres = false;

    CoreGraphics::ResourceTableId                                   virtualTerrainSystemResourceTable;
    CoreGraphics::ResourceTableId                                   virtualTerrainRuntimeResourceTable;
    CoreGraphics::ResourceTableId                                   virtualTerrainDynamicResourceTable;

    SizeT                                                           numPageBufferUpdateEntries;

    Util::FixedArray<Terrain::TerrainSubTexture>                    subTextures;
    Util::FixedArray<uint8>                                         subTextureUpdateKeys;
    Util::FixedArray<Util::FixedArray<Terrain::TerrainSubTexture>>  subTexturesHistory;
    Util::FixedArray<Util::FixedArray<uint8>>                       subTextureUpdateKeysHistory;
    Util::FixedArray<Terrain::SubTextureUpdateJobOutput>            subTextureJobOutputs;
    Util::FixedArray<Util::Array<Terrain::SubTextureUpdateJobOutput>> subTextureJobHistoryOutputs;
    OccupancyQuadTree                                               indirectionOccupancy;
    OccupancyQuadTree                                               physicalTextureTileOccupancy;
    TextureTileCache                                                physicalTextureTileCache;

    Util::FixedArray<uint>                                          indirectionMipOffsets;
    Util::FixedArray<uint>                                          indirectionMipSizes;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureCopies;
    Util::Array<IndirectionEntry>                                   indirectionEntryUpdates;

    CoreGraphics::PassId                                            tileUpdatePass;
    CoreGraphics::PassId                                            tileFallbackPass;

    Util::Array<Terrain::TerrainTileUpdateUniforms>                 pageUniforms;
    Util::Array<Math::uint2>                                        tileOffsets;
    Util::Array<PhysicalPageUpdate>                                 pageUpdatesThisFrame;
    Util::Array<CoreGraphics::BufferCopy>                           indirectionBufferUpdatesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureUpdatesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureCopiesFromThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureCopiesToThisFrame;
    Util::Array<CoreGraphics::BufferCopy>                           indirectionBufferShufflesThisFrame;
    Util::Array<CoreGraphics::TextureCopy>                          indirectionTextureShufflesThisFrame;
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
    _CreateContext();
    using namespace CoreGraphics;

    __bundle.OnPrepareView = TerrainContext::CullPatches;
    __bundle.OnUpdateViewResources = TerrainContext::UpdateLOD;
    __bundle.OnBegin = TerrainContext::RenderUI;

    Jobs::CreateJobSyncInfo sinfo =
    {
        nullptr
    };
    TerrainContext::jobHostSync = Jobs::CreateJobSync(sinfo);

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    terrainState.settings = settings;

    Frame::AddCallback("TerrainContext - Render Terrain GBuffer", [](const IndexT frame, const IndexT bufferIndex)
        {
            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

            // setup shader state, set shader before we set the vertex layout
            SetShaderProgram(terrainState.terrainProgram);
            SetVertexLayout(terrainState.vlo);
            SetPrimitiveTopology(PrimitiveTopology::PatchList);
            SetGraphicsPipeline();

            // set shared resources
            SetResourceTable(terrainState.resourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, "Terrain Sections");

            // go through and render terrain instances
            for (IndexT i = 0; i < runtimes.Size(); i++)
            {
                TerrainRuntimeInfo& rt = runtimes[i];
                SetResourceTable(rt.terrainResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
                {
                    if (rt.sectorVisible[j])
                    {
                        SetStreamVertexBuffer(0, rt.vbo, 0);
                        SetIndexBuffer(rt.ibo, 0);
                        SetPrimitiveGroup(rt.sectorPrimGroups[j]);
                        SetResourceTable(rt.patchTable, NEBULA_DYNAMIC_OFFSET_GROUP, GraphicsPipeline, 2, &rt.sectorUniformOffsets[j][0]);
                        Draw();
                    }
                }
            }

            CommandBufferEndMarker(GraphicsQueueType);
        });

    Frame::AddCallback("TerrainContext - Render Terrain Screenspace", [](const IndexT frame, const IndexT bufferIndex)
        {
            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
            if (runtimes.Size() > 0)
            {
                // set shader and draw fsq
                SetShaderProgram(terrainState.terrainScreenPass);
                CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
                RenderUtil::DrawFullScreenQuad::ApplyMesh();
                SetResourceTable(terrainState.resourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                CoreGraphics::Draw();
                CoreGraphics::EndBatch();
            }
        });

    Frame::AddCallback("TerrainContext - Terrain Shadows", [](const IndexT frame, const IndexT bufferIndex)
        {
        });

    // create vertex buffer
    terrainState.components =
    {
        VertexComponent{ (VertexComponent::SemanticName)0, 0, VertexComponent::Format::Float3 },
        VertexComponent{ (VertexComponent::SemanticName)1, 0, VertexComponent::Format::Float2 },
        VertexComponent{ (VertexComponent::SemanticName)2, 0, VertexComponent::Format::Float2 },
    };

    terrainState.terrainShader = ShaderGet("shd:terrain.fxb");
    terrainState.terrainZProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainZ"));
    terrainState.terrainProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("Terrain"));
    terrainState.terrainScreenPass = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainScreenSpace"));
    terrainState.resourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);
    IndexT systemConstantsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TerrainSystemUniforms");

    CoreGraphics::BufferCreateInfo sysBufInfo;
    sysBufInfo.name = "VirtualSystemBuffer"_atm;
    sysBufInfo.size = 1;
    sysBufInfo.elementSize = sizeof(Terrain::TerrainSystemUniforms);
    sysBufInfo.mode = CoreGraphics::HostToDevice;
    sysBufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    terrainState.systemConstants = CoreGraphics::CreateBuffer(sysBufInfo);

    terrainState.albedoSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialAlbedo");
    terrainState.normalsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialNormals");
    terrainState.pbrSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialPBR");
    terrainState.maskSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialMask");

    ResourceTableSetConstantBuffer(terrainState.resourceTable, { terrainState.systemConstants, systemConstantsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0});
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
    terrainVirtualTileState.terrainScreenspacePass = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainVirtualScreenSpace"));
    terrainVirtualTileState.terrainTileUpdateProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainTileUpdate"));
    terrainVirtualTileState.terrainTileFallbackProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainLowresFallback"));

    terrainVirtualTileState.virtualTerrainSystemResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);
    terrainVirtualTileState.virtualTerrainRuntimeResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
    terrainVirtualTileState.virtualTerrainDynamicResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "VirtualRuntimeBuffer"_atm;
    bufInfo.size = 1;
    bufInfo.elementSize = sizeof(Terrain::TerrainRuntimeUniforms);
    bufInfo.mode = CoreGraphics::HostToDevice;
    bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
    terrainVirtualTileState.runtimeConstants = CoreGraphics::CreateBuffer(bufInfo);

    uint size = terrainState.settings.worldSizeX / SubTextureWorldSize;

    bufInfo.name = "SubTextureBuffer"_atm;
    bufInfo.size = size * size;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTexture);
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    terrainVirtualTileState.subTextureBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "SubTextureStagingBuffer"_atm;
    bufInfo.size = size * size;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTexture);
    bufInfo.mode = BufferAccessMode::HostLocal;
    bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
    terrainVirtualTileState.subtextureStagingBuffers.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < terrainVirtualTileState.subtextureStagingBuffers.Size(); i++)
        terrainVirtualTileState.subtextureStagingBuffers[i] = CoreGraphics::CreateBuffer(bufInfo);

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "IndirectionTexture"_atm;
    texInfo.width = IndirectionTextureSize;
    texInfo.height = IndirectionTextureSize;
    texInfo.format = CoreGraphics::PixelFormat::R32F;
    texInfo.usage = CoreGraphics::SampleTexture | CoreGraphics::TransferTextureDestination | CoreGraphics::TransferTextureSource;
    texInfo.mips = IndirectionNumMips;
    texInfo.clear = true;
    texInfo.clearColorU4 = Math::uint4{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    terrainVirtualTileState.indirectionTexture = CoreGraphics::CreateTexture(texInfo);

    // setup indirection texture upload buffer
    terrainVirtualTileState.indirectionUploadBuffers.Resize(CoreGraphics::GetNumBufferedFrames());
    for (IndexT i = 0; i < terrainVirtualTileState.indirectionUploadBuffers.Size(); i++)
    {
        SizeT bufferSize = IndirectionUpdateBufferSize;
        char* buf = n_new_array(char, bufferSize);
        memset(buf, 0xFF, bufferSize);

        CoreGraphics::BufferCreateInfo bufInfo;
        bufInfo.name = "IndirectionUploadBuffer"_atm;
        bufInfo.byteSize = IndirectionUpdateBufferSize;
        bufInfo.mode = CoreGraphics::HostLocal;
        bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
        bufInfo.data = buf;
        bufInfo.dataSize = bufferSize;
        terrainVirtualTileState.indirectionUploadBuffers[i] = CoreGraphics::CreateBuffer(bufInfo);

        n_delete_array(buf);
    }

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

    CoreGraphics::LockResourceSubmission();

    // clear the buffer, the subsequent fills are to clear the buffers
    CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();

    bufInfo.name = "PageStatusBuffer"_atm;
    bufInfo.elementSize = sizeof(uint);
    bufInfo.size = offset;
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    terrainVirtualTileState.pageStatusBuffer = CoreGraphics::CreateBuffer(bufInfo);

    {
        Util::FixedArray<uint> clearData;
        clearData.Resize(offset);
        clearData.Fill(0);
        bufInfo.name = "PageStatusClearBuffer"_atm;
        bufInfo.elementSize = sizeof(uint);
        bufInfo.size = offset;
        bufInfo.mode = BufferAccessMode::DeviceLocal;
        bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
        bufInfo.data = nullptr;
        bufInfo.dataSize = 0;
        terrainVirtualTileState.pageStatusClearBuffer = CoreGraphics::CreateBuffer(bufInfo);
    }

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
    bufInfo.mode = BufferAccessMode::DeviceToHost;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    terrainVirtualTileState.pageUpdateReadbackBuffers.Resize(GetNumBufferedFrames());
    for (IndexT i = 0; i < terrainVirtualTileState.pageUpdateReadbackBuffers.Size(); i++)
    {
        terrainVirtualTileState.pageUpdateReadbackBuffers[i] = CoreGraphics::CreateBuffer(bufInfo);
        CoreGraphics::BufferFill(terrainVirtualTileState.pageUpdateReadbackBuffers[i], 0x0, sub);
    }

    // we're done
    CoreGraphics::UnlockResourceSubmission();

    CoreGraphics::TextureCreateInfo albedoCacheInfo;
    albedoCacheInfo.name = "AlbedoPhysicalCache"_atm;
    albedoCacheInfo.width = PhysicalTexturePaddedSize;
    albedoCacheInfo.height = PhysicalTexturePaddedSize;
    albedoCacheInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
    albedoCacheInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.physicalAlbedoCache = CoreGraphics::CreateTexture(albedoCacheInfo);

    CoreGraphics::TextureCreateInfo normalCacheInfo;
    normalCacheInfo.name = "NormalPhysicalCache"_atm;
    normalCacheInfo.width = PhysicalTexturePaddedSize;
    normalCacheInfo.height = PhysicalTexturePaddedSize;
    normalCacheInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
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
    lowResAlbedoInfo.name = "AlbedoLowres"_atm;
    lowResAlbedoInfo.mips = LowresFallbackMips;
    lowResAlbedoInfo.width = LowresFallbackSize;
    lowResAlbedoInfo.height = LowresFallbackSize;
    lowResAlbedoInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
    lowResAlbedoInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.lowresAlbedo = CoreGraphics::CreateTexture(lowResAlbedoInfo);

    CoreGraphics::TextureCreateInfo lowResNormalInfo;
    lowResNormalInfo.name = "NormalLowres"_atm;
    lowResNormalInfo.mips = LowresFallbackMips;
    lowResNormalInfo.width = LowresFallbackSize;
    lowResNormalInfo.height = LowresFallbackSize;
    lowResNormalInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
    lowResNormalInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.lowresNormal = CoreGraphics::CreateTexture(lowResNormalInfo);

    CoreGraphics::TextureCreateInfo lowResMaterialInfo;
    lowResMaterialInfo.name = "MaterialLowres"_atm;
    lowResMaterialInfo.mips = LowresFallbackMips;
    lowResMaterialInfo.width = LowresFallbackSize;
    lowResMaterialInfo.height = LowresFallbackSize;
    lowResMaterialInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResMaterialInfo.usage = CoreGraphics::TextureUsage::RenderTexture;
    terrainVirtualTileState.lowresMaterial = CoreGraphics::CreateTexture(lowResMaterialInfo);

#ifdef CreateEvent
#undef CreateEvent
#endif
    CoreGraphics::EventCreateInfo eventInfo{ "Biome Update Finished Event"_atm, false, nullptr, nullptr, nullptr };
    terrainVirtualTileState.biomeUpdatedEvent = CoreGraphics::CreateEvent(eventInfo);

    // setup virtual sub textures buffer
    terrainVirtualTileState.subTexturesHistory.Resize(CoreGraphics::GetNumBufferedFrames());
    terrainVirtualTileState.subTextureUpdateKeysHistory.Resize(CoreGraphics::GetNumBufferedFrames());
    terrainVirtualTileState.subTextureJobOutputs.Resize(size * size);
    terrainVirtualTileState.subTextureJobOutputs.Fill({});
    terrainVirtualTileState.subTextureJobHistoryOutputs.Resize(CoreGraphics::GetNumBufferedFrames());
    terrainVirtualTileState.subTextures.Resize(size * size);
    terrainVirtualTileState.subTextureUpdateKeys.Resize(size * size);
        
    for (uint y = 0; y < size; y++)
    {
        for (uint x = 0; x < size; x++)
        {
            uint index = x + y * size;
            terrainVirtualTileState.subTextures[index].tiles = 0;
            terrainVirtualTileState.subTextures[index].maxMip = 0;
            terrainVirtualTileState.subTextures[index].indirectionOffset[0] = UINT32_MAX;
            terrainVirtualTileState.subTextures[index].indirectionOffset[1] = UINT32_MAX;

            // position is calculated as the center of each cell, offset by half of the world size (so we are oriented around 0)
            float xPos = x * SubTextureWorldSize - terrainState.settings.worldSizeX / 2.0f;
            float yPos = y * SubTextureWorldSize - terrainState.settings.worldSizeZ / 2.0f;
            terrainVirtualTileState.subTextures[index].worldCoordinate[0] = xPos;
            terrainVirtualTileState.subTextures[index].worldCoordinate[1] = yPos;
            terrainVirtualTileState.subTextureUpdateKeys[index] = 0;
        }
    }

    // setup indirection occupancy, the indirection texture is 2048, and the maximum allocation size is 256 indirection pixels
    terrainVirtualTileState.indirectionOccupancy.Setup(IndirectionTextureSize, 256, 1);

    // setup the physical texture occupancy, the texture is 8192x8192 pixels, and the page size is 256, so effectively make this a quad tree that ends at individual pages
    //terrainVirtualTileState.physicalTextureTileOccupancy.Setup(PhysicalTexturePaddedSize, PhysicalTexturePaddedSize, PhysicalTextureTilePaddedSize);
    terrainVirtualTileState.physicalTextureTileCache.Setup(PhysicalTextureTilePaddedSize, PhysicalTexturePaddedSize, 0x3FFF);

    ResourceTableSetConstantBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable, 
        {
            terrainState.systemConstants,
            ShaderGetResourceSlot(terrainState.terrainShader, "TerrainSystemUniforms"),
            0,
            false, false,
            NEBULA_WHOLE_BUFFER_SIZE,
            0
        });

    ResourceTableSetRWBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable,
        {
            terrainVirtualTileState.subTextureBuffer,
            ShaderGetResourceSlot(terrainState.terrainShader, "TerrainSubTexturesBuffer"),
            0, 
            false, false,
            NEBULA_WHOLE_BUFFER_SIZE,
            0
        });

    ResourceTableSetRWBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable,
        {
            terrainVirtualTileState.pageUpdateListBuffer,
            ShaderGetResourceSlot(terrainState.terrainShader, "PageUpdateListBuffer"),
            0,
            false, false,
            NEBULA_WHOLE_BUFFER_SIZE,
            0
        });

    ResourceTableSetRWBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable,
        {
            terrainVirtualTileState.pageStatusBuffer,
            ShaderGetResourceSlot(terrainState.terrainShader, "PageStatusBuffer"),
            0,
            false, false,
            NEBULA_WHOLE_BUFFER_SIZE,
            0
        });

    ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainSystemResourceTable);

    ResourceTableSetConstantBuffer(terrainVirtualTileState.virtualTerrainRuntimeResourceTable,
        {
            terrainVirtualTileState.runtimeConstants,
            ShaderGetResourceSlot(terrainState.terrainShader, "TerrainRuntimeUniforms"),
            0,
            false, false,
            sizeof(Terrain::TerrainRuntimeUniforms),
            0
        });
    ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainRuntimeResourceTable);

    ResourceTableSetConstantBuffer(terrainVirtualTileState.virtualTerrainDynamicResourceTable,
        {
            CoreGraphics::GetGraphicsConstantBuffer(MainThreadConstantBuffer),
            ShaderGetResourceSlot(terrainState.terrainShader, "TerrainTileUpdateUniforms"),
            0,
            false, true,
            sizeof(Terrain::TerrainTileUpdateUniforms),
            0
        });
    ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainDynamicResourceTable);

    // create pass for updating the physical cache tiles
    CoreGraphics::PassCreateInfo tileUpdatePassCreate;
    tileUpdatePassCreate.name = "TerrainVirtualTileUpdate";
    tileUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.physicalAlbedoCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalAlbedoCache) }));
    tileUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.physicalNormalCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalNormalCache) }));
    tileUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.physicalMaterialCache, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.physicalMaterialCache) }));
    AttachmentFlagBits bits = AttachmentFlagBits::Load | AttachmentFlagBits::Store;
    tileUpdatePassCreate.colorAttachmentFlags.AppendArray({ bits, bits, bits });
    tileUpdatePassCreate.colorAttachmentClears.AppendArray({ Math::vec4(0,0,0,0), Math::vec4(0,0,0,0), Math::vec4(0,0,0,0) });
    
    CoreGraphics::Subpass subpass;
    subpass.attachments.AppendArray({ 0, 1, 2 });
    subpass.bindDepth = false;
    subpass.numScissors = 3;
    subpass.numViewports = 3;

    tileUpdatePassCreate.subpasses.Append(subpass);
    terrainVirtualTileState.tileUpdatePass = CoreGraphics::CreatePass(tileUpdatePassCreate);

    // create pass for updating the fallback textures
    CoreGraphics::PassCreateInfo lowresUpdatePassCreate;
    lowresUpdatePassCreate.name = "TerrainVirtualLowresUpdate";
    lowresUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.lowresAlbedo, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresAlbedo) }));
    lowresUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.lowresNormal, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresNormal) }));
    lowresUpdatePassCreate.colorAttachments.Append(CreateTextureView({ terrainVirtualTileState.lowresMaterial, 0, 1, 0, 1, TextureGetPixelFormat(terrainVirtualTileState.lowresMaterial) }));
    bits = AttachmentFlagBits::Store;
    lowresUpdatePassCreate.colorAttachmentFlags.AppendArray({ bits, bits, bits });
    lowresUpdatePassCreate.colorAttachmentClears.AppendArray({ Math::vec4(0,0,0,0), Math::vec4(0,0,0,0), Math::vec4(0,0,0,0) });

    lowresUpdatePassCreate.subpasses.Append(subpass);
    terrainVirtualTileState.tileFallbackPass = CoreGraphics::CreatePass(lowresUpdatePassCreate);

    Frame::AddCallback("TerrainContext - Prepare", [](const IndexT frame, const IndexT bufferIndex)
        {
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_TRANSFER, "Terrain Prepare");

            BarrierPush(GraphicsQueueType,
                BarrierStage::PixelShader,
                BarrierStage::Transfer,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrier
                    {
                        terrainVirtualTileState.pageStatusBuffer,
                        BarrierAccess::ShaderRead,
                        BarrierAccess::TransferWrite,
                        0, NEBULA_WHOLE_BUFFER_SIZE
                    },
                });

            BufferCopy copy{ 0,0,0 };
            Copy(GraphicsQueueType,
                terrainVirtualTileState.pageStatusClearBuffer, { copy },
                terrainVirtualTileState.pageStatusBuffer, { copy },
                BufferGetByteSize(terrainVirtualTileState.pageStatusBuffer));

            BarrierPop(GraphicsQueueType);

            if (terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Size() > 0
                || terrainVirtualTileState.indirectionBufferShufflesThisFrame.Size() > 0
                || terrainVirtualTileState.indirectionTextureCopiesFromThisFrame.Size() > 0)
            {
                BarrierInsert(GraphicsQueueType,
                    BarrierStage::AllGraphicsShaders,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    {
                        TextureBarrier
                        {
                            terrainVirtualTileState.indirectionTexture,
                            ImageSubresourceInfo(ImageAspect::ColorBits, 0, TextureGetNumMips(terrainVirtualTileState.indirectionTexture), 0, 1),
                            ImageLayout::ShaderRead,
                            ImageLayout::TransferDestination,
                            BarrierAccess::ShaderRead,
                            BarrierAccess::TransferWrite,
                        },
                    },
                    nullptr,
                    "Terrain Prepare Barrier");

                BarrierInsert(GraphicsQueueType,
                    BarrierStage::Host,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrier
                        {
                            terrainVirtualTileState.indirectionUploadBuffers[bufferIndex],
                            BarrierAccess::HostWrite,
                            BarrierAccess::TransferRead,
                            0, NEBULA_WHOLE_BUFFER_SIZE
                        },
                    },
                    "Terrain Prepare Barrier");

                if (terrainVirtualTileState.indirectionTextureCopiesFromThisFrame.Size() > 0)
                {
                    Copy(GraphicsQueueType,
                        terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureCopiesFromThisFrame,
                        terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureCopiesToThisFrame);
                }

                BarrierInsert(GraphicsQueueType,
                    BarrierStage::Transfer,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    {
                        TextureBarrier
                        {
                            terrainVirtualTileState.indirectionTexture,
                            ImageSubresourceInfo(ImageAspect::ColorBits, 0, TextureGetNumMips(terrainVirtualTileState.indirectionTexture), 0, 1),
                            ImageLayout::TransferDestination,
                            ImageLayout::TransferDestination,
                            BarrierAccess::TransferWrite,
                            BarrierAccess::TransferWrite,
                        },
                    },
                    nullptr,
                    "Terrain Prepare Barrier");

                if (terrainVirtualTileState.indirectionBufferShufflesThisFrame.Size() > 0)
                {
                    // perform the moves and clears of indirection pixels
                    Copy(GraphicsQueueType,
                        terrainVirtualTileState.indirectionUploadBuffers[bufferIndex], terrainVirtualTileState.indirectionBufferShufflesThisFrame,
                        terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureShufflesThisFrame);
                }

                BarrierInsert(GraphicsQueueType,
                    BarrierStage::Transfer,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    {
                        TextureBarrier
                        {
                            terrainVirtualTileState.indirectionTexture,
                            ImageSubresourceInfo(ImageAspect::ColorBits, 0, TextureGetNumMips(terrainVirtualTileState.indirectionTexture), 0, 1),
                            ImageLayout::TransferDestination,
                            ImageLayout::TransferDestination,
                            BarrierAccess::TransferWrite,
                            BarrierAccess::TransferWrite,
                        },
                    },
                    nullptr,
                    "Terrain Prepare Barrier");

                if (terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Size() > 0)
                {
                    // update the new pixels
                    Copy(GraphicsQueueType,
                        terrainVirtualTileState.indirectionUploadBuffers[bufferIndex], terrainVirtualTileState.indirectionBufferUpdatesThisFrame,
                        terrainVirtualTileState.indirectionTexture, terrainVirtualTileState.indirectionTextureUpdatesThisFrame);
                }

                BarrierInsert(GraphicsQueueType,
                    BarrierStage::Transfer,
                    BarrierStage::AllGraphicsShaders,
                    BarrierDomain::Global,
                    {
                        TextureBarrier
                        {
                            terrainVirtualTileState.indirectionTexture,
                            ImageSubresourceInfo(ImageAspect::ColorBits, 0, TextureGetNumMips(terrainVirtualTileState.indirectionTexture), 0, 1),
                            ImageLayout::TransferDestination,
                            ImageLayout::ShaderRead,
                            BarrierAccess::TransferWrite,
                            BarrierAccess::ShaderRead,
                        },
                    },
                    nullptr,
                    "Terrain Prepare Barrier");

                BarrierInsert(GraphicsQueueType,
                    BarrierStage::Transfer,
                    BarrierStage::Host,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrier
                        {
                            terrainVirtualTileState.indirectionUploadBuffers[bufferIndex],
                            BarrierAccess::TransferRead,
                            BarrierAccess::HostWrite,
                            0, NEBULA_WHOLE_BUFFER_SIZE
                        },
                    },
                    "Terrain Prepare Barrier");

                terrainVirtualTileState.indirectionTextureCopiesFromThisFrame.Clear();
                terrainVirtualTileState.indirectionTextureCopiesToThisFrame.Clear();

                terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Clear();
                terrainVirtualTileState.indirectionTextureUpdatesThisFrame.Clear();

                terrainVirtualTileState.indirectionBufferShufflesThisFrame.Clear();
                terrainVirtualTileState.indirectionTextureShufflesThisFrame.Clear();
            }

            if (terrainVirtualTileState.virtualSubtextureBufferUpdate)
            {
                BarrierPush(GraphicsQueueType,
                    BarrierStage::Host,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrier
                        {
                            terrainVirtualTileState.subtextureStagingBuffers[bufferIndex],
                            BarrierAccess::HostWrite,
                            BarrierAccess::TransferRead,
                            0, NEBULA_WHOLE_BUFFER_SIZE
                        },
                    });

                BarrierPush(GraphicsQueueType,
                    BarrierStage::AllGraphicsShaders,
                    BarrierStage::Transfer,
                    BarrierDomain::Global,
                    nullptr,
                    {
                        BufferBarrier
                        {
                            terrainVirtualTileState.subTextureBuffer,
                            BarrierAccess::ShaderRead,
                            BarrierAccess::TransferWrite,
                            0, NEBULA_WHOLE_BUFFER_SIZE
                        },
                    });

                BufferCopy from, to;
                from.offset = 0;
                to.offset = 0;
                Copy(GraphicsQueueType
                    , terrainVirtualTileState.subtextureStagingBuffers[bufferIndex], { from }
                    , terrainVirtualTileState.subTextureBuffer, { to }
                    , BufferGetByteSize(terrainVirtualTileState.subTextureBuffer));

                BarrierPop(GraphicsQueueType);
                BarrierPop(GraphicsQueueType);

                terrainVirtualTileState.virtualSubtextureBufferUpdate = false;
            }           

            CommandBufferEndMarker(GraphicsQueueType);
        });

    Frame::AddCallback("TerrainContext - Clear Page Update Buffer", [](const IndexT frame, const IndexT bufferIndex)
        {
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Clear Page Entries Buffer");

            BarrierInsert(GraphicsQueueType,
                BarrierStage::Transfer,
                BarrierStage::ComputeShader,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrier
                    {
                        terrainVirtualTileState.pageUpdateListBuffer,
                        BarrierAccess::TransferRead,
                        BarrierAccess::ShaderWrite,
                        0, NEBULA_WHOLE_BUFFER_SIZE
                    },
                },
                "Page Readback Buffer Barrier");

            SetShaderProgram(terrainVirtualTileState.terrainPageClearUpdateBufferProgram);
            SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

            // run a single compute shader to clear the number of page entries
            Compute(1, 1, 1);

            BarrierInsert(GraphicsQueueType,
                BarrierStage::ComputeShader,
                BarrierStage::ComputeShader,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrier
                    {
                        terrainVirtualTileState.pageUpdateListBuffer,
                        BarrierAccess::ShaderWrite,
                        BarrierAccess::ShaderWrite,
                        0, NEBULA_WHOLE_BUFFER_SIZE
                    },
                },
                "Page Readback Buffer Barrier");

            CommandBufferEndMarker(GraphicsQueueType);
        });

    Frame::AddCallback("TerrainContext - Render Terrain Prepass", [](const IndexT frame, const IndexT bufferIndex)
        {
            if (terrainState.renderToggle == false)
                return;

            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

            // setup shader state, set shader before we set the vertex layout
            SetShaderProgram(terrainVirtualTileState.terrainPrepassProgram);
            SetVertexLayout(terrainState.vlo);
            SetPrimitiveTopology(PrimitiveTopology::PatchList);
            SetGraphicsPipeline();

            // set shared resources
            SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, "Terrain Sections");

            // go through and render terrain instances
            for (IndexT i = 0; i < runtimes.Size(); i++)
            {
                TerrainRuntimeInfo& rt = runtimes[i];
                SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
                {
                    if (rt.sectorVisible[j])
                    {
                        SetStreamVertexBuffer(0, rt.vbo, 0);
                        SetIndexBuffer(rt.ibo, 0);
                        SetPrimitiveGroup(rt.sectorPrimGroups[j]);
                        SetResourceTable(rt.patchTable, NEBULA_DYNAMIC_OFFSET_GROUP, GraphicsPipeline, 2, &rt.sectorUniformOffsets[j][0]);
                        Draw();
                    }
                }
            }

            CommandBufferEndMarker(GraphicsQueueType);
        });

    Frame::AddCallback("TerrainContext - Extract Readback Data Buffer", [](const IndexT frame, const IndexT bufferIndex)
        {
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_TRANSFER, "Terrain Page Update List Readback Copy");
            BarrierPush(GraphicsQueueType,
                BarrierStage::PixelShader,
                BarrierStage::Transfer,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrier
                    {
                        terrainVirtualTileState.pageUpdateListBuffer,
                        BarrierAccess::ShaderWrite,
                        BarrierAccess::TransferRead,
                        0, NEBULA_WHOLE_BUFFER_SIZE
                    },
                });

            BarrierPush(GraphicsQueueType,
                BarrierStage::Host,
                BarrierStage::Transfer,
                BarrierDomain::Global,
                nullptr,
                {
                    BufferBarrier
                    {
                        terrainVirtualTileState.pageUpdateReadbackBuffers[bufferIndex],
                        BarrierAccess::HostRead,
                        BarrierAccess::TransferWrite,
                        0, NEBULA_WHOLE_BUFFER_SIZE
                    },
                });

            CoreGraphics::BufferCopy from, to;
            from.offset = 0;
            to.offset = 0;
            Copy(
                GraphicsQueueType,
                terrainVirtualTileState.pageUpdateListBuffer, { from }, terrainVirtualTileState.pageUpdateReadbackBuffers[bufferIndex], { to },
                sizeof(Terrain::PageUpdateList)
            );

            BarrierPop(GraphicsQueueType);
            BarrierPop(GraphicsQueueType);

            CommandBufferEndMarker(GraphicsQueueType);
        });

        Frame::AddCallback("TerrainContext - Update Physical Texture Cache", [](const IndexT frame, const IndexT bufferIndex)
            {
                if (terrainVirtualTileState.updateLowres && CoreGraphics::EventPoll(terrainVirtualTileState.biomeUpdatedEvent))
                {
                    CoreGraphics::EventHostReset(terrainVirtualTileState.biomeUpdatedEvent);
                    terrainVirtualTileState.updateLowres = false;

                    // begin the lowres update
                    CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Terrain Lowres Update");

                    PassBegin(terrainVirtualTileState.tileFallbackPass, PassRecordMode::ExecuteInline);

                    // setup state for update
                    SetShaderProgram(terrainVirtualTileState.terrainTileFallbackProgram);
                    SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                    SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                    Math::rectangle<int> rect;
                    rect.left = 0;
                    rect.top = 0;
                    rect.right = TextureGetDimensions(terrainVirtualTileState.lowresAlbedo).width;
                    rect.bottom = TextureGetDimensions(terrainVirtualTileState.lowresAlbedo).height;
                    CoreGraphics::SetViewport(rect, 0);
                    CoreGraphics::SetViewport(rect, 1);
                    CoreGraphics::SetViewport(rect, 2);
                    CoreGraphics::SetScissorRect(rect, 0);
                    CoreGraphics::SetScissorRect(rect, 1);
                    CoreGraphics::SetScissorRect(rect, 2);

                    // update textures
                    RenderUtil::DrawFullScreenQuad::ApplyMesh();
                    Draw();

                    PassEnd(terrainVirtualTileState.tileFallbackPass);
                    CommandBufferEndMarker(GraphicsQueueType);

                    CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_TRANSFER, "Terrain Lowres Mipmap");

                    // generate mipmaps for all the lowres textures
                    CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresAlbedo);
                    CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresNormal);
                    CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresMaterial);

                    CommandBufferEndMarker(GraphicsQueueType);
                }

                if (terrainVirtualTileState.pageUpdatesThisFrame.Size() > 0)
                {

                    // update physical cache
                    CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Terrain Update Physical Texture Cache");

                    // start the pass
                    PassBegin(terrainVirtualTileState.tileUpdatePass, PassRecordMode::ExecuteInline);

                    SetShaderProgram(terrainVirtualTileState.terrainTileUpdateProgram);

                    // apply the mesh for all quads
                    RenderUtil::DrawFullScreenQuad::ApplyMesh();
                    SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
                    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

                    // go through pending page updates and render into the physical texture caches
                    for (IndexT i = 0; i < runtimes.Size(); i++)
                    {
                        TerrainRuntimeInfo& rt = runtimes[i];
                        SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

                        for (int j = 0; j < terrainVirtualTileState.pageUpdatesThisFrame.Size(); j++)
                        {
                            PhysicalPageUpdate& pageUpdate = terrainVirtualTileState.pageUpdatesThisFrame[j];
                            SetResourceTable(terrainVirtualTileState.virtualTerrainDynamicResourceTable, NEBULA_DYNAMIC_OFFSET_GROUP, GraphicsPipeline, 2, pageUpdate.constantBufferOffsets);

                            // update viewport rectangle
                            Math::rectangle<int> rect;
                            rect.left = pageUpdate.tileOffset[0];
                            rect.top = pageUpdate.tileOffset[1];
                            rect.right = rect.left + PhysicalTextureTilePaddedSize;
                            rect.bottom = rect.top + PhysicalTextureTilePaddedSize;
                            CoreGraphics::SetViewport(rect, 0);
                            CoreGraphics::SetViewport(rect, 1);
                            CoreGraphics::SetViewport(rect, 2);
                            CoreGraphics::SetScissorRect(rect, 0);
                            CoreGraphics::SetScissorRect(rect, 1);
                            CoreGraphics::SetScissorRect(rect, 2);

                            // draw tile
                            Draw();
                        }
                        terrainVirtualTileState.pageUpdatesThisFrame.Clear();
                    }

                    PassEnd(terrainVirtualTileState.tileUpdatePass);

                    // we need a barrier here for the tile updates since we are not using the framescript and thus don't get any automatic barriers
                    BarrierInsert(GraphicsQueueType,
                        BarrierStage::PassOutput,
                        BarrierStage::AllGraphicsShaders,
                        BarrierDomain::Global,
                        {
                            TextureBarrier
                            {
                                terrainVirtualTileState.physicalAlbedoCache,
                                ImageSubresourceInfo::ColorNoMipNoLayer(),
                                ImageLayout::ColorRenderTexture,
                                ImageLayout::ShaderRead,
                                BarrierAccess::ColorAttachmentWrite,
                                BarrierAccess::ShaderRead,
                            },
                            TextureBarrier
                            {
                                terrainVirtualTileState.physicalNormalCache,
                                ImageSubresourceInfo::ColorNoMipNoLayer(),
                                ImageLayout::ColorRenderTexture,
                                ImageLayout::ShaderRead,
                                BarrierAccess::ColorAttachmentWrite,
                                BarrierAccess::ShaderRead,
                            },
                            TextureBarrier
                            {
                                terrainVirtualTileState.physicalMaterialCache,
                                ImageSubresourceInfo::ColorNoMipNoLayer(),
                                ImageLayout::ColorRenderTexture,
                                ImageLayout::ShaderRead,
                                BarrierAccess::ColorAttachmentWrite,
                                BarrierAccess::ShaderRead,
                            },
                        },
                        nullptr,
                        "Terrain Physical Cache Update Barrier");
                    
                    CommandBufferEndMarker(GraphicsQueueType);
                }
            });

    Frame::AddCallback("TerrainContext - Screen Space Resolve", [](const IndexT frame, const IndexT bufferIndex)
        {
            CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Terrain Screenspace Pass");
            SetShaderProgram(terrainVirtualTileState.terrainScreenspacePass);

            CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
            RenderUtil::DrawFullScreenQuad::ApplyMesh();
            SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
            SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);
            CoreGraphics::Draw();
            CoreGraphics::EndBatch();

            CommandBufferEndMarker(GraphicsQueueType);
        });

    // create vlo
    terrainState.vlo = CreateVertexLayout({ terrainState.components });
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
    DestroyBuffer(terrainVirtualTileState.pageStatusClearBuffer);
    DestroyBuffer(terrainVirtualTileState.subTextureBuffer);
    for (CoreGraphics::BufferId id : terrainVirtualTileState.subtextureStagingBuffers)
        DestroyBuffer(id);
    DestroyBuffer(terrainVirtualTileState.pageUpdateListBuffer);
    for (CoreGraphics::BufferId id : terrainVirtualTileState.pageUpdateReadbackBuffers)
        DestroyBuffer(id);
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
    const Resources::ResourceName& decisionMap,
    const Resources::ResourceName& albedoMap)
{
    n_assert(terrainState.settings.worldSizeX > 0);
    n_assert(terrainState.settings.worldSizeZ > 0);
    using namespace CoreGraphics;
    const Graphics::ContextEntityId cid = GetContextId(entity);
    TerrainLoadInfo& loadInfo = terrainAllocator.Get<Terrain_LoadInfo>(cid.id);
    TerrainRuntimeInfo& runtimeInfo = terrainAllocator.Get<Terrain_RuntimeInfo>(cid.id);

    runtimeInfo.decisionMap = Resources::CreateResource(decisionMap, "terrain"_atm, nullptr, nullptr, true);
    runtimeInfo.heightMap = Resources::CreateResource(heightMap, "terrain"_atm, nullptr, nullptr, true);
    runtimeInfo.lowResAlbedoMap = Resources::CreateResource(albedoMap, "terrain"_atm, nullptr, nullptr, true);
    Resources::SetMaxLOD(runtimeInfo.decisionMap, 0.0f, false);
    Resources::SetMaxLOD(runtimeInfo.heightMap, 0.0f, false);
    Resources::SetMaxLOD(runtimeInfo.lowResAlbedoMap, 0.0f, false);

    //runtimeInfo.heightSource.Setup(heightMap);
    //runtimeInfo.albedoSource.Setup(albedoMap);

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
    SizeT numVertsX = terrainState.settings.tileWidth / terrainState.settings.vertexDensityX + 1;
    SizeT numVertsY = terrainState.settings.tileHeight / terrainState.settings.vertexDensityY + 1;
    SizeT vertDistanceX = terrainState.settings.vertexDensityX;
    SizeT vertDistanceY = terrainState.settings.vertexDensityY;
    SizeT numBufferedFrame = CoreGraphics::GetNumBufferedFrames();

    // setup resource tables, one for the per-chunk draw arguments, and one for the whole terrain 
    runtimeInfo.patchTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);
    runtimeInfo.terrainConstants = ShaderCreateConstantBuffer(terrainState.terrainShader, "TerrainRuntimeUniforms");
    runtimeInfo.terrainResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
    IndexT constantsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TerrainRuntimeUniforms");
    IndexT feedbackSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TextureLODReadback");

    IndexT slot = ShaderGetResourceSlot(terrainState.terrainShader, "PatchUniforms");
    ResourceTableSetConstantBuffer(runtimeInfo.terrainResourceTable, { runtimeInfo.terrainConstants, constantsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
    ResourceTableSetConstantBuffer(runtimeInfo.patchTable, { CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::MainThreadConstantBuffer), slot, 0, false, true, sizeof(Terrain::PatchUniforms), 0 });
    ResourceTableCommitChanges(runtimeInfo.patchTable);
    ResourceTableCommitChanges(runtimeInfo.terrainResourceTable);

    // allocate a tile vertex buffer
    Util::FixedArray<TerrainVert> verts(numVertsX * numVertsY);

    // allocate terrain index buffer, every fourth pixel will generate two triangles 
    SizeT numTris = numVertsX * numVertsY;
    Util::FixedArray<TerrainQuad> quads(numTris);

    // setup sections
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

            CoreGraphics::PrimitiveGroup group;
            group.SetBaseIndex(0);
            group.SetBaseVertex(0);
            group.SetBoundingBox(box);
            group.SetNumIndices(numTris * 4);
            group.SetNumVertices(0);
            runtimeInfo.sectorPrimGroups.Append(group);
        }
    }

    // walk through and set up sections, oriented around origo, so half of the sections are in the negative
    for (IndexT y = 0; y < numVertsY; y++)
    {
        for (IndexT x = 0; x < numVertsX; x++)
        {
            if (x == numVertsX - 1)
                continue;
            if (y == numVertsY - 1)
                continue;

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
            vt1.tileUv.x = x / (float)(numVertsX - 1);
            vt1.tileUv.y = y / (float)(numVertsY - 1);

            TerrainVert& vt2 = verts[vidx2];
            v2.position.storeu3(vt2.position.v);
            vt2.uv.x = v2.uv.x;
            vt2.uv.y = v2.uv.y;
            vt2.tileUv.x = x / (float)(numVertsX - 1) + 1.0f / (numVertsX - 1);
            vt2.tileUv.y = y / (float)(numVertsY - 1);

            TerrainVert& vt3 = verts[vidx3];
            v3.position.storeu3(vt3.position.v);
            vt3.uv.x = v3.uv.x;
            vt3.uv.y = v3.uv.y;
            vt3.tileUv.x = x / (float)(numVertsX - 1);
            vt3.tileUv.y = y / (float)(numVertsY - 1) + 1.0f / (numVertsY - 1);

            TerrainVert& vt4 = verts[vidx4];
            v4.position.storeu3(vt4.position.v);
            vt4.uv.x = v4.uv.x;
            vt4.uv.y = v4.uv.y;
            vt4.tileUv.x = x / (float)(numVertsX - 1) + 1.0f / (numVertsX - 1);
            vt4.tileUv.y = y / (float)(numVertsY - 1) + 1.0f / (numVertsX - 1);

            // setup triangle tris
            TerrainQuad& q1 = quads[vidx1];
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
    vboInfo.dataSize = verts.ByteSize();;
    runtimeInfo.vbo = CreateBuffer(vboInfo);

    // create ibo
    BufferCreateInfo iboInfo;
    iboInfo.name = "terrain_ibo"_atm;
    iboInfo.size = numTris * 4;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
    iboInfo.mode = CoreGraphics::HostToDevice;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = quads.Begin();
    iboInfo.dataSize = quads.ByteSize();
    runtimeInfo.ibo = CreateBuffer(iboInfo);
}

//------------------------------------------------------------------------------
/**
*/
TerrainBiomeId 
TerrainContext::CreateBiome(
    BiomeSetupSettings settings, 
    BiomeMaterial flatMaterial, 
    BiomeMaterial slopeMaterial, 
    BiomeMaterial heightMaterial, 
    BiomeMaterial heightSlopeMaterial,
    const Resources::ResourceName& mask)
{
    Ids::Id32 ret = terrainBiomeAllocator.Alloc();
    terrainBiomeAllocator.Set<TerrainBiome_Settings>(ret, settings);

    // setup texture arrays
    CoreGraphics::TextureCreateInfo arrayTexInfo;
    arrayTexInfo.width = 2048;
    arrayTexInfo.height = 2048;
    arrayTexInfo.bindless = false;
    arrayTexInfo.type = CoreGraphics::Texture2DArray;
    arrayTexInfo.mips = CoreGraphics::TextureAutoMips;  // let coregraphics figure out how many mips this is
    arrayTexInfo.layers = 4;                                // use 4 layers, one for each material

    // create our textures
    arrayTexInfo.format = CoreGraphics::PixelFormat::BC7sRGB;
    arrayTexInfo.name = "BiomeAlbedo";
    terrainState.biomeAlbedoArray[terrainState.biomeCounter] = CoreGraphics::CreateTexture(arrayTexInfo);
    arrayTexInfo.name = "BiomeNormal";
    arrayTexInfo.format = CoreGraphics::PixelFormat::BC5;
    terrainState.biomeNormalArray[terrainState.biomeCounter] = CoreGraphics::CreateTexture(arrayTexInfo);
    arrayTexInfo.name = "BiomeMaterial";
    arrayTexInfo.format = CoreGraphics::PixelFormat::BC7;
    terrainState.biomeMaterialArray[terrainState.biomeCounter] = CoreGraphics::CreateTexture(arrayTexInfo);

    // lock the handover submission because it's on the graphics queue
    CoreGraphics::LockResourceSubmission();
    CoreGraphics::SubmissionContextId sub = CoreGraphics::GetHandoverSubmissionContext();

    // insert barrier before starting our blits
    CoreGraphics::BarrierInsert(
        CoreGraphics::SubmissionContextGetCmdBuffer(sub),
        CoreGraphics::BarrierStage::AllGraphicsShaders,
        CoreGraphics::BarrierStage::Transfer,
        CoreGraphics::BarrierDomain::Global,
        {
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeAlbedoArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeAlbedoArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::BarrierAccess::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
            },
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeNormalArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeNormalArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::BarrierAccess::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
            },
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeMaterialArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeMaterialArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::BarrierAccess::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
            }
        },
        nullptr,
        nullptr);

    Util::Array<BiomeMaterial> mats = { flatMaterial, slopeMaterial, heightMaterial, heightSlopeMaterial };
    for (int i = 0; i < mats.Size(); i++)
    {
        {
            CoreGraphics::TextureId albedo = Resources::CreateResource(mats[i].albedo.Value(), "terrain", nullptr, nullptr, true);
            SizeT mips = CoreGraphics::TextureGetNumMips(albedo);
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(albedo);

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        albedo,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(albedo), 0, 1 },
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::BarrierAccess::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                    },
                },
                nullptr,
                nullptr);

            // now copy this texture into the texture array, and make sure it downsamples properly
            for (int j = 0; j < mips; j++)
            {
                Math::rectangle<int> to;
                to.left = 0;
                to.top = 0;
                to.right = Math::n_max(1, (int)arrayTexInfo.width >> j);
                to.bottom = Math::n_max(1, (int)arrayTexInfo.height >> j);
                Math::rectangle<int> from;
                from.left = 0;
                from.top = 0;
                from.right = Math::n_max(1, (int)dims.width >> j);
                from.bottom = Math::n_max(1, (int)dims.height >> j);

                // copy data over
                CoreGraphics::TextureCopy fromTex, toTex;
                fromTex.layer = 0;
                fromTex.mip = j;
                fromTex.region = from;
                toTex.layer = i;
                toTex.mip = j;
                toTex.region = to;
                CoreGraphics::Copy(CoreGraphics::InvalidQueueType, albedo, { fromTex }, terrainState.biomeAlbedoArray[terrainState.biomeCounter], { toTex }, sub);
            }

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        albedo,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(albedo), 0, 1 },
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                        CoreGraphics::BarrierAccess::ShaderRead,
                    },
                },
                nullptr,
                nullptr);

            // destroy the texture
            CoreGraphics::SubmissionContextFreeResource(sub, albedo);
        }

        {
            CoreGraphics::TextureId normal = Resources::CreateResource(mats[i].normal.Value(), "terrain", nullptr, nullptr, true);
            SizeT mips = CoreGraphics::TextureGetNumMips(normal);
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(normal);

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        normal,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(normal), 0, 1 },
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::BarrierAccess::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                    },
                },
                nullptr,
                nullptr);

            // now copy this texture into the texture array, and make sure it downsamples properly
            for (int j = 0; j < mips; j++)
            {
                Math::rectangle<int> to;
                to.left = 0;
                to.top = 0;
                to.right = Math::n_max(1, (int)arrayTexInfo.width >> j);
                to.bottom = Math::n_max(1, (int)arrayTexInfo.height >> j);
                Math::rectangle<int> from;
                from.left = 0;
                from.top = 0;
                from.right = Math::n_max(1, (int)dims.width >> j);
                from.bottom = Math::n_max(1, (int)dims.height >> j);

                // copy data over
                CoreGraphics::TextureCopy fromTex, toTex;
                fromTex.layer = 0;
                fromTex.mip = j;
                fromTex.region = from;
                toTex.layer = i;
                toTex.mip = j;
                toTex.region = to;
                CoreGraphics::Copy(CoreGraphics::InvalidQueueType, normal, { fromTex }, terrainState.biomeNormalArray[terrainState.biomeCounter], { toTex }, sub);
            }

            // signal event
            CoreGraphics::EventSignal(terrainVirtualTileState.biomeUpdatedEvent, CoreGraphics::SubmissionContextGetCmdBuffer(sub), CoreGraphics::BarrierStage::Transfer);

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        normal,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(normal), 0, 1 },
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                        CoreGraphics::BarrierAccess::ShaderRead,
                    },
                },
                nullptr,
                nullptr);

            // destroy the texture
            CoreGraphics::SubmissionContextFreeResource(sub, normal);
        }

        {
            CoreGraphics::TextureId material = Resources::CreateResource(mats[i].material.Value(), "terrain", nullptr, nullptr, true);
            SizeT mips = CoreGraphics::TextureGetNumMips(material);
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(material);

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        material,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(material), 0, 1 },
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::BarrierAccess::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                    },
                },
                nullptr,
                nullptr);

            // now copy this texture into the texture array, and make sure it downsamples properly
            for (int j = 0; j < mips; j++)
            {
                Math::rectangle<int> to;
                to.left = 0;
                to.top = 0;
                to.right = Math::n_max(1, (int)arrayTexInfo.width >> j);
                to.bottom = Math::n_max(1, (int)arrayTexInfo.height >> j);
                Math::rectangle<int> from;
                from.left = 0;
                from.top = 0;
                from.right = Math::n_max(1, (int)dims.width >> j);
                from.bottom = Math::n_max(1, (int)dims.height >> j);

                // copy data over
                CoreGraphics::TextureCopy fromTex, toTex;
                fromTex.layer = 0;
                fromTex.mip = j;
                fromTex.region = from;
                toTex.layer = i;
                toTex.mip = j;
                toTex.region = to;
                CoreGraphics::Copy(CoreGraphics::InvalidQueueType, material, { fromTex }, terrainState.biomeMaterialArray[terrainState.biomeCounter], { toTex }, sub);
            }

            CoreGraphics::BarrierInsert(
                CoreGraphics::SubmissionContextGetCmdBuffer(sub),
                CoreGraphics::BarrierStage::Transfer,
                CoreGraphics::BarrierStage::AllGraphicsShaders,
                CoreGraphics::BarrierDomain::Global,
                {
                    CoreGraphics::TextureBarrier
                    {
                        material,
                        CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(material), 0, 1 },
                        CoreGraphics::ImageLayout::TransferSource,
                        CoreGraphics::ImageLayout::ShaderRead,
                        CoreGraphics::BarrierAccess::TransferRead,
                        CoreGraphics::BarrierAccess::ShaderRead,
                    },
                },
                nullptr,
                nullptr);

            // destroy the texture
            CoreGraphics::SubmissionContextFreeResource(sub, material);
        }
    }

    // insert barrier before starting our blits
    CoreGraphics::BarrierInsert(
        CoreGraphics::SubmissionContextGetCmdBuffer(sub),
        CoreGraphics::BarrierStage::Transfer,
        CoreGraphics::BarrierStage::AllGraphicsShaders,
        CoreGraphics::BarrierDomain::Global,
        {
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeAlbedoArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeAlbedoArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
                CoreGraphics::BarrierAccess::ShaderRead,
            },
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeNormalArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeNormalArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
                CoreGraphics::BarrierAccess::ShaderRead,
            },
            CoreGraphics::TextureBarrier
            {
                terrainState.biomeMaterialArray[terrainState.biomeCounter],
                CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, (uint)CoreGraphics::TextureGetNumMips(terrainState.biomeMaterialArray[terrainState.biomeCounter]), 0, 4 },
                CoreGraphics::ImageLayout::TransferDestination,
                CoreGraphics::ImageLayout::ShaderRead,
                CoreGraphics::BarrierAccess::TransferWrite,
                CoreGraphics::BarrierAccess::ShaderRead,
            }
        },
        nullptr,
        nullptr);

    CoreGraphics::UnlockResourceSubmission();

    terrainState.biomeMasks[terrainState.biomeCounter] = Resources::CreateResource(mask, "terrain", nullptr, nullptr, true);

    IndexT albedoSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialAlbedoArray");
    IndexT normalsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialNormalArray");
    IndexT materialsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialPBRArray");
    IndexT maskSlot = ShaderGetResourceSlot(terrainState.terrainShader, "MaterialMaskArray");

    ResourceTableSetTexture(terrainState.resourceTable, { terrainState.biomeAlbedoArray[terrainState.biomeCounter], albedoSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainState.resourceTable, { terrainState.biomeNormalArray[terrainState.biomeCounter], normalsSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainState.resourceTable, { terrainState.biomeMaterialArray[terrainState.biomeCounter], materialsSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainState.resourceTable, { terrainState.biomeMasks[terrainState.biomeCounter], maskSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableCommitChanges(terrainState.resourceTable);

    ResourceTableSetTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable, { terrainState.biomeAlbedoArray[terrainState.biomeCounter], albedoSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable, { terrainState.biomeNormalArray[terrainState.biomeCounter], normalsSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable, { terrainState.biomeMaterialArray[terrainState.biomeCounter], materialsSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableSetTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable, { terrainState.biomeMasks[terrainState.biomeCounter], maskSlot, terrainState.biomeCounter, CoreGraphics::SamplerId::Invalid(), false, false });
    ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainSystemResourceTable);

    terrainBiomeAllocator.Set<TerrainBiome_Index>(ret, terrainState.biomeCounter);
    terrainState.biomeCounter++;

    // trigger lowres update
    terrainVirtualTileState.updateLowres = true;

    return TerrainBiomeId(ret);
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetBiomeSlopeThreshold(TerrainBiomeId id, float threshold)
{
    terrainBiomeAllocator.Get<TerrainBiome_Settings>(id.id).slopeThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetBiomeHeightThreshold(TerrainBiomeId id, float threshold)
{
    terrainBiomeAllocator.Get<TerrainBiome_Settings>(id.id).heightThreshold = threshold;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::CullPatches(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(CullPatches, Terrain);
    static Math::mat4 cameraTransform;
    cameraTransform = Math::inverse(Graphics::CameraContext::GetTransform(view->GetCamera()));

    {
        Jobs::JobContext jctx;

        static SubTextureUpdateJobUniforms uniforms;
        uniforms.maxMip = IndirectionNumMips - 1;
        uniforms.physicalTileSize = PhysicalTextureTileSize;
        uniforms.subTextureWorldSize = SubTextureWorldSize;
        terrainVirtualTileState.subTextureJobOutputs.Fill({});

        // uniform data is the observer transform
        jctx.uniform.numBuffers = 2;
        jctx.uniform.scratchSize = 0;

        jctx.uniform.data[0] = (unsigned char*)&cameraTransform;
        jctx.uniform.dataSize[0] = sizeof(Math::mat4);

        jctx.uniform.data[1] = (unsigned char*)&uniforms;
        jctx.uniform.dataSize[1] = sizeof(SubTextureUpdateJobUniforms);

        // just one input buffer of all the transforms
        jctx.input.numBuffers = 1;
        jctx.input.data[0] = (unsigned char*)terrainVirtualTileState.subTextures.Begin();
        jctx.input.dataSize[0] = sizeof(Terrain::TerrainSubTexture) * terrainVirtualTileState.subTextures.Size();
        jctx.input.sliceSize[0] = sizeof(Terrain::TerrainSubTexture);

        jctx.output.numBuffers = 1;
        jctx.output.data[0] = (unsigned char*)terrainVirtualTileState.subTextureJobOutputs.Begin();
        jctx.output.dataSize[0] = sizeof(Terrain::SubTextureUpdateJobOutput) * terrainVirtualTileState.subTextureJobOutputs.Size();
        jctx.output.sliceSize[0] = sizeof(Terrain::SubTextureUpdateJobOutput);

        Jobs::JobId job = Jobs::CreateJob({ TerrainSubTextureUpdateJob });
        Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, jctx);

        TerrainContext::runningJobs.Enqueue(job);
    }

    const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
    for (IndexT i = 0; i < runtimes.Size(); i++)
    {
        TerrainRuntimeInfo& rt = runtimes[i];

        // start job for sections
        Jobs::JobContext jctx;

        // uniform data is the observer transform
        jctx.uniform.numBuffers = 1;
        jctx.uniform.data[0] = (unsigned char*)&viewProj;
        jctx.uniform.dataSize[0] = sizeof(Math::mat4);
        jctx.uniform.scratchSize = 0;

        // just one input buffer of all the transforms
        jctx.input.numBuffers = 1;
        jctx.input.data[0] = (unsigned char*)rt.sectionBoxes.Begin();
        jctx.input.dataSize[0] = sizeof(Math::bbox) * rt.sectionBoxes.Size();
        jctx.input.sliceSize[0] = sizeof(Math::bbox);

        // the output is the visibility result
        jctx.output.numBuffers = 1;
        jctx.output.data[0] = (unsigned char*)rt.sectorVisible.Begin();
        jctx.output.dataSize[0] = sizeof(bool) * rt.sectorVisible.Size();
        jctx.output.sliceSize[0] = sizeof(bool);

        // create and run job
        Jobs::JobId job = Jobs::CreateJob({ TerrainCullJob });
        Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, jctx);

        TerrainContext::runningJobs.Enqueue(job);
    }

    // signal sync event
    Jobs::JobSyncSignal(TerrainContext::jobHostSync, Graphics::GraphicsServer::renderSystemsJobPort);
}


//------------------------------------------------------------------------------
/**
    Unpack from packed ushort vectors to full size
*/
void
UnpackPageDataEntry(uint* packed, uint& status, uint& subTextureIndex, uint& mip, uint& pageCoordX, uint& pageCoordY, uint& subTextureTileX, uint& subTextureTileY)
{
    status = packed[0] & 0x3;
    subTextureIndex = packed[0] >> 2;
    mip = packed[1];
    pageCoordX = packed[2] & 0xFFFF;
    pageCoordY = packed[2] >> 16;
    subTextureTileX = packed[3] & 0xFFFF;
    subTextureTileY = packed[3] >> 16;
}

//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is bigger by mapping mips 0..X to 1..X in the new region
*/
void
IndirectionMoveShiftDown(uint oldMaxMip, uint oldTiles, const Math::uint2& oldCoord, uint newMaxMip, uint newTiles, const Math::uint2& newCoord, const uint bufferIndex, uint& uploadBufferOffset)
{
    // if downscale, copy whole mipchain of indirection shifted up, cutting off the last mip
    // lod is the mip level for the new tile
    for (uint i = 0; i < oldMaxMip; i++)
    {
        // the number of pixels should be the number of tiles mipped
        uint mipDiff = newMaxMip - oldMaxMip;
        uint newMip = i + mipDiff;
        uint width = oldTiles >> i;
        n_assert(newTiles >> newMip == width);
        uint dataSize = width * sizeof(IndirectionEntry);

        Math::uint2 mippedNewCoord{ newCoord.x >> newMip, newCoord.y >> newMip };
        Math::uint2 mippedOldCoord{ oldCoord.x >> i, oldCoord.y >> i };

        terrainVirtualTileState.indirectionTextureCopiesFromThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), i, 0 });
        terrainVirtualTileState.indirectionTextureCopiesToThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), newMip, 0 });
    }
}

//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is smaller, mapping mips 0..X to 0..X-1 in the new region
*/
void
IndirectionMoveShiftUp(uint oldMaxMip, uint oldTiles, const Math::uint2& oldCoord, uint newMaxMip, uint newTiles, const Math::uint2& newCoord, const uint bufferIndex, uint& uploadBufferOffset)
{
    // if upscale, copy whole mipchain of indirection, starting at mip 1, leaving mip 0 to be updated later
    // lod is the mip level for the new tile
    for (uint i = 0; i < newMaxMip; i++)
    {
        // the number of pixels that should be updated
        uint mipDiff = oldMaxMip - newMaxMip;
        uint oldMip = mipDiff + i;
        uint width = newTiles >> i;
        n_assert(oldTiles >> oldMip == width);
        uint dataSize = width * sizeof(IndirectionEntry);

        Math::uint2 mippedNewCoord{ newCoord.x >> i, newCoord.y >> i };
        Math::uint2 mippedOldCoord{ oldCoord.x >> oldMip, oldCoord.y >> oldMip };

        terrainVirtualTileState.indirectionTextureCopiesFromThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), oldMip, 0 });
        terrainVirtualTileState.indirectionTextureCopiesToThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), i, 0 });
    }
}

//------------------------------------------------------------------------------
/**
    Clear old region
*/
void
IndirectionClear(uint oldMaxMip, uint oldTiles, const Math::uint2& oldCoord, const uint bufferIndex, uint& uploadBufferOffset)
{
    // go through old region and reset indirection pixels
    for (uint i = 0; i < oldMaxMip; i++)
    {
        Math::uint2 mippedOldCoord{ oldCoord.x >> i, oldCoord.y >> i };
        uint width = oldTiles >> i;
        uint dataSize = width * sizeof(IndirectionEntry);

        // add copy commands
        terrainVirtualTileState.indirectionBufferShufflesThisFrame.Append(CoreGraphics::BufferCopy{ uploadBufferOffset });
        terrainVirtualTileState.indirectionTextureShufflesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), i, 0 });

        for (uint j = 0; j < width; j++)
        {
            // create buffer of invalid indirection entry pixels
            Util::FixedArray<IndirectionEntry> pixels(width);
            pixels.Fill(IndirectionEntry{ 0xF, 0xFFFFFFFF, 0xFFFFFFFF });

            // update upload buffer
            CoreGraphics::BufferUpdateArray(terrainVirtualTileState.indirectionUploadBuffers[bufferIndex], pixels.Begin(), width, uploadBufferOffset);
            uploadBufferOffset += dataSize;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::UpdateLOD(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
    N_SCOPE(UpdateLOD, Terrain);
    Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetTransform(view->GetCamera()));
    const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

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
        BiomeSetupSettings settings = terrainBiomeAllocator.Get<TerrainBiome_Settings>(j);
        systemUniforms.BiomeRules[j][0] = settings.slopeThreshold;
        systemUniforms.BiomeRules[j][1] = settings.heightThreshold;
        systemUniforms.BiomeRules[j][2] = settings.uvScaleFactor;
        systemUniforms.BiomeRules[j][3] = CoreGraphics::TextureGetNumMips(terrainState.biomeMasks[j]);
    }
    BufferUpdate(terrainState.systemConstants, systemUniforms, 0);
    BufferFlush(terrainState.systemConstants);

    // copy subtextures from previous frame into this frame after we are done processing the GPU side readback
    uint uploadBufferOffset = 0;

    struct PendingDelete
    {
        uint oldMaxMip;
        uint oldTiles;
        Math::uint2 oldCoord;
    };

    // handle readback from the GPU
    CoreGraphics::BufferInvalidate(terrainVirtualTileState.pageUpdateReadbackBuffers[ctx.bufferIndex]);
    Terrain::PageUpdateList* updateList = (Terrain::PageUpdateList*)CoreGraphics::BufferMap(terrainVirtualTileState.pageUpdateReadbackBuffers[ctx.bufferIndex]);
    CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);

    terrainVirtualTileState.numPixels = updateList->NumEntries;
    for (uint i = 0; i < updateList->NumEntries; i++)
    {
        uint status, subTextureIndex, mip, pageCoordX, pageCoordY, subTextureTileX, subTextureTileY;
        UnpackPageDataEntry(updateList->Entry[i], status, subTextureIndex, mip, pageCoordX, pageCoordY, subTextureTileX, subTextureTileY);

        // the update state is either 1 if the page is allocated, or 2 if it used to be allocated but has since been deallocated
        uint updateState = status;
        uint index = pageCoordX + pageCoordY * (dims.width >> mip);
        const TerrainSubTexture& subTexture = terrainVirtualTileState.subTexturesHistory[ctx.bufferIndex][subTextureIndex];

        n_assert(mip < 0xF);
        n_assert(subTextureTileX < PhysicalTextureTileSize);
        n_assert(subTextureTileY < PhysicalTextureTileSize);

        TileCacheEntry cacheEntry;
        cacheEntry.entry.mip = mip;
        cacheEntry.entry.tileX = subTextureTileX;
        cacheEntry.entry.tileY = subTextureTileY;
        cacheEntry.entry.tiles = subTexture.tiles;
        cacheEntry.entry.subTextureIndex = subTextureIndex;
        cacheEntry.entry.subTextureUpdateKey = terrainVirtualTileState.subTextureUpdateKeys[subTextureIndex];

        Math::uint2 newCoord;
        n_assert(subTexture.tiles > 0);

        bool update = terrainVirtualTileState.physicalTextureTileCache.Cache(cacheEntry, newCoord);
        if (update)
        {
            IndirectionEntry entry;
            entry.mip = mip;
            entry.physicalOffsetX = newCoord.x;
            entry.physicalOffsetY = newCoord.y;

            terrainVirtualTileState.indirectionEntryUpdates.Append(entry);
            terrainVirtualTileState.indirectionTextureCopies.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(pageCoordX, pageCoordY, pageCoordX + 1, pageCoordY + 1), mip, 0 });

            // calculate the world space size of this tile
            float metersPerTile = SubTextureWorldSize / float(subTexture.tiles >> mip);
            float metersPerPixel = metersPerTile / float(PhysicalTextureTilePaddedSize);
            float metersPerTilePadded = metersPerTile + PhysicalTextureTilePadding * metersPerPixel;

            // value to normalize [0..264]i to [0..1]f (1/tilesize) *
            Terrain::TerrainTileUpdateUniforms tileUpdateUniforms;
            tileUpdateUniforms.MetersPerTile = metersPerTilePadded;

            // pagePos is the relative page id into the subtexture, ranging from 0-subTexture.size
            tileUpdateUniforms.SparseTileWorldOffset[0] = subTexture.worldCoordinate[0] + subTextureTileX * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
            tileUpdateUniforms.SparseTileWorldOffset[1] = subTexture.worldCoordinate[1] + subTextureTileY * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
                
            terrainVirtualTileState.pageUniforms.Append(tileUpdateUniforms);
            terrainVirtualTileState.tileOffsets.Append({ newCoord.x, newCoord.y });
        }
    }
    CoreGraphics::BufferUnmap(terrainVirtualTileState.pageUpdateReadbackBuffers[ctx.bufferIndex]);

    IndexT i;

    // run through tile page updates
    SizeT numPagesThisFrame = Math::n_min(64, terrainVirtualTileState.pageUniforms.Size());
    for (i = 0; i < numPagesThisFrame; i++)
    {
        PhysicalPageUpdate pageUpdate;
        pageUpdate.constantBufferOffsets[0] = CoreGraphics::SetGraphicsConstants(CoreGraphics::MainThreadConstantBuffer, terrainVirtualTileState.pageUniforms[i]);
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

    // run through indirection updates and set them up
    numPagesThisFrame = Math::n_min(64, terrainVirtualTileState.indirectionEntryUpdates.Size());
    for (i = 0; i < numPagesThisFrame; i++)
    {
        // setup indirection update
        terrainVirtualTileState.indirectionBufferUpdatesThisFrame.Append({ uploadBufferOffset });
        terrainVirtualTileState.indirectionTextureUpdatesThisFrame.Append(terrainVirtualTileState.indirectionTextureCopies[i]);
        CoreGraphics::BufferUpdate(terrainVirtualTileState.indirectionUploadBuffers[ctx.bufferIndex], terrainVirtualTileState.indirectionEntryUpdates[i], uploadBufferOffset);
        uploadBufferOffset += sizeof(terrainVirtualTileState.indirectionEntryUpdates[i]);
    }
    if (i > 0)
    {
        terrainVirtualTileState.indirectionEntryUpdates.EraseRange(0, numPagesThisFrame - 1);
        terrainVirtualTileState.indirectionTextureCopies.EraseRange(0, numPagesThisFrame - 1);
    }

    // wait for cull and subtexture update jobs
    if (TerrainContext::runningJobs.Size() > 0)
    {
        // wait for all jobs to finish
        Jobs::JobSyncHostWait(TerrainContext::jobHostSync);

        // destroy jobs
        while (!TerrainContext::runningJobs.IsEmpty())
            Jobs::DestroyJob(TerrainContext::runningJobs.Dequeue());
    }

    Util::FixedArray<TerrainSubTexture>& subTextures = terrainVirtualTileState.subTextures;
    for (IndexT i = 0; i < terrainVirtualTileState.subTextureJobOutputs.Size(); i++)
    {
        const Terrain::SubTextureUpdateJobOutput& output = terrainVirtualTileState.subTextureJobOutputs[i];
        Terrain::TerrainSubTexture& subTex = subTextures[i];
        int lowestLod;
        switch (output.updateState)
        {
        case SubTextureUpdateState::Grew:
        case SubTextureUpdateState::Shrank:
        case SubTextureUpdateState::Created:
        {
            Math::uint2 newCoord = terrainVirtualTileState.indirectionOccupancy.Allocate(output.newTiles);
            n_assert(newCoord.x != 0xFFFFFFFF && newCoord.y != 0xFFFFFFFF);

            // calculate new lowest lod
            lowestLod = (int)output.newMaxMip - (int)output.oldMaxMip;
            lowestLod = Math::n_max(0, lowestLod);

            // update subtexture
            subTex.tiles = output.newTiles;
            subTex.indirectionOffset[0] = newCoord.x;
            subTex.indirectionOffset[1] = newCoord.y;
            subTex.maxMip = output.newMaxMip;

            terrainVirtualTileState.virtualSubtextureBufferUpdate = true;

            // add output for update on the next cycle
            Terrain::SubTextureUpdateJobOutput deferredOutput = output;
            deferredOutput.newCoord = newCoord;
            terrainVirtualTileState.subTextureJobHistoryOutputs[ctx.bufferIndex].Append(deferredOutput);
            terrainVirtualTileState.subTextureUpdateKeys[i]++;
            break;
        }
        case SubTextureUpdateState::Deleted:
            subTex.tiles = 0;
            subTex.indirectionOffset[0] = 0xFFFFFFFF;
            subTex.indirectionOffset[1] = 0xFFFFFFFF;
            subTex.maxMip = 0;
            terrainVirtualTileState.virtualSubtextureBufferUpdate = true;
            terrainVirtualTileState.subTextureJobHistoryOutputs[ctx.bufferIndex].Append(output);
            terrainVirtualTileState.subTextureUpdateKeys[i]++;
            break;
        default:
            lowestLod = 0;
            break;
        }
    }


    Util::Array<PendingDelete> pendingDeletes;

    // go through previous outputs and when that frame is finished rendering, issue moves and shuffles
    for (const Terrain::SubTextureUpdateJobOutput& output : terrainVirtualTileState.subTextureJobHistoryOutputs[ctx.bufferIndex])
    {
        switch (output.updateState)
        {
        case SubTextureUpdateState::Grew:
            IndirectionMoveShiftDown(output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, output.newCoord, ctx.bufferIndex, uploadBufferOffset);
            pendingDeletes.Append(PendingDelete{ output.oldMaxMip, output.oldTiles, output.oldCoord });
            break;
        case SubTextureUpdateState::Shrank:
            IndirectionMoveShiftUp(output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, output.newCoord, ctx.bufferIndex, uploadBufferOffset);
            pendingDeletes.Append(PendingDelete{ output.oldMaxMip, output.oldTiles, output.oldCoord });
            break;
        case SubTextureUpdateState::Deleted:
            pendingDeletes.Append(PendingDelete{ output.oldMaxMip, output.oldTiles, output.oldCoord });
            break;
        }
    }

    for (const PendingDelete& toDelete : pendingDeletes)
    {
        IndirectionClear(toDelete.oldMaxMip, toDelete.oldTiles, toDelete.oldCoord, ctx.bufferIndex, uploadBufferOffset);
        terrainVirtualTileState.indirectionOccupancy.Deallocate(toDelete.oldCoord, toDelete.oldTiles);
    }

    // clear old updates
    n_assert(uploadBufferOffset <= IndirectionUpdateBufferSize);
    terrainVirtualTileState.subTextureJobHistoryOutputs[ctx.bufferIndex].Clear();

    // update history buffer
    terrainVirtualTileState.subTexturesHistory[ctx.bufferIndex] = terrainVirtualTileState.subTextures;
    terrainVirtualTileState.subTextureUpdateKeysHistory[ctx.bufferIndex] = terrainVirtualTileState.subTextureUpdateKeys;

    if (terrainVirtualTileState.virtualSubtextureBufferUpdate)
    {
        BufferUpdateArray(
            terrainVirtualTileState.subtextureStagingBuffers[ctx.bufferIndex],
            subTextures.Begin(),
            subTextures.Size());
    }

    for (IndexT i = 0; i < runtimes.Size(); i++)
    {
        TerrainRuntimeInfo& rt = runtimes[i];

        for (IndexT j = 0; j < rt.sectorVisible.Size(); j++)
        {
            if (rt.sectorVisible[j])
            {
                const Math::bbox& box = rt.sectionBoxes[j];
                Terrain::PatchUniforms uniforms;
                uniforms.OffsetPatchPos[0] = box.pmin.x;
                uniforms.OffsetPatchPos[1] = box.pmin.z;
                uniforms.OffsetPatchUV[0] = rt.sectorUv[j].x;
                uniforms.OffsetPatchUV[1] = rt.sectorUv[j].y;
                rt.sectorUniformOffsets[j][0] = 0;
                rt.sectorUniformOffsets[j][1] = CoreGraphics::SetGraphicsConstants(CoreGraphics::MainThreadConstantBuffer, uniforms);
            }
        }

        TerrainRuntimeUniforms uniforms;
        Math::mat4().store(uniforms.Transform);
        uniforms.DecisionMap = TextureGetBindlessHandle(rt.decisionMap);
        uniforms.HeightMap = TextureGetBindlessHandle(rt.heightMap);

        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(rt.heightMap);
        uniforms.VirtualTerrainTextureSize[0] = dims.width;
        uniforms.VirtualTerrainTextureSize[1] = dims.height;
        uniforms.VirtualTerrainTextureSize[2] = 1.0f / dims.width;
        uniforms.VirtualTerrainTextureSize[3] = 1.0f / dims.height;
        uniforms.MaxHeight = rt.maxHeight;
        uniforms.MinHeight = rt.minHeight;
        uniforms.WorldSizeX = rt.worldWidth;
        uniforms.WorldSizeZ = rt.worldHeight;
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

        for (SizeT j = 0; j < terrainVirtualTileState.indirectionMipOffsets.Size(); j++)
        {
            uniforms.VirtualPageBufferMipOffsets[j / 4][j % 4] = terrainVirtualTileState.indirectionMipOffsets[j];
            uniforms.VirtualPageBufferMipSizes[j / 4][j % 4] = terrainVirtualTileState.indirectionMipSizes[j];
        }
        uniforms.VirtualPageBufferNumPages = CoreGraphics::BufferGetSize(terrainVirtualTileState.pageStatusBuffer);

        BufferUpdate(terrainVirtualTileState.runtimeConstants, uniforms, 0);
        BufferFlush(terrainVirtualTileState.runtimeConstants);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::RenderUI(const Graphics::FrameContext& ctx)
{
    if (ImGui::Begin("Terrain Debug 2"))
    {
        ImGui::SetWindowSize(ImVec2(240, 400), ImGuiCond_Once);
        ImGui::Checkbox("Debug Render", &terrainState.debugRender);
        ImGui::Checkbox("Don't Render", &terrainState.renderToggle);
        ImGui::LabelText("Updates", "Number of updates %d", terrainVirtualTileState.numPixels);

        {
            ImGui::Text("Indirection texture occupancy quadtree");
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 start = ImGui::GetCursorScreenPos();
            ImVec2 fullSize = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
            drawList->PushClipRect(
                ImVec2{ start.x, start.y },
                ImVec2{ Math::n_max(start.x + fullSize.x, start.x + 512.0f), Math::n_min(start.y + fullSize.y, start.y + 512.0f) }, true);

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
                ImVec2{ Math::n_max(start.x + fullSize.x, start.x + 512.0f), Math::n_min(start.y + fullSize.y, start.y + 512.0f) }, true);

            terrainVirtualTileState.physicalTextureTileOccupancy.DebugRender(drawList, start, 0.0625f);
            drawList->PopClipRect();

            // set back cursor so we can draw our box
            ImGui::SetCursorScreenPos(start);
            ImGui::InvisibleButton("Physical texture occupancy quadtree", ImVec2(512.0f, 512.0f));
            */
        }

        {
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.physicalNormalCache);

            ImVec2 imageSize = { (float)dims.width, (float)dims.height };

            static Dynui::ImguiTextureId textureInfo;
            textureInfo.nebulaHandle = terrainVirtualTileState.physicalNormalCache.HashCode64();
            textureInfo.mip = 0;
            textureInfo.layer = 0;

            ImGui::NewLine();
            ImGui::Separator();

            imageSize.x = ImGui::GetWindowContentRegionWidth();
            float ratio = (float)dims.height / (float)dims.width;
            imageSize.y = imageSize.x * ratio;

            ImGui::Image((void*)&textureInfo, imageSize);
        }

        {
            CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);

            ImVec2 imageSize = { (float)dims.width, (float)dims.height };

            static Dynui::ImguiTextureId textureInfo;
            textureInfo.nebulaHandle = terrainVirtualTileState.indirectionTexture.HashCode64();
            textureInfo.mip = 0;
            textureInfo.layer = 0;

            ImGui::NewLine();
            ImGui::Separator();

            imageSize.x = ImGui::GetWindowContentRegionWidth();
            float ratio = (float)dims.height / (float)dims.width;
            imageSize.y = imageSize.x * ratio;

            ImGui::Image((void*)&textureInfo, imageSize);
        }
    }

    ImGui::End();
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
