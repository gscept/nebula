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
#include "raytracing/raytracingcontext.h"

#include "resources/resourceserver.h"
#include "materials/materialloader.h"

#include "gpulang/render/terrain/shaders/terrain_mesh_generate.h"

#include "frame/default.h"

N_DECLARE_COUNTER(N_TERRAIN_TOTAL_AVAILABLE_DATA, Terrain Total Data Size);

namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
TerrainContext::TerrainBiomeAllocator TerrainContext::terrainBiomeAllocator;

const uint IndirectionTextureSize = 2048;
const uint SubTextureWorldSize = 64;
const uint SubTextureMaxTiles = 256;
const uint IndirectionNumMips = Math::log2(SubTextureMaxTiles) + 1;
const float SubTextureRange = 300.0f; // 300 meter range
const float SubTextureSwapDistance = 10.0f;
const float SubTextureFadeStart = 200.0f;
const uint SubTextureMaxPixels = 65536;

const uint LowresFallbackSize = 4096;
const uint LowresFallbackMips = Math::log2(LowresFallbackSize) + 1;

const uint PhysicalTextureTileSize = 256;
const uint PhysicalTextureTileHalfPadding = 4;
const uint PhysicalTextureTilePadding = PhysicalTextureTileHalfPadding * 2;
const uint PhysicalTextureNumTiles = 32;
const uint PhysicalTextureTilePaddedSize = PhysicalTextureTileSize + PhysicalTextureTilePadding;
const uint PhysicalTextureSize = (PhysicalTextureTileSize) * PhysicalTextureNumTiles;
const uint PhysicalTexturePaddedSize = (PhysicalTextureTilePaddedSize) * PhysicalTextureNumTiles;

const uint TerrainShadowMapSize = 2048;


__ImplementContext(TerrainContext, TerrainContext::terrainAllocator);

struct
{
    CoreGraphics::ShaderId terrainShader;
    CoreGraphics::ShaderId tileShader;
    CoreGraphics::ShaderProgramId terrainShadowProgram;
    CoreGraphics::VertexLayoutId vlo;

    CoreGraphics::BufferId biomeBuffer;
    Terrain::MaterialLayers::STRUCT biomeMaterials;
    Util::Array<CoreGraphics::TextureId> biomeTextures;

    BiomeMaterial biomeResources[Terrain::MAX_BIOMES][BiomeSettings::BiomeMaterialLayer::NumLayers];
    CoreGraphics::TextureId biomeMasks[Terrain::MAX_BIOMES];
    CoreGraphics::TextureId biomeWeights[Terrain::MAX_BIOMES];
    Threading::AtomicCounter biomeLoaded[Terrain::MAX_BIOMES][4];
    uint biomeLowresGenerated[Terrain::MAX_BIOMES];
    IndexT biomeCounter;

    Graphics::GraphicsEntityId sun;

    Math::vec4 cachedSunDirection;
    bool shadowMapInvalid;
    bool updateShadowMap;

    Threading::CriticalSection syncPoint;
    CoreGraphics::ResourceTableId terrainShadowResourceTable;

    CoreGraphics::TextureId terrainPosBuffer;
    
    float mipLoadDistance = 1500.0f;
    float mipRenderPadding = 150.0f;
    SizeT layers;

    bool debugRender;
    bool renderToggle;

} terrainState;

struct
{
    CoreGraphics::ShaderProgramId                                   terrainPrepassProgram;
    CoreGraphics::PipelineId                                        terrainPrepassPipeline;
    CoreGraphics::ShaderProgramId                                   terrainPageClearUpdateBufferProgram;
    CoreGraphics::ShaderProgramId                                   terrainResolveProgram;
    CoreGraphics::PipelineId                                        terrainResolvePipeline;
    CoreGraphics::ShaderProgramId                                   terrainTileWriteProgram;
    CoreGraphics::ShaderProgramId                                   terrainWriteLowresProgram;

} terrainVirtualTileState;

struct
{
    CoreGraphics::ShaderProgramId meshProgram;
    CoreGraphics::VertexAlloc vertexBuffer, indexBuffer;
    CoreGraphics::ResourceTableId meshGenTable;
    CoreGraphics::BufferId patchBuffer, constantsBuffer;
    SizeT numTris, numPatches;
    bool updateMesh = false;
    std::function<void()> terrainSetupCallback;
    IndexT setupBlasFrame;
} raytracingState;

struct TerrainVert
{
    Math::float3 position;
    short uv[2];
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
TerrainContext::Create()
{
    __CreateContext();
    using namespace CoreGraphics;

    Core::CVarCreate(Core::CVarType::CVar_Int, "r_terrain_debug", "0", "Show debug interface for the terrain system [0,1]");

#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
    Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

    terrainState.updateShadowMap = true;
    terrainState.cachedSunDirection = Math::vec4(0);

    // create vertex buffer
    Util::Array<VertexComponent> vertexComponents = 
    {
        VertexComponent{ 0, VertexComponent::Format::Float3 },
        VertexComponent{ 1, VertexComponent::Format::Short2 },
    };
    terrainState.vlo = CreateVertexLayout({ .name = "Terrain"_atm, .comps = vertexComponents });

    terrainState.terrainShader = ShaderGet("shd:terrain/shaders/terrain.gplb");
    terrainState.tileShader = ShaderGet("shd:terrain/shaders/terrain_tile_write.gplb");

    terrainState.terrainShadowProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureMask("TerrainShadows"));

    CoreGraphics::ShaderId meshGenShader = ShaderGet("shd:terrain/shaders/terrain_mesh_generate.gplb");
    raytracingState.meshProgram = ShaderGetProgram(meshGenShader, ShaderFeatureMask("Main"));
    raytracingState.meshGenTable = ShaderCreateResourceTable(meshGenShader, NEBULA_BATCH_GROUP, 1);

    CoreGraphics::BufferCreateInfo biomeBufferInfo;
    biomeBufferInfo.name = "BiomeBuffer"_atm;
    biomeBufferInfo.size = 1;
    biomeBufferInfo.elementSize = sizeof(Terrain::MaterialLayers::STRUCT);
    biomeBufferInfo.mode = CoreGraphics::DeviceAndHost;
    biomeBufferInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    terrainState.biomeBuffer = CoreGraphics::CreateBuffer(biomeBufferInfo);



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

    terrainVirtualTileState.terrainPageClearUpdateBufferProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureMask("TerrainPageClearUpdateBuffer"));
    terrainVirtualTileState.terrainPrepassProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureMask("TerrainPrepass"));
    terrainVirtualTileState.terrainResolveProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureMask("TerrainResolve"));
    terrainVirtualTileState.terrainTileWriteProgram = ShaderGetProgram(terrainState.tileShader, ShaderFeatureMask("TerrainTileWrite"));
    terrainVirtualTileState.terrainWriteLowresProgram = ShaderGetProgram(terrainState.tileShader, ShaderFeatureMask("TerrainLowresWrite"));
    if (CoreGraphics::RayTracingSupported)
    {
        FrameScript_default::RegisterSubgraph_TerrainRaytracingMeshGenerate_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
        {
            if (raytracingState.updateMesh)
            {
                CmdBeginMarker(cmdBuf, NEBULA_MARKER_COMPUTE, "Terrain Update Raytracing Mesh");

                CmdSetShaderProgram(cmdBuf, raytracingState.meshProgram);
                CmdSetResourceTable(cmdBuf, raytracingState.meshGenTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr);
                CmdDispatch(cmdBuf, Math::divandroundup(raytracingState.numTris, 64), raytracingState.numPatches, 1);

                CmdEndMarker(cmdBuf);

                // Avoid more mesh updates
                raytracingState.updateMesh = false;

                raytracingState.setupBlasFrame = bufferIndex + CoreGraphics::GetNumBufferedFrames();
            }
        });
    }

    
    FrameScript_default::RegisterSubgraph_TerrainPrepare_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Terrain Shuffle Indirection Regions");

        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            terrainInstance.barrierContext.Start("Terrain Indirection Shuffle Sync", cmdBuf);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageStatusBufferBarrierIndex, terrainInstance.pageStatusBuffer, CoreGraphics::PipelineStage::TransferWrite);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.subtextureBufferBarrierIndex, terrainInstance.subTextureBuffer, CoreGraphics::PipelineStage::TransferWrite);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.indirectionBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.indirectionTexture, CoreGraphics::PipelineStage::TransferWrite);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.indirectionCopyBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.indirectionTextureCopy, CoreGraphics::PipelineStage::TransferRead);
            terrainInstance.barrierContext.Synchronize();

            if (terrainInstance.indirectionBufferUpdatesThisFrame.Size() > 0
            || terrainInstance.indirectionBufferClearsThisFrame.Size() > 0
            || terrainInstance.indirectionTextureFromCopiesThisFrame.Size() > 0)
            {

                // Perform clears
                if (terrainInstance.indirectionBufferClearsThisFrame.Size() > 0)
                {
                    CmdBarrier(cmdBuf,
                        PipelineStage::HostWrite,
                        PipelineStage::TransferRead,
                        BarrierDomain::Global,
                        {
                            BufferBarrierInfo
                            {
                                terrainInstance.indirectionUploadBuffers.buffers[bufferIndex],
                                    CoreGraphics::BufferSubresourceInfo()
                            },

                        });

                    // perform the moves and clears of indirection pixels
                    CmdCopy(cmdBuf,
                        terrainInstance.indirectionUploadBuffers.buffers[bufferIndex], terrainInstance.indirectionBufferClearsThisFrame,
                        terrainInstance.indirectionTexture, terrainInstance.indirectionTextureClearsThisFrame);

                    CmdBarrier(cmdBuf,
                        PipelineStage::TransferRead,
                        PipelineStage::HostWrite,
                        BarrierDomain::Global,
                        {
                                BufferBarrierInfo
                            {
                                terrainInstance.indirectionUploadBuffers.buffers[bufferIndex],
                                    CoreGraphics::BufferSubresourceInfo()
                            },
                        });
                }

                // Perform mip shifts
                if (terrainInstance.indirectionTextureFromCopiesThisFrame.Size() > 0)
                {
                    CmdCopy(cmdBuf,
                        terrainInstance.indirectionTextureCopy, terrainInstance.indirectionTextureFromCopiesThisFrame,
                        terrainInstance.indirectionTexture, terrainInstance.indirectionTextureToCopiesThisFrame);
                }

                // Perform CPU side updates
                if (terrainInstance.indirectionBufferUpdatesThisFrame.Size() > 0)
                {
                    CmdBarrier(cmdBuf,
                        PipelineStage::HostWrite,
                        PipelineStage::TransferRead,
                        BarrierDomain::Global,
                        {
                            BufferBarrierInfo
                            {
                                    terrainInstance.indirectionUploadBuffers.buffers[bufferIndex],
                                    CoreGraphics::BufferSubresourceInfo()
                            },

                        });

                    // update the new pixels
                    CmdCopy(cmdBuf,
                        terrainInstance.indirectionUploadBuffers.buffers[bufferIndex], terrainInstance.indirectionBufferUpdatesThisFrame,
                        terrainInstance.indirectionTexture, terrainInstance.indirectionTextureUpdatesThisFrame);

                    CmdBarrier(cmdBuf,
                        PipelineStage::TransferRead,
                        PipelineStage::HostWrite,
                        BarrierDomain::Global,
                        {
                                BufferBarrierInfo
                            {
                                terrainInstance.indirectionUploadBuffers.buffers[bufferIndex],
                                    CoreGraphics::BufferSubresourceInfo()
                            },
                        });
                }

                // Clear pending updates
                terrainInstance.indirectionTextureFromCopiesThisFrame.Clear();
                terrainInstance.indirectionTextureToCopiesThisFrame.Clear();

                terrainInstance.indirectionBufferUpdatesThisFrame.Clear();
                terrainInstance.indirectionTextureUpdatesThisFrame.Clear();

                terrainInstance.indirectionBufferClearsThisFrame.Clear();
                terrainInstance.indirectionTextureClearsThisFrame.Clear();
            }

            // If we have subtexture updates, make sure to copy them too
            if (terrainInstance.virtualSubtextureBufferUpdate)
            {
                CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Copy SubTextures from Host Buffer");
                CmdBarrier(cmdBuf,
                    PipelineStage::HostWrite,
                    PipelineStage::TransferRead,
                    BarrierDomain::Global,
                    {
                        BufferBarrierInfo
                        {
                            terrainInstance.subtextureStagingBuffers.buffers[bufferIndex],
                                CoreGraphics::BufferSubresourceInfo()
                        },
                    });

                BufferCopy from, to;
                from.offset = 0;
                to.offset = 0;
                CmdCopy(cmdBuf
                    , terrainInstance.subtextureStagingBuffers.buffers[bufferIndex], { from }
                    , terrainInstance.subTextureBuffer, { to }
                , BufferGetByteSize(terrainInstance.subTextureBuffer));

                CmdBarrier(cmdBuf,
                    PipelineStage::TransferRead,
                    PipelineStage::HostWrite,
                    BarrierDomain::Global,
                    {
                        BufferBarrierInfo
                        {
                            terrainInstance.subtextureStagingBuffers.buffers[bufferIndex],
                                CoreGraphics::BufferSubresourceInfo()
                        },
                    });

                terrainInstance.virtualSubtextureBufferUpdate = false;
                CmdEndMarker(cmdBuf);
            }
        }
        CmdEndMarker(cmdBuf);
        
    });

    FrameScript_default::RegisterSubgraph_TerrainIndirectionCopy_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Terrain Copy Indirection Mips");

        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {

            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            terrainInstance.barrierContext.Start("Terrain Indirection Copy Sync", cmdBuf);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.indirectionBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.indirectionTexture, CoreGraphics::PipelineStage::TransferRead);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.indirectionCopyBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.indirectionTextureCopy, CoreGraphics::PipelineStage::TransferWrite);
            terrainInstance.barrierContext.Synchronize();

            // Copy this frame of indirection to copy
            TextureDimensions dims = TextureGetDimensions(terrainInstance.indirectionTexture);
            for (IndexT mip = 0; mip < TextureGetNumMips(terrainInstance.indirectionTexture); mip++)
            {
                TextureCopy cp;
                cp.mip = mip;
                cp.layer = 0;
                cp.region = Math::rectangle(0, 0, dims.width >> mip, dims.height >> mip);
                CmdCopy(cmdBuf,
                        terrainInstance.indirectionTexture, { cp }, terrainInstance.indirectionTextureCopy, { cp });
            }
        }
        CmdEndMarker(cmdBuf);
    }, nullptr);

    FrameScript_default::RegisterSubgraphPipelines_TerrainPrepass_Pass([](const CoreGraphics::PassId pass, uint subpass)
    {
        if (terrainVirtualTileState.terrainPrepassPipeline != InvalidPipelineId)
            DestroyGraphicsPipeline(terrainVirtualTileState.terrainPrepassPipeline);
        terrainVirtualTileState.terrainPrepassPipeline = CreateGraphicsPipeline(
            {
                .shader = terrainVirtualTileState.terrainPrepassProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = InputAssemblyKey{ { .topo = CoreGraphics::PrimitiveTopology::PatchList, .primRestart = false } }
            });
    });

    FrameScript_default::RegisterSubgraphSync_TerrainPrepass_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
            terrainInstance.barrierContext.Start("Terrain Prepass Pass Sync", cmdBuf);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageStatusBufferBarrierIndex, terrainInstance.pageStatusBuffer, CoreGraphics::PipelineStage::PixelShaderWrite);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageUpdateListBarrierIndex, terrainInstance.pageUpdateListBuffer, CoreGraphics::PipelineStage::PixelShaderWrite);
            terrainInstance.barrierContext.Synchronize();
        }
    });

    FrameScript_default::RegisterSubgraph_TerrainPrepass_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (terrainState.renderToggle == false)
            return;

        // setup shader state, set shader before we set the vertex layout
        CmdSetGraphicsPipeline(cmdBuf, terrainVirtualTileState.terrainPrepassPipeline);
        CmdSetVertexLayout(cmdBuf, terrainState.vlo);
        CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::PatchList);

        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            // set shared resources
            CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

            // Draw terrains

            SizeT numQuadsX = terrainInstance.createInfo.quadsPerTileX;
            SizeT numQuadsY = terrainInstance.createInfo.quadsPerTileY;
            SizeT numVertsX = numQuadsX + 1;
            SizeT numVertsY = numQuadsY + 1;
            SizeT numTris = numVertsX * numVertsY;

            CoreGraphics::PrimitiveGroup group;
            group.SetBaseIndex(0);
            group.SetBaseVertex(0);
            group.SetNumIndices(numTris * 4);
            group.SetNumVertices(0);

            TerrainRuntimeInfo& rt = runtimes[instanceIndex];
            CmdSetResourceTable(cmdBuf, terrainInstance.runtimeTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            CmdSetVertexBuffer(cmdBuf, 0, rt.vbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index32, rt.ibo, 0);
            CmdDraw(cmdBuf, terrainInstance.numPatchesThisFrame, group);
        }
    });

    FrameScript_default::RegisterSubgraph_TerrainPageExtract_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_TRANSFER, "Terrain Copy Pages to Readback");
        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
            terrainInstance.barrierContext.Start("Terrain Readback Copy Sync", cmdBuf);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageUpdateListBarrierIndex, terrainInstance.pageUpdateListBuffer, CoreGraphics::PipelineStage::TransferRead);
            terrainInstance.barrierContext.Synchronize();

            CoreGraphics::BufferCopy from, to;
            from.offset = 0;
            to.offset = 0;
            CmdCopy(
                cmdBuf,
                terrainInstance.pageUpdateListBuffer, { from }, terrainInstance.pageUpdateReadbackBuffers.buffers[bufferIndex], { to },
                sizeof(Terrain::PageUpdateList::STRUCT)
            );
        }
        CmdEndMarker(cmdBuf);
    });

    FrameScript_default::RegisterSubgraph_SunTerrainShadows_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        if (!terrainState.renderToggle)
            return;

        Threading::CriticalScope scope(&terrainState.syncPoint);
        if (!terrainState.updateShadowMap)
            return;

        terrainState.updateShadowMap = false;

        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            // Setup shader state, set shader before we set the vertex layout
            CmdSetShaderProgram(cmdBuf, terrainState.terrainShadowProgram);

            // Set shared resources
            CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
            CmdSetResourceTable(cmdBuf, terrainInstance.runtimeTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

            // Dispatch
            uint x = Math::divandroundup(TerrainShadowMapSize, 64);
            CmdDispatch(cmdBuf, x, TerrainShadowMapSize, 1);
        }
    });

    FrameScript_default::RegisterSubgraph_TerrainPagesClear_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdBeginMarker(cmdBuf, NEBULA_MARKER_COMPUTE, "Terrain Clear Page Status Buffer");
        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            terrainInstance.barrierContext.Start("Terrain Page Clear Sync", cmdBuf);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageStatusBufferBarrierIndex, terrainInstance.pageStatusBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite);
            terrainInstance.barrierContext.SyncBuffer(terrainInstance.pageUpdateListBarrierIndex, terrainInstance.pageUpdateListBuffer, CoreGraphics::PipelineStage::ComputeShaderWrite);
            terrainInstance.barrierContext.Synchronize();

            CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainPageClearUpdateBufferProgram);
            CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

            // run a single compute shader to clear the number of page entries
            uint numDispatches = Math::divandroundup(Terrain::MAX_PAGE_UPDATES, 64);
            CmdDispatch(cmdBuf, numDispatches, 1, 1);
        }
        CmdEndMarker(cmdBuf);
    });

    FrameScript_default::RegisterSubgraph_TerrainUpdateCaches_Compute([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
            terrainInstance.barrierContext.Start("Terrain Update Caches Sync", cmdBuf);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.albedoCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalAlbedoCacheBC, CoreGraphics::PipelineStage::ComputeShaderWrite);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.normalCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalNormalCacheBC, CoreGraphics::PipelineStage::ComputeShaderWrite);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.materialCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalMaterialCacheBC, CoreGraphics::PipelineStage::ComputeShaderWrite);
            terrainInstance.barrierContext.Synchronize();

            if (terrainInstance.updateLowres)
            {
                terrainInstance.updateLowres = false;

                CmdBeginMarker(cmdBuf, NEBULA_MARKER_COMPUTE, "Update Lowres caches");

                // Transition rendered to mips to be shader read
                CoreGraphics::CmdBarrier(
                    cmdBuf,
                    CoreGraphics::PipelineStage::AllShadersRead,
                    CoreGraphics::PipelineStage::ComputeShaderWrite,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        {
                            terrainInstance.lowresAlbedo,
                            CoreGraphics::TextureSubresourceInfo(),
                        },
                        {
                            terrainInstance.lowresMaterial,
                            CoreGraphics::TextureSubresourceInfo(),
                        },
                        {
                            terrainInstance.lowresNormal,
                            CoreGraphics::TextureSubresourceInfo(),
                        }
                    });

                // Setup state for update
                CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainWriteLowresProgram);
                CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
                CmdSetResourceTable(cmdBuf, terrainInstance.runtimeTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

                CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainInstance.lowresAlbedo);

                CmdDispatch(cmdBuf, Math::divandroundup(dims.width, 8), Math::divandroundup(dims.height, 8), 1);

                // Transition rendered to mips to be shader read
                CoreGraphics::CmdBarrier(
                    cmdBuf,
                    CoreGraphics::PipelineStage::ComputeShaderWrite,
                    CoreGraphics::PipelineStage::AllShadersRead,
                    CoreGraphics::BarrierDomain::Global,
                    {
                        {
                            terrainInstance.lowresAlbedo,
                            CoreGraphics::TextureSubresourceInfo(),
                        },
                        {
                            terrainInstance.lowresMaterial,
                            CoreGraphics::TextureSubresourceInfo(),
                        },
                        {
                            terrainInstance.lowresNormal,
                            CoreGraphics::TextureSubresourceInfo(),
                        }
                    });

                // Now generate mipmaps
                CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainInstance.lowresAlbedo);
                CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainInstance.lowresMaterial);
                CoreGraphics::TextureGenerateMipmaps(cmdBuf, terrainInstance.lowresNormal);

                CmdEndMarker(cmdBuf);
            }

            if (terrainInstance.tileWritesThisFrame.Size() > 0)
            {
                CmdBeginMarker(cmdBuf, NEBULA_MARKER_COMPUTE, "Update Texture Caches");

                CoreGraphics::BarrierPush(
                    cmdBuf
                    , CoreGraphics::PipelineStage::ComputeShaderRead
                    , CoreGraphics::PipelineStage::TransferWrite
                    , CoreGraphics::BarrierDomain::Global
                    , {
                        CoreGraphics::BufferBarrierInfo
                        {
                            .buf = terrainInstance.tileWriteBufferSet.DeviceBuffer(),
                            .subres = CoreGraphics::BufferSubresourceInfo{}
                        }
                    }
                );
                terrainInstance.tileWriteBufferSet.Flush(cmdBuf, terrainInstance.tileWritesThisFrame.ByteSize());
                CoreGraphics::BarrierPop(cmdBuf);
                CmdSetShaderProgram(cmdBuf, terrainVirtualTileState.terrainTileWriteProgram);
                CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

                // go through pending page updates and render into the physical texture caches
                CmdSetResourceTable(cmdBuf, terrainInstance.runtimeTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
                static const uint numDispatches = Math::divandroundup(PhysicalTextureTilePaddedSize, 8);

                CmdDispatch(cmdBuf, numDispatches, numDispatches, terrainInstance.tileWritesThisFrame.Size());
                terrainInstance.tileWritesThisFrame.Clear();


                CmdEndMarker(cmdBuf);
            }
        }
    }, nullptr);

    FrameScript_default::RegisterSubgraphPipelines_TerrainResolve_Pass([](const CoreGraphics::PassId pass, uint subpass)
    {
        if (terrainVirtualTileState.terrainResolvePipeline != InvalidPipelineId)
            DestroyGraphicsPipeline(terrainVirtualTileState.terrainResolvePipeline);
        terrainVirtualTileState.terrainResolvePipeline = CoreGraphics::CreateGraphicsPipeline(
            {
                .shader = terrainVirtualTileState.terrainResolveProgram,
                .pass = pass,
                .subpass = subpass,
                .inputAssembly = InputAssemblyKey{ { .topo = CoreGraphics::PrimitiveTopology::PatchList, .primRestart = false } }
            });
    });
    FrameScript_default::RegisterSubgraphSync_TerrainResolve_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
            terrainInstance.barrierContext.Start("Terrain Resolve Pass Sync", cmdBuf);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.albedoCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalAlbedoCacheBC, CoreGraphics::PipelineStage::PixelShaderRead);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.normalCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalNormalCacheBC, CoreGraphics::PipelineStage::PixelShaderRead);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.materialCacheBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.physicalMaterialCacheBC, CoreGraphics::PipelineStage::PixelShaderRead);
            terrainInstance.barrierContext.SyncTexture(terrainInstance.indirectionBarrierIndex, CoreGraphics::ImageBits::ColorBits, terrainInstance.indirectionTexture, CoreGraphics::PipelineStage::PixelShaderRead);
            terrainInstance.barrierContext.Synchronize();
        }
    });
    FrameScript_default::RegisterSubgraph_TerrainResolve_Pass([](const CoreGraphics::CmdBufferId cmdBuf, const Math::rectangle<int>& viewport, const IndexT frame, const IndexT bufferIndex)
    {
        CmdSetGraphicsPipeline(cmdBuf, terrainVirtualTileState.terrainResolvePipeline);
        CmdSetVertexLayout(cmdBuf, terrainState.vlo);
        CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::PatchList);

        Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
        for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
        {
            TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

            // set shared resources
            CmdSetResourceTable(cmdBuf, terrainInstance.systemTable.tables[bufferIndex], NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

            if (terrainState.renderToggle == false)
                return;

            // Draw terrains
            Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

            SizeT numQuadsX = terrainInstance.createInfo.quadsPerTileX;
            SizeT numQuadsY = terrainInstance.createInfo.quadsPerTileY;
            SizeT numVertsX = numQuadsX + 1;
            SizeT numVertsY = numQuadsY + 1;
            SizeT numTris = numVertsX * numVertsY;

            CoreGraphics::PrimitiveGroup group;
            group.SetBaseIndex(0);
            group.SetBaseVertex(0);
            group.SetNumIndices(numTris * 4);
            group.SetNumVertices(0);

            TerrainRuntimeInfo& rt = runtimes[instanceIndex];
            CmdSetResourceTable(cmdBuf, terrainInstance.runtimeTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

            CmdSetVertexBuffer(cmdBuf, 0, rt.vbo, 0);
            CmdSetIndexBuffer(cmdBuf, IndexType::Index32, rt.ibo, 0);
            CmdDraw(cmdBuf, terrainInstance.numPatchesThisFrame, group);
        }
    }, nullptr);

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
    Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    for (int i = 0; i < terrainInstances.Size(); i++)
    {
        auto& terrainInstance = terrainInstances[i];

        delete terrainInstance.subtexturesFinishedEvent;
        delete terrainInstance.sectionCullFinishedEvent;

        DestroyBuffer(terrainInstance.systemConstants);
        terrainInstance.subtextureStagingBuffers.Destroy();
        DestroyBuffer(terrainInstance.subTextureBuffer);

        terrainInstance.pageUpdateReadbackBuffers.Destroy();
        DestroyTexture(terrainInstance.indirectionTexture);
        DestroyTexture(terrainInstance.indirectionTextureCopy);

        DestroyBuffer(terrainInstance.pageUpdateListBuffer);
        DestroyBuffer(terrainInstance.pageStatusBuffer);

        terrainInstance.patchConstants.Destroy();
        DestroyBuffer(terrainInstance.runtimeConstants);
        terrainInstance.tileWriteBufferSet.Destroy();

        DestroyTexture(terrainInstance.physicalAlbedoCacheBC);
        DestroyTextureView(terrainInstance.physicalAlbedoCacheBCWrite);
        DestroyTexture(terrainInstance.physicalNormalCacheBC);
        DestroyTextureView(terrainInstance.physicalNormalCacheBCWrite);
        DestroyTexture(terrainInstance.physicalMaterialCacheBC);
        DestroyTextureView(terrainInstance.physicalMaterialCacheBCWrite);

        DestroyTexture(terrainInstance.lowresAlbedo);
        DestroyTexture(terrainInstance.lowresNormal);
        DestroyTexture(terrainInstance.lowresMaterial);

        terrainInstance.systemTable.Destroy();
        DestroyResourceTable(terrainInstance.runtimeTable);

        terrainInstance.subTextures.Clear();
        terrainInstance.gpuSubTextures.Clear();
        terrainInstance.physicalTextureTileCache.Clear();
        terrainInstance.indirectionMipOffsets.Clear();
        terrainInstance.indirectionMipSizes.Clear();
        terrainInstance.indirectionTextureCopies.Clear();
        terrainInstance.indirectionEntryUpdates.Clear();
        terrainInstance.indirectionBuffer.Clear();
        terrainInstance.indirectionUploadBuffers.Destroy();
        terrainInstance.indirectionUploadOffsets.Clear();
        terrainInstance.tileWrites.Clear();
        terrainInstance.tileWritesThisFrame.Clear();

        terrainInstance.indirectionBufferUpdatesThisFrame.Clear();
        terrainInstance.indirectionTextureUpdatesThisFrame.Clear();
        terrainInstance.indirectionTextureFromCopiesThisFrame.Clear();
        terrainInstance.indirectionTextureToCopiesThisFrame.Clear();
        terrainInstance.indirectionBufferClearsThisFrame.Clear();
        terrainInstance.indirectionTextureClearsThisFrame.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetupTerrain(
    const Graphics::GraphicsEntityId entity
    ,  const TerrainCreateInfo& createInfo)
{
    n_assert(createInfo.width > 0);
    n_assert(createInfo.height > 0);
    using namespace CoreGraphics;
    const Graphics::ContextEntityId cid = GetContextId(entity);
    TerrainRuntimeInfo& runtimeInfo = terrainAllocator.Get<Terrain_RuntimeInfo>(cid.id);
    TerrainInstanceInfo& instanceInfo = terrainAllocator.Get<Terrain_InstanceInfo>(cid.id);
    runtimeInfo.enableRayTracing = CoreGraphics::RayTracingSupported && createInfo.enableRayTracing;

    instanceInfo.updateLowres = false;

    instanceInfo.pageUpdateListBarrierIndex = instanceInfo.barrierContext.RegisterBuffer(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.pageStatusBufferBarrierIndex = instanceInfo.barrierContext.RegisterBuffer(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.subtextureBufferBarrierIndex = instanceInfo.barrierContext.RegisterBuffer(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.indirectionBarrierIndex = instanceInfo.barrierContext.RegisterTexture(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.indirectionCopyBarrierIndex = instanceInfo.barrierContext.RegisterTexture(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.albedoCacheBarrierIndex = instanceInfo.barrierContext.RegisterTexture(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.materialCacheBarrierIndex = instanceInfo.barrierContext.RegisterTexture(CoreGraphics::PipelineStage::AllShadersRead);
    instanceInfo.normalCacheBarrierIndex = instanceInfo.barrierContext.RegisterTexture(CoreGraphics::PipelineStage::AllShadersRead);

    instanceInfo.subtexturesFinishedEvent = new Threading::Event;
    instanceInfo.subtexturesFinishedEvent->Signal();
    instanceInfo.sectionCullFinishedEvent = new Threading::Event;
    instanceInfo.sectionCullFinishedEvent->Signal();

    instanceInfo.createInfo = createInfo;
    runtimeInfo.worldWidth = createInfo.width;
    runtimeInfo.worldHeight = createInfo.height;
    runtimeInfo.maxHeight = createInfo.maxHeight;
    runtimeInfo.minHeight = createInfo.minHeight;

    // divide world dimensions into 
    runtimeInfo.numTilesX = createInfo.width / createInfo.tileWidth;
    runtimeInfo.numTilesY = createInfo.height / createInfo.tileHeight;
    runtimeInfo.tileWidth = createInfo.tileWidth;
    runtimeInfo.tileHeight = createInfo.tileHeight;
    SizeT height = createInfo.maxHeight - createInfo.minHeight;
    SizeT numQuadsX = createInfo.quadsPerTileX;
    SizeT numQuadsY = createInfo.quadsPerTileY;
    SizeT numVertsX = numQuadsX + 1;
    SizeT numVertsY = numQuadsY + 1;
    SizeT vertDistanceX = createInfo.tileWidth  / createInfo.quadsPerTileX;
    SizeT vertDistanceY = createInfo.tileHeight / createInfo.quadsPerTileY;

    CoreGraphics::BufferCreateInfo sysBufInfo;
    sysBufInfo.name = "VirtualSystemBuffer"_atm;
    sysBufInfo.size = 1;
    sysBufInfo.elementSize = sizeof(Terrain::TerrainSystemUniforms::STRUCT);
    sysBufInfo.mode = CoreGraphics::DeviceAndHost;
    sysBufInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    instanceInfo.systemConstants = CoreGraphics::CreateBuffer(sysBufInfo);

    CoreGraphics::BufferCreateInfo bufInfo;
    bufInfo.name = "TerrainPerPatchData"_atm;
    bufInfo.size = runtimeInfo.numTilesX * runtimeInfo.numTilesY;
    bufInfo.elementSize = sizeof(Terrain::TerrainPatches::STRUCT);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    instanceInfo.patchConstants.Create(bufInfo);

    instanceInfo.systemTable = ShaderCreateResourceTableSet(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);

    instanceInfo.systemTable.ForEach([&instanceInfo](const ResourceTableId table, IndexT i)
    {
        ResourceTableSetRWBuffer(
            table
            , ResourceTableBuffer(instanceInfo.patchConstants.buffers[i], Terrain::TerrainPatches::BINDING)
        );

        ResourceTableCommitChanges(table);
    });

    instanceInfo.runtimeTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);

    bufInfo.name = "TerrainInstanceUniforms"_atm;
    bufInfo.size = 1;
    bufInfo.elementSize = sizeof(Terrain::TerrainInstanceUniforms::STRUCT);
    bufInfo.mode = CoreGraphics::DeviceAndHost;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
    instanceInfo.runtimeConstants = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "TerrainTileWrites"_atm;
    bufInfo.size = Terrain::MAX_TILES_PER_FRAME;
    bufInfo.elementSize = sizeof(TerrainTileWrite::TileWrites::STRUCT);
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
    instanceInfo.tileWriteBufferSet.Create(bufInfo);

    uint subTextureCount = (createInfo.width / SubTextureWorldSize) * (createInfo.height / SubTextureWorldSize);

    bufInfo.name = "TerrainSubTextureBuffer"_atm;
    bufInfo.size = subTextureCount;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTextures::STRUCT);
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferDestination;
    instanceInfo.subTextureBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "TerrainSubTextureStagingBuffer"_atm;
    bufInfo.size = subTextureCount;
    bufInfo.elementSize = sizeof(Terrain::TerrainSubTextures::STRUCT);
    bufInfo.mode = BufferAccessMode::HostLocal;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
    instanceInfo.subtextureStagingBuffers.Create(bufInfo);

    CoreGraphics::TextureCreateInfo texInfo;
    texInfo.name = "TerrainIndirectionTexture"_atm;
    texInfo.width = IndirectionTextureSize;
    texInfo.height = IndirectionTextureSize;
    texInfo.format = CoreGraphics::PixelFormat::R32;
    texInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination | CoreGraphics::TextureUsage::TransferSource;
    texInfo.mips = IndirectionNumMips;
    texInfo.clear = true;
    texInfo.clearColorU4 = Math::uint4{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
    instanceInfo.indirectionTexture = CoreGraphics::CreateTexture(texInfo);

    texInfo.name = "TerrainIndirectionTextureCopy"_atm;
    instanceInfo.indirectionTextureCopy = CoreGraphics::CreateTexture(texInfo);

    uint offset = 0;

    // setup offset and mip size variables
    instanceInfo.indirectionMipOffsets.Resize(IndirectionNumMips);
    instanceInfo.indirectionMipSizes.Resize(IndirectionNumMips);
    for (uint i = 0; i < IndirectionNumMips; i++)
    {
        uint width = IndirectionTextureSize >> i;
        uint height = IndirectionTextureSize >> i;

        instanceInfo.indirectionMipSizes[i] = width;
        instanceInfo.indirectionMipOffsets[i] = offset;
        offset += width * height;
    }

    instanceInfo.indirectionBuffer.Resize(offset);
    instanceInfo.indirectionBuffer.Fill(IndirectionEntry{ 0xF, 0x3FFF, 0x3FFF });

    bufInfo.name = "TerrainUploadBuffer"_atm;
    bufInfo.elementSize = sizeof(IndirectionEntry);
    bufInfo.size = IndirectionTextureSize * IndirectionTextureSize;
    bufInfo.mode = BufferAccessMode::HostLocal;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;
    instanceInfo.indirectionUploadBuffers.Create(bufInfo);
    instanceInfo.indirectionUploadOffsets.Resize(CoreGraphics::GetNumBufferedFrames());
    instanceInfo.indirectionUploadOffsets.Fill(0x0);


    // clear the buffer, the subsequent fills are to clear the buffers
    Util::FixedArray<Terrain::PageStatuses::STRUCT> pageStatusInit(offset);
    Memory::Clear(pageStatusInit.Begin(), pageStatusInit.ByteSize());

    bufInfo.name = "TerrainPageStatusBuffer"_atm;
    bufInfo.elementSize = sizeof(Terrain::PageStatuses::STRUCT);
    bufInfo.size = offset;
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferDestination;
    bufInfo.data = pageStatusInit.Begin();
    bufInfo.dataSize = pageStatusInit.ByteSize();
    instanceInfo.pageStatusBuffer = CoreGraphics::CreateBuffer(bufInfo);


    Util::FixedArray<Terrain::PageUpdateList::STRUCT> pageUpdateInit(1);
    Memory::Clear(pageUpdateInit.Begin(), pageUpdateInit.ByteSize());

    bufInfo.name = "TerrainPageUpdateListBuffer"_atm;
    bufInfo.elementSize = sizeof(Terrain::PageUpdateList::STRUCT);
    bufInfo.size = 1;
    bufInfo.mode = BufferAccessMode::DeviceLocal;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite | CoreGraphics::BufferUsage::TransferSource;
    bufInfo.data = pageUpdateInit.Begin();
    bufInfo.dataSize = pageUpdateInit.ByteSize();
    instanceInfo.pageUpdateListBuffer = CoreGraphics::CreateBuffer(bufInfo);

    bufInfo.name = "TerrainPageUpdateReadbackBuffer"_atm;
    bufInfo.elementSize = sizeof(Terrain::PageUpdateList::STRUCT);
    bufInfo.size = 1;
    bufInfo.usageFlags = CoreGraphics::BufferUsage::TransferDestination;
    bufInfo.mode = BufferAccessMode::HostCached;
    bufInfo.data = nullptr;
    bufInfo.dataSize = 0;

    CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer("Terrain Page Setup");
    instanceInfo.pageUpdateReadbackBuffers.Create(bufInfo);
    for (IndexT i = 0; i < instanceInfo.pageUpdateReadbackBuffers.buffers.Size(); i++)
    {
        CoreGraphics::BufferFill(cmdBuf, instanceInfo.pageUpdateReadbackBuffers.buffers[i], 0x0);
    }

    // we're done
    CoreGraphics::UnlockGraphicsSetupCommandBuffer(cmdBuf);

    CoreGraphics::TextureCreateInfo albedoCacheInfo;
    albedoCacheInfo.name = "Terrain Cache Albedo"_atm;
    albedoCacheInfo.width = PhysicalTexturePaddedSize;
    albedoCacheInfo.height = PhysicalTexturePaddedSize;
    albedoCacheInfo.format = CoreGraphics::PixelFormat::BC3;
    albedoCacheInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite;
    albedoCacheInfo.allowCast = true;
    instanceInfo.physicalAlbedoCacheBC = CoreGraphics::CreateTexture(albedoCacheInfo);

    CoreGraphics::TextureViewCreateInfo albedoCacheWriteInfo;
    albedoCacheWriteInfo.name = "Terrain Cache Albedo Write"_atm;
    albedoCacheWriteInfo.tex = instanceInfo.physicalAlbedoCacheBC;
    albedoCacheWriteInfo.format = CoreGraphics::PixelFormat::R32G32B32A32;
    albedoCacheWriteInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    instanceInfo.physicalAlbedoCacheBCWrite = CoreGraphics::CreateTextureView(albedoCacheWriteInfo);

    CoreGraphics::TextureCreateInfo normalCacheInfo;
    normalCacheInfo.name = "Terrain Cache Normals"_atm;
    normalCacheInfo.width = PhysicalTexturePaddedSize;
    normalCacheInfo.height = PhysicalTexturePaddedSize;
    normalCacheInfo.format = CoreGraphics::PixelFormat::BC5;
    normalCacheInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite;
    normalCacheInfo.allowCast = true;
    instanceInfo.physicalNormalCacheBC = CoreGraphics::CreateTexture(normalCacheInfo);

    CoreGraphics::TextureViewCreateInfo normalCacheWriteInfo;
    normalCacheWriteInfo.name = "Terrain Cache Normals Write";
    normalCacheWriteInfo.format = CoreGraphics::PixelFormat::R32G32B32A32;
    normalCacheWriteInfo.tex = instanceInfo.physicalNormalCacheBC;
    normalCacheWriteInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    instanceInfo.physicalNormalCacheBCWrite = CoreGraphics::CreateTextureView(normalCacheWriteInfo);

    CoreGraphics::TextureCreateInfo materialCacheInfo;
    materialCacheInfo.name = "Terrain Cache Material"_atm;
    materialCacheInfo.width = PhysicalTexturePaddedSize;
    materialCacheInfo.height = PhysicalTexturePaddedSize;
    materialCacheInfo.format = CoreGraphics::PixelFormat::BC3;
    materialCacheInfo.usage = CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::ReadWrite;
    materialCacheInfo.allowCast = true;
    instanceInfo.physicalMaterialCacheBC = CoreGraphics::CreateTexture(materialCacheInfo);

    CoreGraphics::TextureViewCreateInfo materialCacheWriteInfo;
    materialCacheWriteInfo.name = "Terrain Cache Material Write"_atm;
    materialCacheWriteInfo.format = CoreGraphics::PixelFormat::R32G32B32A32;
    materialCacheWriteInfo.tex = instanceInfo.physicalMaterialCacheBC;
    materialCacheWriteInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    instanceInfo.physicalMaterialCacheBCWrite = CoreGraphics::CreateTextureView(materialCacheWriteInfo);

    CoreGraphics::TextureCreateInfo lowResAlbedoInfo;
    lowResAlbedoInfo.name = "Terrain Lowres Albedo"_atm;
    lowResAlbedoInfo.mips = LowresFallbackMips;
    lowResAlbedoInfo.width = LowresFallbackSize;
    lowResAlbedoInfo.height = LowresFallbackSize;
    lowResAlbedoInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResAlbedoInfo.usage = CoreGraphics::TextureUsage::ReadWrite | CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination | CoreGraphics::TextureUsage::TransferSource;
    instanceInfo.lowresAlbedo = CoreGraphics::CreateTexture(lowResAlbedoInfo);

    CoreGraphics::TextureCreateInfo lowResNormalInfo;
    lowResNormalInfo.name = "Terrain Lowres Normal"_atm;
    lowResNormalInfo.mips = LowresFallbackMips;
    lowResNormalInfo.width = LowresFallbackSize;
    lowResNormalInfo.height = LowresFallbackSize;
    lowResNormalInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResNormalInfo.usage = CoreGraphics::TextureUsage::ReadWrite | CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination | CoreGraphics::TextureUsage::TransferSource;
    instanceInfo.lowresNormal = CoreGraphics::CreateTexture(lowResNormalInfo);

    CoreGraphics::TextureCreateInfo lowResMaterialInfo;
    lowResMaterialInfo.name = "Terrain Lowres Material"_atm;
    lowResMaterialInfo.mips = LowresFallbackMips;
    lowResMaterialInfo.width = LowresFallbackSize;
    lowResMaterialInfo.height = LowresFallbackSize;
    lowResMaterialInfo.format = CoreGraphics::PixelFormat::R8G8B8A8;
    lowResMaterialInfo.usage = CoreGraphics::TextureUsage::ReadWrite | CoreGraphics::TextureUsage::Sample | CoreGraphics::TextureUsage::TransferDestination | CoreGraphics::TextureUsage::TransferSource;
    instanceInfo.lowresMaterial = CoreGraphics::CreateTexture(lowResMaterialInfo);

    CoreGraphics::TextureCreateInfo shadowMapInfo;
    shadowMapInfo.name = "TerrainShadowMap"_atm;
    shadowMapInfo.width = TerrainShadowMapSize;
    shadowMapInfo.height = TerrainShadowMapSize;
    shadowMapInfo.format = CoreGraphics::PixelFormat::R16G16F;
    shadowMapInfo.usage = CoreGraphics::TextureUsage::ReadWrite;
    instanceInfo.shadowMap = CoreGraphics::CreateTexture(shadowMapInfo);
    Lighting::LightContext::SetupTerrainShadows(instanceInfo.shadowMap, createInfo.width);

    // setup virtual sub textures buffer
    instanceInfo.subTextures.Resize(subTextureCount);
    instanceInfo.gpuSubTextures.Resize(subTextureCount);

    uint subTextureCountX = createInfo.width / SubTextureWorldSize;
    uint subTextureCountZ = createInfo.height / SubTextureWorldSize;
    for (uint z = 0; z < subTextureCountZ; z++)
    {
        for (uint x = 0; x < subTextureCountX; x++)
        {
            uint index = x + z * subTextureCountX;
            instanceInfo.subTextures[index].numTiles = 0;
            instanceInfo.subTextures[index].maxMip = 0;
            instanceInfo.subTextures[index].indirectionOffset.x = UINT32_MAX;
            instanceInfo.subTextures[index].indirectionOffset.y = UINT32_MAX;
            instanceInfo.subTextures[index].mipBias = 0;

            // position is calculated as the center of each cell, offset by half of the world size (so we are oriented around 0)
            float xPos = x * SubTextureWorldSize - createInfo.width / 2.0f;
            float zPos = z * SubTextureWorldSize - createInfo.height / 2.0f;
            instanceInfo.subTextures[index].worldCoordinate.x = xPos;
            instanceInfo.subTextures[index].worldCoordinate.y = zPos;
            instanceInfo.gpuSubTextures[index].worldCoordinate[0] = xPos;
            instanceInfo.gpuSubTextures[index].worldCoordinate[1] = zPos;
            PackSubTexture(instanceInfo.subTextures[index], instanceInfo.gpuSubTextures[index]);
        }
    }

    // setup indirection occupancy, the indirection texture is 2048, and the maximum allocation size is 256 indirection pixels
    instanceInfo.indirectionOccupancy.Setup(IndirectionTextureSize, 256, 1);

    

    // setup the physical texture occupancy, the texture is 8192x8192 pixels, and the page size is 256, so effectively make this a quad tree that ends at individual pages
    //terrainVirtualTileState.physicalTextureTileOccupancy.Setup(PhysicalTexturePaddedSize, PhysicalTexturePaddedSize, PhysicalTextureTilePaddedSize);
    instanceInfo.physicalTextureTileCache.Setup(PhysicalTextureTilePaddedSize, PhysicalTexturePaddedSize);

    instanceInfo.systemTable.ForEach([&instanceInfo](const ResourceTableId table, const IndexT i)
    {
        ResourceTableSetConstantBuffer(table,
            {
                terrainState.biomeBuffer,
                Terrain::MaterialLayers::BINDING,
                NEBULA_WHOLE_BUFFER_SIZE,
            }
            );

        ResourceTableSetRWTexture(table,
            {
                instanceInfo.shadowMap,
                Terrain::TerrainShadowMap::BINDING,
            });

        ResourceTableSetConstantBuffer(table,
            {
                instanceInfo.systemConstants,
                Terrain::TerrainSystemUniforms::BINDING,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                instanceInfo.subTextureBuffer,
                Terrain::TerrainSubTextures::BINDING,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                instanceInfo.pageUpdateListBuffer,
                Terrain::PageUpdateList::BINDING,
                NEBULA_WHOLE_BUFFER_SIZE,
            });

        ResourceTableSetRWBuffer(table,
            {
                instanceInfo.pageStatusBuffer,
                Terrain::PageStatuses::BINDING,
                NEBULA_WHOLE_BUFFER_SIZE,
            });


        ResourceTableCommitChanges(table);
    });


    ResourceTableSetConstantBuffer(instanceInfo.runtimeTable,
        {
            instanceInfo.runtimeConstants,
            Terrain::TerrainInstanceUniforms::BINDING,
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.physicalAlbedoCacheBCWrite,
            Terrain::AlbedoCacheOutputBC::BINDING
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.physicalNormalCacheBCWrite,
            Terrain::NormalCacheOutputBC::BINDING
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.physicalMaterialCacheBCWrite,
            Terrain::MaterialCacheOutputBC::BINDING
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.lowresAlbedo,
            Terrain::AlbedoLowresOutput::BINDING
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.lowresNormal,
            Terrain::NormalLowresOutput::BINDING
        });

    ResourceTableSetRWTexture(instanceInfo.runtimeTable,
        {
            instanceInfo.lowresMaterial,
            Terrain::MaterialLowresOutput::BINDING
        });

    ResourceTableSetRWBuffer(instanceInfo.runtimeTable,
        {
            instanceInfo.tileWriteBufferSet.DeviceBuffer(),
            Terrain::TileWrites::BINDING
        });

    ResourceTableCommitChanges(instanceInfo.runtimeTable);

    // allocate a tile vertex buffer
    Util::FixedArray<TerrainVert> verts(numVertsX * numVertsY);

    // allocate terrain index buffer, every fourth pixel will generate two triangles 
    SizeT numTris = numVertsX * numVertsY;
    Util::FixedArray<TerrainQuad> quads(numQuadsX * numQuadsY);
    Util::FixedArray<Terrain::TerrainPatchData> patchData(runtimeInfo.numTilesY * runtimeInfo.numTilesX);

    // setup sections
    uint patchCounter = 0;
    for (uint y = 0; y < runtimeInfo.numTilesY; y++)
    {
        for (uint x = 0; x < runtimeInfo.numTilesX; x++)
        {
            Math::bbox box;
            box.set(
                Math::point(
                    x * createInfo.tileWidth - createInfo.width / 2 + createInfo.tileWidth / 2,
                createInfo.minHeight,
                    y * createInfo.tileHeight - createInfo.height / 2 + createInfo.tileHeight / 2
                ),
                Math::vector(createInfo.tileWidth / 2, height, createInfo.tileHeight / 2));
            runtimeInfo.sectionBoxes.Append(box);
            runtimeInfo.sectorVisible.Append(true);
            runtimeInfo.sectorLod.Append(1000.0f);
            runtimeInfo.sectorUpdateTextureTile.Append(false);
            runtimeInfo.sectorUniformOffsets.Append(Util::FixedArray<uint>(2, 0));
            runtimeInfo.sectorTileOffsets.Append(Util::FixedArray<uint>(2, 0));
            runtimeInfo.sectorAllocatedTile.Append(Math::uint3{ UINT32_MAX, UINT32_MAX, UINT32_MAX });
            runtimeInfo.sectorTextureTileSize.Append(Math::uint2{ 0, 0 });
            runtimeInfo.sectorUv.Append({ x * createInfo.tileWidth / createInfo.width, y * createInfo.tileHeight / createInfo.height });

            if (CoreGraphics::RayTracingSupported)
            {
                Terrain::TerrainPatchData patch;
                patch.PosOffset[0] = box.pmin.x;
                patch.PosOffset[1] = box.pmin.z;
                patch.UvOffset[0] = runtimeInfo.sectorUv.Back().x;
                patch.UvOffset[1] = runtimeInfo.sectorUv.Back().y;
                patchData[patchCounter++] = patch;
            }
        }
    }

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
            v1.uv = Math::vec2(x * vertDistanceX / float(createInfo.tileWidth), y * vertDistanceY / float(createInfo.tileHeight));

            v2.position.set((x + 1) * vertDistanceX, 0, y * vertDistanceY, 1);
            v2.uv = Math::vec2((x + 1) * vertDistanceX / float(createInfo.tileWidth), y * vertDistanceY / float(createInfo.tileHeight));

            v3.position.set(x * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
            v3.uv = Math::vec2(x * vertDistanceX / float(createInfo.tileWidth), (y + 1) * vertDistanceY / float(createInfo.tileHeight));

            v4.position.set((x + 1) * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
            v4.uv = Math::vec2((x + 1) * vertDistanceX / float(createInfo.tileWidth), (y + 1) * vertDistanceY / float(createInfo.tileHeight));

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
            vt1.uv[0] = uint(v1.uv.x * 1000);
            vt1.uv[1] = uint(v1.uv.y * 1000);

            TerrainVert& vt2 = verts[vidx2];
            v2.position.storeu3(vt2.position.v);
            vt2.uv[0] = uint(v2.uv.x * 1000);
            vt2.uv[1] = uint(v2.uv.y * 1000);

            TerrainVert& vt3 = verts[vidx3];
            v3.position.storeu3(vt3.position.v);
            vt3.uv[0] = uint(v3.uv.x * 1000);
            vt3.uv[1] = uint(v3.uv.y * 1000);

            TerrainVert& vt4 = verts[vidx4];
            v4.position.storeu3(vt4.position.v);
            vt4.uv[0] = uint(v4.uv.x * 1000);
            vt4.uv[1] = uint(v4.uv.y * 1000);

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
    vboInfo.usageFlags = CoreGraphics::BufferUsage::Vertex | (runtimeInfo.enableRayTracing ? CoreGraphics::BufferUsage::ReadWrite : CoreGraphics::BufferUsage(0x0));
    vboInfo.data = verts.Begin();
    vboInfo.dataSize = verts.ByteSize();
    runtimeInfo.vbo = CreateBuffer(vboInfo);

    // create ibo
    BufferCreateInfo iboInfo;
    iboInfo.name = "terrain_ibo"_atm;
    iboInfo.size = numTris * 4;
    iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
    iboInfo.mode = CoreGraphics::DeviceLocal;
    iboInfo.usageFlags = CoreGraphics::BufferUsage::Index | (runtimeInfo.enableRayTracing ? CoreGraphics::BufferUsage::ReadWrite : CoreGraphics::BufferUsage(0x0));
    iboInfo.data = quads.Begin();
    iboInfo.dataSize = quads.ByteSize();
    runtimeInfo.ibo = CreateBuffer(iboInfo);



    // Set up extra vertex buffers to produce raytracing proxy if raytracing is enabled
    if (runtimeInfo.enableRayTracing)
    {
        CoreGraphics::BufferCreateInfo patchBufferInfo;
        patchBufferInfo.byteSize = patchData.ByteSize();
        patchBufferInfo.usageFlags = CoreGraphics::BufferUsage::ReadWrite;
        patchBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceLocal;
        patchBufferInfo.data = patchData.Begin();
        patchBufferInfo.dataSize = patchData.ByteSize();
        raytracingState.patchBuffer = CoreGraphics::CreateBuffer(patchBufferInfo);

        CoreGraphics::BufferCreateInfo constantBufferInfo;
        constantBufferInfo.byteSize = patchData.ByteSize();
        constantBufferInfo.usageFlags = CoreGraphics::BufferUsage::ConstantBuffer;
        constantBufferInfo.mode = CoreGraphics::BufferAccessMode::DeviceAndHost;
        raytracingState.constantsBuffer = CoreGraphics::CreateBuffer(constantBufferInfo);


        // Create index buffer for triangles used by raytracing
        Util::Array<IndexT> indices;
        uint quad = 0;
        for (IndexT y = 0; y < numQuadsY; y++)
        {
            for (IndexT x = 0; x < numQuadsX; x++)
            {
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

                // setup triangle tris
                indices.Append(vidx1, vidx3, vidx4);
                indices.Append(vidx4, vidx2, vidx1);
            }
        }

        raytracingState.vertexBuffer = CoreGraphics::AllocateVertices(vboInfo.size * vboInfo.elementSize * runtimeInfo.numTilesY * runtimeInfo.numTilesX);
        raytracingState.indexBuffer = CoreGraphics::AllocateIndices(indices.Size() * IndexType::SizeOf(IndexType::Index32) * runtimeInfo.numTilesY * runtimeInfo.numTilesX);

        CoreGraphics::ResourceTableSetConstantBuffer(raytracingState.meshGenTable,
                                        CoreGraphics::ResourceTableBuffer(raytracingState.constantsBuffer, TerrainMeshGenerate::GenerationUniforms::BINDING)
        );
        CoreGraphics::ResourceTableSetRWBuffer(raytracingState.meshGenTable,
                                       CoreGraphics::ResourceTableBuffer(runtimeInfo.vbo, TerrainMeshGenerate::VertexInput::BINDING)
        );
        CoreGraphics::ResourceTableSetRWBuffer(raytracingState.meshGenTable,
                                       CoreGraphics::ResourceTableBuffer(runtimeInfo.ibo, TerrainMeshGenerate::IndexInput::BINDING)
        );
        CoreGraphics::ResourceTableSetRWBuffer(raytracingState.meshGenTable,
                                       CoreGraphics::ResourceTableBuffer(
                                               CoreGraphics::GetVertexBuffer()
                                               , TerrainMeshGenerate::VertexOutput::BINDING
                                               , raytracingState.vertexBuffer.size
                                               , raytracingState.vertexBuffer.offset
        ));

        CoreGraphics::ResourceTableSetRWBuffer(raytracingState.meshGenTable,
                                       CoreGraphics::ResourceTableBuffer(raytracingState.patchBuffer, TerrainMeshGenerate::TerrainPatches::BINDING)
        );
        CoreGraphics::ResourceTableCommitChanges(raytracingState.meshGenTable);

        CoreGraphics::BufferCreateInfo indexUploadBufferInfo;
        indexUploadBufferInfo.byteSize = indices.Size() * IndexType::SizeOf(IndexType::Index32);
        indexUploadBufferInfo.mode = CoreGraphics::BufferAccessMode::HostLocal;
        indexUploadBufferInfo.usageFlags = CoreGraphics::BufferUsage::TransferSource;
        indexUploadBufferInfo.data = indices.Begin();
        indexUploadBufferInfo.dataSize = indices.ByteSize();
        CoreGraphics::BufferId tempIndexBuffer = CoreGraphics::CreateBuffer(indexUploadBufferInfo);

        CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::LockGraphicsSetupCommandBuffer("Terrain geometry setup");

        // The index buffer is static for all patches, so let's just copy it over
        for (IndexT i = 0; i < runtimeInfo.numTilesY * runtimeInfo.numTilesX; i++)
        {
            CoreGraphics::BufferCopy fromCopy{ .offset = 0 }, toCopy{ .offset = raytracingState.indexBuffer.offset + i * (indices.ByteSize()) };
            CoreGraphics::CmdCopy(cmdBuf, tempIndexBuffer, { fromCopy }, CoreGraphics::GetIndexBuffer(), { toCopy }, indices.ByteSize());
        }
        CoreGraphics::UnlockGraphicsSetupCommandBuffer(cmdBuf);
        CoreGraphics::DestroyBuffer(tempIndexBuffer);

        // Prepare to run a compute shader to displace every vertex based on the height map
        raytracingState.numPatches = runtimeInfo.numTilesY * runtimeInfo.numTilesX;
        raytracingState.numTris = numQuadsY * numQuadsX * 2;

        Graphics::RegisterEntity<Raytracing::RaytracingContext>(entity);

        CoreGraphics::PrimitiveGroup group;
        group.SetBaseIndex(0);
        group.SetBaseVertex(0);
        group.SetNumIndices(indices.Size());
        group.SetNumVertices(0);

        MaterialInterfaces::TerrainMaterial mat;
        mat.LowresAlbedoFallback = CoreGraphics::TextureGetBindlessHandle(instanceInfo.lowresAlbedo);
        mat.LowresMaterialFallback = CoreGraphics::TextureGetBindlessHandle(instanceInfo.lowresMaterial);
        mat.LowresNormalFallback = CoreGraphics::TextureGetBindlessHandle(instanceInfo.lowresNormal);

        /// Setup with raytracing
        Util::Array<Math::mat4> patchTransforms;
        patchTransforms.Resize(runtimeInfo.sectionBoxes.Size());
        uint patchCounter = 0;
        for (uint y = 0; y < runtimeInfo.numTilesY; y++)
        {
            for (uint x = 0; x < runtimeInfo.numTilesX; x++)
            {
                Math::point pt(
                    x * createInfo.tileWidth - createInfo.width / 2 + createInfo.tileWidth / 2,
                    createInfo.minHeight,
                    y * createInfo.tileHeight - createInfo.height / 2 + createInfo.tileHeight / 2
                );

                patchTransforms[patchCounter++].translate(xyz(pt));
            }
        }

        uint materialOffset = Materials::MaterialLoader::RegisterTerrainMaterial(mat);
        Raytracing::RaytracingContext::SetupMesh(
            entity
            , Raytracing::UpdateType::Static
            , CoreGraphics::VertexComponent::Float3
            , CoreGraphics::IndexType::Index32
            , raytracingState.vertexBuffer
            , raytracingState.indexBuffer
            , group
            , vboInfo.elementSize
            , verts.ByteSize()
            , patchTransforms
            , materialOffset
            , MaterialTemplates::MaterialProperties::Terrain
            , CoreGraphics::VertexLayoutType::Normal);

        // Invalidate the BLAS when the compute shader has displaced all terrain patches
        raytracingState.terrainSetupCallback = [entity]()
        {
            Raytracing::RaytracingContext::InvalidateBLAS(entity);
        };
        raytracingState.setupBlasFrame = 0xFFFFFFFF;
    }

    runtimeInfo.loadBits = 0x0;
    runtimeInfo.lowresGenerated = false;
    runtimeInfo.decisionMapResource = Resources::CreateResource(createInfo.decisionMap, "terrain"_atm, [&runtimeInfo](Resources::ResourceId id)
    {
        runtimeInfo.decisionMapResource = id;
        runtimeInfo.decisionMap = TextureId(runtimeInfo.decisionMapResource);
        runtimeInfo.lowresGenerated = false;
        runtimeInfo.loadBits |= TerrainRuntimeInfo::DecisionMapLoaded;
    }, nullptr, false, false);
    runtimeInfo.decisionMap = TextureId(runtimeInfo.decisionMapResource);

    runtimeInfo.heightMapResource = Resources::CreateResource(createInfo.heightMap, "terrain"_atm, [&runtimeInfo, numVertsX, numVertsY](Resources::ResourceId id)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        runtimeInfo.heightMapResource = id;
        runtimeInfo.heightMap = TextureId(id);
        runtimeInfo.lowresGenerated = false;
        runtimeInfo.loadBits |= TerrainRuntimeInfo::HeightMapLoaded;
        terrainState.shadowMapInvalid = true;

        // If we are using raytracing, trigger a raytracing mesh update
        if (runtimeInfo.enableRayTracing)
        {
            raytracingState.updateMesh = true;

            TerrainMeshGenerate::GenerationUniforms::STRUCT generationConstants;
            Math::mat4 transform;
            transform.store(&generationConstants.Transform[0][0]);
            generationConstants.MinHeight = runtimeInfo.minHeight;
            generationConstants.MaxHeight = runtimeInfo.maxHeight;
            generationConstants.VerticesPerPatch = numVertsX * numVertsY;
            generationConstants.WorldSize[0] = runtimeInfo.worldWidth;
            generationConstants.WorldSize[1] = runtimeInfo.worldHeight;
            generationConstants.HeightMap = CoreGraphics::TextureGetBindlessHandle(runtimeInfo.heightMap);
            CoreGraphics::BufferUpdate(raytracingState.constantsBuffer, generationConstants);
        }
    }, nullptr, false, false);
    runtimeInfo.heightMap = TextureId(runtimeInfo.heightMapResource);
}

//------------------------------------------------------------------------------
/**
*/
TerrainBiomeId 
TerrainContext::CreateBiome(const BiomeSettings& settings)
{
    Ids::Id32 ret = terrainBiomeAllocator.Alloc();
    terrainBiomeAllocator.Set<TerrainBiome_Settings>(ret, settings);

    IndexT biomeIndex = terrainState.biomeCounter;
    terrainState.biomeResources[biomeIndex][BiomeSettings::BiomeMaterialLayer::Flat] = settings.materials[BiomeSettings::BiomeMaterialLayer::Flat];
    terrainState.biomeResources[biomeIndex][BiomeSettings::BiomeMaterialLayer::Slope] = settings.materials[BiomeSettings::BiomeMaterialLayer::Slope];
    terrainState.biomeResources[biomeIndex][BiomeSettings::BiomeMaterialLayer::Height] = settings.materials[BiomeSettings::BiomeMaterialLayer::Height];
    terrainState.biomeResources[biomeIndex][BiomeSettings::BiomeMaterialLayer::HeightSlope] = settings.materials[BiomeSettings::BiomeMaterialLayer::HeightSlope];
    for (int i = 0; i < BiomeSettings::BiomeMaterialLayer::NumLayers; i++)
    {
        terrainState.biomeLoaded[biomeIndex][i] = 0x0;
        terrainState.biomeLowresGenerated[i] = false;
        terrainState.biomeResources[biomeIndex][i].albedoRes = Resources::CreateResource(terrainState.biomeResources[biomeIndex][i].albedo.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            Threading::CriticalScope scope(&terrainState.syncPoint);
            CoreGraphics::TextureIdLock _0(id);
            terrainState.biomeMaterials.MaterialAlbedos[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::AlbedoLoaded;
        }, nullptr, false, false);

        terrainState.biomeResources[biomeIndex][i].normalRes = Resources::CreateResource(terrainState.biomeResources[biomeIndex][i].normal.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            Threading::CriticalScope scope(&terrainState.syncPoint);
            CoreGraphics::TextureIdLock _0(id);
            terrainState.biomeMaterials.MaterialNormals[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::NormalLoaded;
        }, nullptr, false, false);

        terrainState.biomeResources[biomeIndex][i].materialRes = Resources::CreateResource(terrainState.biomeResources[biomeIndex][i].material.Value(), "terrain", [i, biomeIndex](Resources::ResourceId id)
        {
            Threading::CriticalScope scope(&terrainState.syncPoint);
            CoreGraphics::TextureIdLock _0(id);
            terrainState.biomeMaterials.MaterialPBRs[biomeIndex][i] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::MaterialLoaded;
        }, nullptr, false, false);
    }

    Resources::CreateResource(settings.biomeMask, "terrain", [biomeIndex](Resources::ResourceId id)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        CoreGraphics::TextureIdLock _0(id);
        terrainState.biomeMaterials.MaterialMasks[biomeIndex / 4][biomeIndex % 4] = CoreGraphics::TextureGetBindlessHandle(id);
        terrainState.biomeTextures.Append(id);
        terrainState.biomeMasks[terrainState.biomeCounter] = id;
        terrainState.biomeLowresGenerated[biomeIndex] = false;
        for (int i = 0; i < 4; i++)
            terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::MaskLoaded;
    }, nullptr, false, false);

    if (settings.biomeParameters.useMaterialWeights)
    {
        Resources::CreateResource(settings.biomeParameters.weights, "terrain", [biomeIndex](Resources::ResourceId id)
        {
            Threading::CriticalScope scope(&terrainState.syncPoint);
            CoreGraphics::TextureIdLock _0(id);
            terrainState.biomeMaterials.MaterialWeights[biomeIndex / 4][biomeIndex % 4] = CoreGraphics::TextureGetBindlessHandle(id);
            terrainState.biomeTextures.Append(id);
            terrainState.biomeWeights[terrainState.biomeCounter] = id;
            terrainState.biomeLowresGenerated[biomeIndex] = false;
            for (int i = 0; i < 4; i++)
                terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::WeightsLoaded;
        }, nullptr, false, false);
    }
    else
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        for (int i = 0; i < 4; i++)
            terrainState.biomeLoaded[biomeIndex][i] |= BiomeLoadBits::WeightsLoaded;
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
TerrainContext::DestroyBiome(TerrainBiomeId id)
{
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
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
    Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
    {
        TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
        Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetView(view->GetCamera()));
        terrainInstance.indirectionUploadOffsets[ctx.bufferIndex] = 0;

        if (raytracingState.setupBlasFrame == ctx.frameIndex)
            raytracingState.terrainSetupCallback();

        n_assert(terrainInstance.subtexturesDoneCounter == 0);
        terrainInstance.subtexturesDoneCounter = 1;
        terrainInstance.subTextureNumOutputs = 0;

        SubTextureUpdateJobUniforms uniforms;
        uniforms.maxMip = IndirectionNumMips - 1;
        uniforms.physicalTileSize = PhysicalTextureTileSize;
        uniforms.subTextureWorldSize = SubTextureWorldSize;

        Jobs2::JobDispatch(
            [
                uniforms
                , &terrainInstance
                , camera = cameraTransform
            ]
            (SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
        {
            N_SCOPE(TerrainSubTextureUpdateJob, Terrain);

            // Iterate over work group
            for (IndexT i = 0; i < groupSize; i++)
            {
                // Get item index
                IndexT index = i + invocationOffset;
                if (index >= totalJobs)
                    return;

                const Terrain::SubTexture& subTexture = terrainInstance.subTextures[index];

                // mask out y coordinate by multiplying result with, 1, 0 ,1
                Math::vec4 min = Math::vec4(subTexture.worldCoordinate.x, 0, subTexture.worldCoordinate.y, 0);
                Math::vec4 max = min + Math::vec4(uniforms.subTextureWorldSize, 0.0f, uniforms.subTextureWorldSize, 0.0f);
                Math::vec4 cameraXZ = camera.position * Math::vec4(1, 0, 1, 0);
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
                lod = Math::min((uint)Math::log2(t), uniforms.maxMip);

                // calculate the resolution by offseting the max resolution with the lod
                resolution = SubTextureMaxPixels >> lod;

skipResolution:

            // calculate the amount of tiles, which is the final lodded resolution divided by the size of a tile
            // the max being maxResolution and the smallest being 1
                uint tiles = resolution / uniforms.physicalTileSize;

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
                    int outputIndex = Threading::Interlocked::Add(&terrainInstance.subTextureNumOutputs, 1);
                    n_assert(outputIndex < SubTextureMaxUpdates);

                    SubTextureUpdateJobOutput& output = terrainInstance.subTextureJobOutputs[outputIndex];
                    output.index = index;
                    output.oldMaxMip = subTexture.maxMip;
                    output.oldTiles = subTexture.numTiles;
                    output.oldCoord = subTexture.indirectionOffset;

                    output.newMaxMip = tiles > 0 ? Math::log2(tiles) : 0;
                    output.newTiles = tiles;

                    output.mipBias = output.newMaxMip > output.oldMaxMip ? output.newMaxMip - output.oldMaxMip : 0.0f;

                    // default is that the subtexture did not change
                    output.updateState = state;
                }

            }
        }, terrainInstance.subTextures.Size(), 256, {}, & terrainInstance.subtexturesDoneCounter, terrainInstance.subtexturesFinishedEvent);

        n_assert(terrainInstance.sectionCullDoneCounter == 0);
        terrainInstance.sectionCullDoneCounter = 1;
        const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());

        Math::vec4 m_col_x[4];
        Math::vec4 m_col_y[4];
        Math::vec4 m_col_z[4];
        Math::vec4 m_col_w[4];
        m_col_x[0] = Math::splat_x(viewProj.r[0]);
        m_col_x[1] = Math::splat_x(viewProj.r[1]);
        m_col_x[2] = Math::splat_x(viewProj.r[2]);
        m_col_x[3] = Math::splat_x(viewProj.r[3]);

        m_col_y[0] = Math::splat_y(viewProj.r[0]);
        m_col_y[1] = Math::splat_y(viewProj.r[1]);
        m_col_y[2] = Math::splat_y(viewProj.r[2]);
        m_col_y[3] = Math::splat_y(viewProj.r[3]);

        m_col_z[0] = Math::splat_z(viewProj.r[0]);
        m_col_z[1] = Math::splat_z(viewProj.r[1]);
        m_col_z[2] = Math::splat_z(viewProj.r[2]);
        m_col_z[3] = Math::splat_z(viewProj.r[3]);

        m_col_w[0] = Math::splat_w(viewProj.r[0]);
        m_col_w[1] = Math::splat_w(viewProj.r[1]);
        m_col_w[2] = Math::splat_w(viewProj.r[2]);
        m_col_w[3] = Math::splat_w(viewProj.r[3]);

        terrainInstance.numPatchesThisFrame = 0;

        terrainInstance.sectionCullDoneCounter = runtimes.Size();
        for (IndexT i = 0; i < runtimes.Size(); i++)
        {
            TerrainRuntimeInfo& rt = runtimes[i];
            Jobs2::JobDispatch(
                [
                    boundingBoxes = rt.sectionBoxes.ConstBegin()
                        , visibilities = rt.sectorVisible.Begin()
                        , instanceCounter = &terrainInstance.numPatchesThisFrame
                        , patchData = (Terrain::TerrainPatchData*)CoreGraphics::BufferMap(terrainInstance.patchConstants.buffers[ctx.bufferIndex])
                        , m_col_x
                        , m_col_y
                        , m_col_z
                        , m_col_w
                ]
            (SizeT totalJobs, SizeT groupSize, IndexT groupIndex, SizeT invocationOffset)
            {
                N_SCOPE(BruteforceViewFrustumCulling, Visibility);

                // Iterate over work group
                for (IndexT i = 0; i < groupSize; i++)
                {
                    // Get item index
                    IndexT index = i + invocationOffset;
                    if (index >= totalJobs)
                        return;

                    const Math::ClipStatus::Type clip = boundingBoxes[index].clipstatus(m_col_x, m_col_y, m_col_z, m_col_w);
                    if (clip != Math::ClipStatus::Outside)
                    {
                        uint offset = Threading::Interlocked::Add(instanceCounter, 1);

                        Terrain::TerrainPatchData patch;
                        patch.PosOffset[0] = boundingBoxes[index].pmin.x;
                        patch.PosOffset[1] = boundingBoxes[index].pmin.z;
                        patch.UvOffset[0] = 0.0f;
                        patch.UvOffset[1] = 0.0f;
                        patchData[offset] = patch;
                        visibilities[index] = true;
                    }
                    else
                    {
                        visibilities[index] = false;
                    }
                }
            }, rt.sectionBoxes.Size(), 256, {}, & terrainInstance.sectionCullDoneCounter, terrainInstance.sectionCullFinishedEvent);
            CoreGraphics::BufferUnmap(terrainInstance.patchConstants.buffers[ctx.bufferIndex]);
        }
        if (runtimes.IsEmpty())
            terrainInstance.sectionCullFinishedEvent->Signal();
    }
    
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
Upload(TerrainContext::TerrainInstanceInfo& instance, T* data, uint size, uint alignment)
{
    uint& offset = instance.indirectionUploadOffsets[CoreGraphics::GetBufferedFrameIndex()];
    uint uploadOffset = Math::align(offset, alignment);
    CoreGraphics::BufferUpdateArray(instance.indirectionUploadBuffers.Buffer(), data, size, offset);
    offset = uploadOffset + size * sizeof(T);
    return uploadOffset;
}

//------------------------------------------------------------------------------
/**
*/
void
IndirectionUpdate(
    TerrainContext::TerrainInstanceInfo& instance
    , uint mip
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

    instance.indirectionEntryUpdates.Append(entry);
    instance.indirectionTextureCopies.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(indirectionPixelX, indirectionPixelY, indirectionPixelX + 1, indirectionPixelY + 1), (uint)mip, 0 });

    uint mipOffset = instance.indirectionMipOffsets[mip];
    uint size = instance.indirectionMipSizes[mip];
    instance.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * size] = entry;
}

//------------------------------------------------------------------------------
/**
*/
void
IndirectionErase(
    TerrainContext::TerrainInstanceInfo& instance
    , uint mip
    , uint indirectionOffsetX
    , uint indirectionOffsetY
    , uint subTextureTileX
    , uint subTextureTileY
)
{
    IndirectionEntry entry{ 0xF, 0x3FFF, 0x3FFF };

    // Calculate indirection pixel in subtexture
    uint indirectionPixelX = (indirectionOffsetX >> mip) + subTextureTileX;
    uint indirectionPixelY = (indirectionOffsetY >> mip) + subTextureTileY;

    instance.indirectionEntryUpdates.Append(entry);
    instance.indirectionTextureCopies.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(indirectionPixelX, indirectionPixelY, indirectionPixelX + 1, indirectionPixelY + 1), (uint)mip, 0 });

    uint mipOffset = instance.indirectionMipOffsets[mip];
    uint size = instance.indirectionMipSizes[mip];
    instance.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * size] = entry;
}

//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is bigger by mapping mips 0..X to 1..X in the new region
*/
void
IndirectionMoveGrow(
    TerrainContext::TerrainInstanceInfo& instance
    , uint oldMaxMip
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

        instance.indirectionTextureFromCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), i, 0 });
        instance.indirectionTextureToCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), newMip, 0 });

        // Update CPU buffer
        uint fromMipOffset = instance.indirectionMipOffsets[i];
        uint fromSize = instance.indirectionMipSizes[i];
        uint fromStart = mippedOldCoord.x + mippedOldCoord.y * fromSize;
        uint toMipOffset = instance.indirectionMipOffsets[newMip];
        uint toSize = instance.indirectionMipSizes[newMip];
        uint toStart = mippedNewCoord.x + mippedNewCoord.y * toSize;

        for (uint j = 0; j < width; j++)
        {
            uint fromRowOffset = fromSize * j;
            uint toRowOffset = toSize * j;
            memmove(instance.indirectionBuffer.Begin() + toMipOffset + toRowOffset + toStart, instance.indirectionBuffer.Begin() + fromMipOffset + fromRowOffset + fromStart, dataSize);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Copies mip chain from old region to new region which is smaller, mapping mips 0..X to 0..X-1 in the new region
*/
void
IndirectionMoveShrink(
    TerrainContext::TerrainInstanceInfo& instance
    , uint oldMaxMip
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

        instance.indirectionTextureFromCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedOldCoord.x, mippedOldCoord.y, mippedOldCoord.x + width, mippedOldCoord.y + width), oldMip, 0 });
        instance.indirectionTextureToCopiesThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedNewCoord.x, mippedNewCoord.y, mippedNewCoord.x + width, mippedNewCoord.y + width), i, 0 });

        // Update CPU buffer
        uint fromMipOffset = instance.indirectionMipOffsets[oldMip];
        uint fromSize = instance.indirectionMipSizes[oldMip];
        uint fromStart = mippedOldCoord.x + mippedOldCoord.y * fromSize;
        uint toMipOffset = instance.indirectionMipOffsets[i];
        uint toSize = instance.indirectionMipSizes[i];
        uint toStart = mippedNewCoord.x + mippedNewCoord.y * toSize;

        for (uint j = 0; j < width; j++)
        {
            uint fromRowOffset = fromSize * j;
            uint toRowOffset = toSize * j;
            memmove(instance.indirectionBuffer.Begin() + toMipOffset + toRowOffset + toStart, instance.indirectionBuffer.Begin() + fromMipOffset + fromRowOffset + fromStart, dataSize);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Clear old region
*/
void
IndirectionClear(
    TerrainContext::TerrainInstanceInfo& instance
    , uint mips
    , uint tiles
    , const Math::uint2& coord)
{
    uint width = tiles;
    uint dataSize = width * width * sizeof(IndirectionEntry);

    // Grab the largest mip and fill with 0 pixels
    Util::FixedArray<IndirectionEntry> pixels(width * width);
    pixels.Fill(IndirectionEntry{ 0xF, 0x3FFF, 0x3FFF });

    // Upload to GPU, the lowest mip will have enough values to cover all mips
    uint offset = Upload(instance, pixels.Begin(), pixels.Size(), 4);

    // go through old region and reset indirection pixels
    for (uint i = 0; i <= mips; i++)
    {
        Math::uint2 mippedCoord{ coord.x >> i, coord.y >> i };
        uint width = tiles >> i;

        instance.indirectionBufferClearsThisFrame.Append(CoreGraphics::BufferCopy{ static_cast<uint>(offset) });
        instance.indirectionTextureClearsThisFrame.Append(CoreGraphics::TextureCopy{ Math::rectangle<SizeT>(mippedCoord.x, mippedCoord.y, mippedCoord.x + width, mippedCoord.y + width), i, 0 });

        // Update CPU buffer
        uint mipOffset = instance.indirectionMipOffsets[i];
        uint rowSize = instance.indirectionMipSizes[i];
        uint rowStart = mippedCoord.x + mippedCoord.y * rowSize;

        for (uint j = 0; j < width; j++)
        {
            uint rowOffset = rowSize * j;
            memcpy(instance.indirectionBuffer.Begin() + mipOffset + rowOffset + rowStart, pixels.Begin(), width * sizeof(IndirectionEntry));
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
    Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();


    for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
    {
        TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];

        if (terrainState.sun != Graphics::InvalidGraphicsEntityId)
        {
            Math::mat4 sunTransform = Lighting::LightContext::GetTransform(terrainState.sun);
            if (terrainState.cachedSunDirection != sunTransform.z_axis)
            {
                Threading::CriticalScope scope(&terrainState.syncPoint);
                terrainState.shadowMapInvalid = true;
                terrainState.cachedSunDirection = sunTransform.z_axis;
            }
        }

        Terrain::TerrainSystemUniforms::STRUCT systemUniforms;
        systemUniforms.TerrainPosBuffer = CoreGraphics::TextureGetBindlessHandle(FrameScript_default::Texture_TerrainPosBuffer());
        systemUniforms.MinLODDistance = 0.0f;
        systemUniforms.MaxLODDistance = terrainState.mipLoadDistance;
        systemUniforms.VirtualLodDistance = terrainState.mipLoadDistance;
        systemUniforms.MinTessellation = 1.0f;
        systemUniforms.MaxTessellation = 32.0f;
        systemUniforms.Debug = terrainState.debugRender;
        systemUniforms.NumBiomes = terrainState.biomeCounter;
        systemUniforms.AlbedoPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.physicalAlbedoCacheBC);
        systemUniforms.NormalPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.physicalNormalCacheBC);
        systemUniforms.MaterialPhysicalCacheBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.physicalMaterialCacheBC);
        systemUniforms.AlbedoLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.lowresAlbedo);
        systemUniforms.NormalLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.lowresNormal);
        systemUniforms.MaterialLowresBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.lowresMaterial);
        systemUniforms.IndirectionBuffer = CoreGraphics::TextureGetBindlessHandle(terrainInstance.indirectionTexture);

        bool biomesLoaded = true;
        for (IndexT j = 0; j < terrainState.biomeCounter; j++)
        {
            Threading::CriticalScope scope(&terrainState.syncPoint);
            BiomeParameters settings = terrainBiomeAllocator.Get<TerrainBiome_Settings>(j).biomeParameters;
            for (IndexT i = 0; i < 4; i++)
            {
                bool allBitsLoaded = AllBits(terrainState.biomeLoaded[j][i], BiomeLoadBits::AlbedoLoaded | BiomeLoadBits::NormalLoaded | BiomeLoadBits::MaterialLoaded | BiomeLoadBits::MaskLoaded | BiomeLoadBits::WeightsLoaded);
                biomesLoaded &= allBitsLoaded;
            }
            if (!terrainState.biomeLowresGenerated[j] && biomesLoaded)
            {
                terrainInstance.updateLowres = true;
                terrainState.biomeLowresGenerated[j] = true;
                CoreGraphics::BufferUpdate(terrainState.biomeBuffer, terrainState.biomeMaterials);
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
        BufferUpdate(terrainInstance.systemConstants, systemUniforms, 0);

        struct PendingDelete
        {
            uint oldMaxMip;
            uint oldTiles;
            Math::uint2 oldCoord;
        };

        // Wait for subtextures job to finish this frame
        terrainInstance.subtexturesFinishedEvent->Wait();

        for (IndexT i = 0; i < terrainInstance.subTextureNumOutputs; i++)
        {
            const Terrain::SubTextureUpdateJobOutput& output = terrainInstance.subTextureJobOutputs[i];
            Terrain::SubTexture& subTex = terrainInstance.subTextures[output.index];
            Terrain::TerrainSubTexture& compressedSubTex = terrainInstance.gpuSubTextures[output.index];

            switch (output.updateState)
            {
                case SubTextureUpdateState::Grew:
                case SubTextureUpdateState::Shrank:
                case SubTextureUpdateState::Created:
                {
                    Math::uint2 newCoord = terrainInstance.indirectionOccupancy.Allocate(output.newTiles);
                    if (newCoord.x == 0xFFFFFFFF || newCoord.y == 0xFFFFFFFF)
                        break;
                    //n_assert(newCoord.x != 0xFFFFFFFF && newCoord.y != 0xFFFFFFFF);

                    // update subtexture
                    subTex.numTiles = output.newTiles;
                    subTex.indirectionOffset = newCoord;
                    subTex.maxMip = output.newMaxMip;
                    terrainInstance.virtualSubtextureBufferUpdate = true;
                    switch (output.updateState)
                    {
                        case SubTextureUpdateState::Grew:
                            IndirectionClear(terrainInstance, output.newMaxMip - output.oldMaxMip, output.newTiles, newCoord);
                            IndirectionMoveGrow(terrainInstance, output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, newCoord);
                            terrainInstance.indirectionOccupancy.Deallocate(output.oldCoord, output.oldTiles);
                            break;
                        case SubTextureUpdateState::Shrank:
                            IndirectionMoveShrink(terrainInstance, output.oldMaxMip, output.oldTiles, output.oldCoord, output.newMaxMip, output.newTiles, newCoord);
                            terrainInstance.indirectionOccupancy.Deallocate(output.oldCoord, output.oldTiles);
                            break;
                        case SubTextureUpdateState::Created:
                            IndirectionClear(terrainInstance, output.newMaxMip, output.newTiles, newCoord);
                            break;
                        default: break;
                    }
                    PackSubTexture(subTex, compressedSubTex);
                    break;
                }
                case SubTextureUpdateState::Deleted:
                    terrainInstance.indirectionOccupancy.Deallocate(output.oldCoord, output.oldTiles);
                    subTex.numTiles = 0;
                    subTex.indirectionOffset.x = 0xFFFFFFFF;
                    subTex.indirectionOffset.y = 0xFFFFFFFF;
                    subTex.maxMip = 0;
                    terrainInstance.virtualSubtextureBufferUpdate = true;
                    PackSubTexture(subTex, compressedSubTex);
                    break;
                default:
                    break;
            }
        }

        if (biomesLoaded)
        {
            // Handle readback from the GPU
            CoreGraphics::BufferInvalidate(terrainInstance.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);
            Terrain::PageUpdateList::STRUCT* updateList = (Terrain::PageUpdateList::STRUCT*)CoreGraphics::BufferMap(terrainInstance.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);
            terrainInstance.numPixels = updateList->NumEntries;
            if (terrainInstance.numPixels < Terrain::MAX_PAGE_UPDATES)
            {
                for (uint i = 0; i < updateList->NumEntries; i++)
                {
                    uint status, subTextureIndex, mip, maxMip, subTextureTileX, subTextureTileY;
                    UnpackPageDataEntry(updateList->Entry[i], status, subTextureIndex, mip, maxMip, subTextureTileX, subTextureTileY);

                    // the update state is either 1 if the page is allocated, or 2 if it used to be allocated but has since been deallocated
                    uint updateState = status;
                    SubTexture& subTexture = terrainInstance.subTextures[subTextureIndex];

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
                    TextureTileCache::CacheResult result = terrainInstance.physicalTextureTileCache.Cache(cacheEntry);
                    if (result.didCache)
                    {
                        IndirectionUpdate(terrainInstance, mip, result.cached.x, result.cached.y, subTexture.indirectionOffset.x, subTexture.indirectionOffset.y, subTextureTileX, subTextureTileY);

                        // Calculate some metrics, the meters per tile, meters per pixel and then the padded meters per tile value
                        float metersPerTile = SubTextureWorldSize / float(subTexture.numTiles >> mip);
                        float metersPerPixel = metersPerTile / float(PhysicalTextureTilePaddedSize);
                        float metersPerTilePadded = metersPerTile + PhysicalTextureTilePadding * metersPerPixel;

                        TerrainTileWrite::TileWrite write;
                        write.WorldOffset[0] = subTexture.worldCoordinate.x + subTextureTileX * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
                        write.WorldOffset[1] = subTexture.worldCoordinate.y + subTextureTileY * metersPerTile - metersPerPixel * PhysicalTextureTileHalfPadding;
                        n_assert(result.cached.x < USHRT_MAX && result.cached.y < USHRT_MAX);
                        write.WriteOffset_MetersPerTile[0] = (result.cached.x & 0xFFFF) | ((result.cached.y & 0xFFFF) << 16);
                        write.WriteOffset_MetersPerTile[1] = *reinterpret_cast<int*>(&metersPerTilePadded);

                        terrainInstance.tileWrites.Append(write);
                    }
                    else
                    {
                        // Calculate indirection pixel in subtexture
                        uint indirectionPixelX = (subTexture.indirectionOffset.x >> mip) + subTextureTileX;
                        uint indirectionPixelY = (subTexture.indirectionOffset.y >> mip) + subTextureTileY;
                        uint mipOffset = terrainInstance.indirectionMipOffsets[mip];
                        uint mipSize = terrainInstance.indirectionMipSizes[mip];

                        // If the cache has the tile but the indirection has been cleared, issue an update for the indirection only
                        const IndirectionEntry& entry = terrainInstance.indirectionBuffer[mipOffset + indirectionPixelX + indirectionPixelY * mipSize];
                        if (entry.mip == 0xF)
                            IndirectionUpdate(terrainInstance, mip, result.cached.x, result.cached.y, subTexture.indirectionOffset.x, subTexture.indirectionOffset.y, subTextureTileX, subTextureTileY);
                    }
                }
                CoreGraphics::BufferUnmap(terrainInstance.pageUpdateReadbackBuffers.buffers[ctx.bufferIndex]);

                IndexT i;

                // Setup constants for tile updates
                SizeT numPagesThisFrame = Math::min(TerrainTileWrite::MAX_TILES_PER_FRAME, terrainInstance.tileWrites.Size());
                for (i = 0; i < numPagesThisFrame; i++)
                {
                    TerrainTileWrite::TileWrite& write = terrainInstance.tileWrites[i];
                    terrainInstance.tileWritesThisFrame.Append(write);
                }
                if (i > 0)
                {
                    terrainInstance.tileWrites.EraseRange(0, numPagesThisFrame - 1);
                }
                if (!terrainInstance.tileWritesThisFrame.IsEmpty())
                    CoreGraphics::BufferUpdateArray(terrainInstance.tileWriteBufferSet.HostBuffer(), terrainInstance.tileWritesThisFrame);

                // Update buffers for indirection pixel uploads
                numPagesThisFrame = Math::min(TerrainTileWrite::MAX_TILES_PER_FRAME, terrainInstance.indirectionEntryUpdates.Size());
                uint offset = Upload(terrainInstance, terrainInstance.indirectionEntryUpdates.Begin(), terrainInstance.indirectionEntryUpdates.Size(), 4);
                for (i = 0; i < numPagesThisFrame; i++)
                {
                    // setup indirection update
                    terrainInstance.indirectionBufferUpdatesThisFrame.Append(CoreGraphics::BufferCopy{ static_cast<uint>(offset + i * sizeof(Terrain::IndirectionEntry)) });
                    terrainInstance.indirectionTextureUpdatesThisFrame.Append(terrainInstance.indirectionTextureCopies[i]);
                }
                if (i > 0)
                {
                    terrainInstance.indirectionEntryUpdates.EraseRange(0, numPagesThisFrame);
                    terrainInstance.indirectionTextureCopies.EraseRange(0, numPagesThisFrame);
                }

                // Flush upload buffer
                CoreGraphics::BufferFlush(terrainInstance.indirectionUploadBuffers.buffers[ctx.bufferIndex], 0, terrainInstance.indirectionUploadOffsets[ctx.bufferIndex]);
            }
        }

        // Wait for jobs to finish
        terrainInstance.sectionCullFinishedEvent->Wait();

        if (terrainInstance.virtualSubtextureBufferUpdate)
        {
            auto bla = reinterpret_cast<SubTextureCompressed*>(terrainInstance.gpuSubTextures.Begin());
            BufferUpdateArray(
                terrainInstance.subtextureStagingBuffers.buffers[ctx.bufferIndex],
                terrainInstance.gpuSubTextures.Begin(),
                terrainInstance.gpuSubTextures.Size());
        }

        TerrainRuntimeInfo& rt = runtimes[instanceIndex];

        TerrainInstanceUniforms::STRUCT uniforms;
        Math::mat4().store(&uniforms.Transform[0][0]);
        uniforms.DecisionMap = TextureGetBindlessHandle(CoreGraphics::TextureId(rt.decisionMap));
        uniforms.HeightMap = TextureGetBindlessHandle(CoreGraphics::TextureId(rt.heightMap));

        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(rt.heightMap);
        CoreGraphics::TextureDimensions dataBufferDims = CoreGraphics::TextureGetDimensions(FrameScript_default::Texture_TerrainPosBuffer());
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
        uniforms.VirtualTerrainNumSubTextures[0] = terrainInstance.createInfo.width / SubTextureWorldSize;
        uniforms.VirtualTerrainNumSubTextures[1] = terrainInstance.createInfo.height / SubTextureWorldSize;
        uniforms.PhysicalInvPaddedTextureSize = 1.0f / PhysicalTexturePaddedSize;
        uniforms.PhysicalTileSize = PhysicalTextureTileSize;
        uniforms.PhysicalTilePaddedSize = PhysicalTextureTilePaddedSize;
        uniforms.PhysicalTilePadding = PhysicalTextureTileHalfPadding;

        CoreGraphics::TextureDimensions indirectionDims = CoreGraphics::TextureGetDimensions(terrainInstance.indirectionTexture);
        uniforms.VirtualTerrainNumPages[0] = indirectionDims.width;
        uniforms.VirtualTerrainNumPages[1] = indirectionDims.height;
        uniforms.VirtualTerrainNumMips = IndirectionNumMips;

        uniforms.LowresResolution[0] = uniforms.LowresResolution[1] = LowresFallbackSize;
        uniforms.LowresNumMips = LowresFallbackMips - 1;

        uniforms.IndirectionResolution[0] = uniforms.IndirectionResolution[1] = IndirectionTextureSize;
        uniforms.IndirectionNumMips = IndirectionNumMips - 1;

        uniforms.LowresFadeStart = SubTextureFadeStart * SubTextureFadeStart;
        uniforms.LowresFadeDistance = 1.0f / ((SubTextureRange - SubTextureFadeStart) * (SubTextureRange - SubTextureFadeStart));

        for (SizeT j = 0; j < terrainInstance.indirectionMipOffsets.Size(); j++)
        {
            uniforms.VirtualPageBufferMipOffsets[j / 4][j % 4] = terrainInstance.indirectionMipOffsets[j];
            uniforms.VirtualPageBufferMipSizes[j / 4][j % 4] = terrainInstance.indirectionMipSizes[j];
        }
        uniforms.VirtualPageBufferNumPages = CoreGraphics::BufferGetSize(terrainInstance.pageStatusBuffer);

        BufferUpdate(terrainInstance.runtimeConstants, uniforms);

        if (terrainState.shadowMapInvalid)
            terrainState.updateShadowMap = true;
    }
}


//------------------------------------------------------------------------------
/**
*/
bool
TerrainContext::GetVisible()
{
    return terrainState.renderToggle;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetVisible(bool visible)
{
    terrainState.renderToggle = visible;
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
            Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();

            if (ImGui::BeginTable("Terrain Instances", 2, ImGuiTableFlags_Resizable))
            {
                ImGui::TableNextColumn();

                static int current = -1;
                if (ImGui::BeginListBox("###TerrainInstances"))
                {
                    for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
                    {
                        TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
                        if (ImGui::Selectable(Util::String::Sprintf("Instance %d", instanceIndex).AsCharPtr()))
                        {
                            current = instanceIndex;
                        }
                    }

                    ImGui::EndListBox();
                }
                ImGui::TableNextColumn();
                if (current != -1)
                {
                    TerrainInstanceInfo& terrainInstance = terrainInstances[current];
                    ImGui::LabelText("###Updates", "Number of updates %d", terrainInstance.numPixels);
                    {
                        ImGui::Text("Indirection texture occupancy quadtree");
                        ImDrawList* drawList = ImGui::GetWindowDrawList();
                        ImVec2 start = ImGui::GetCursorScreenPos();
                        ImVec2 fullSize = ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y);
                        drawList->PushClipRect(
                            ImVec2{ start.x, start.y },
                            ImVec2{ Math::max(start.x + fullSize.x, start.x + 512.0f), Math::min(start.y + fullSize.y, start.y + 512.0f) }, true);

                        terrainInstance.indirectionOccupancy.DebugRender(drawList, start, 0.25f);
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
                        ImGui::Text("Physical Albedo Cache");
                        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainInstance.physicalAlbedoCacheBC);

                        ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                        static Dynui::ImguiTextureId textureInfo;
                        textureInfo.nebulaHandle = terrainInstance.physicalAlbedoCacheBC;
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
                        ImGui::Text("Physical Normal Cache");
                        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainInstance.physicalNormalCacheBC);

                        ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                        static Dynui::ImguiTextureId textureInfo;
                        textureInfo.nebulaHandle = terrainInstance.physicalNormalCacheBC;
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
                        ImGui::Text("Physical Normal Cache");
                        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainInstance.physicalMaterialCacheBC);

                        ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                        static Dynui::ImguiTextureId textureInfo;
                        textureInfo.nebulaHandle = terrainInstance.physicalMaterialCacheBC;
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

                        CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainInstance.shadowMap);

                        ImVec2 imageSize = { (float)dims.width, (float)dims.height };

                        static Dynui::ImguiTextureId textureInfo;
                        textureInfo.nebulaHandle = terrainInstance.shadowMap;
                        textureInfo.mip = mip;
                        textureInfo.layer = 0;

                        imageSize.x = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
                        float ratio = (float)dims.height / (float)dims.width;
                        imageSize.y = imageSize.x * ratio;

                        ImGui::Image((void*)&textureInfo, imageSize);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::End();

        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::ClearCache()
{
    Util::Array<TerrainInstanceInfo>& terrainInstances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    for (IndexT instanceIndex = 0; instanceIndex < terrainInstances.Size(); instanceIndex++)
    {
        TerrainInstanceInfo& terrainInstance = terrainInstances[instanceIndex];
        terrainInstance.physicalTextureTileCache.Clear();
    }
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

#if WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetHeightmap(Graphics::GraphicsEntityId entity, CoreGraphics::TextureId heightmap)
{
    Graphics::ContextEntityId id = GetContextId(entity);
    TerrainRuntimeInfo& rt = terrainAllocator.Get<Terrain_RuntimeInfo>(id.id);
    rt.heightMap = heightmap;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetBiomeMask(TerrainBiomeId biomeId, CoreGraphics::TextureId biomemask)
{
    terrainState.biomeMasks[biomeId.id] = biomemask;
    terrainState.biomeMaterials.MaterialMasks[biomeId.id / 4][biomeId.id % 4] = CoreGraphics::TextureGetBindlessHandle(biomemask);
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetBiomeLayer(TerrainBiomeId biomeId, BiomeSettings::BiomeMaterialLayer layer, const Resources::ResourceName& albedo, const Resources::ResourceName& normal, const Resources::ResourceName& material)
{
    terrainState.biomeResources[biomeId.id][layer].albedo = albedo;
    terrainState.biomeResources[biomeId.id][layer].normal = normal;
    terrainState.biomeResources[biomeId.id][layer].material = material;

    if (terrainState.biomeResources[biomeId.id][layer].albedoRes != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(terrainState.biomeResources[biomeId.id][layer].albedoRes);
        terrainState.biomeResources[biomeId.id][layer].albedoRes = Resources::InvalidResourceId;
    }

    if (terrainState.biomeResources[biomeId.id][layer].normalRes != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(terrainState.biomeResources[biomeId.id][layer].normalRes);
        terrainState.biomeResources[biomeId.id][layer].normalRes = Resources::InvalidResourceId;
    }

    if (terrainState.biomeResources[biomeId.id][layer].materialRes != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(terrainState.biomeResources[biomeId.id][layer].materialRes);
        terrainState.biomeResources[biomeId.id][layer].materialRes = Resources::InvalidResourceId;
    }

    terrainState.biomeLoaded[biomeId.id][layer] &= ~(BiomeLoadBits::AlbedoLoaded | BiomeLoadBits::NormalLoaded | BiomeLoadBits::MaterialLoaded);
    terrainState.biomeResources[biomeId.id][layer].albedoRes = Resources::CreateResource(terrainState.biomeResources[biomeId.id][layer].albedo.Value(), "terrain", [layer, i = biomeId.id](Resources::ResourceId id)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        CoreGraphics::TextureIdLock _0(id);
        terrainState.biomeMaterials.MaterialAlbedos[i][layer] = CoreGraphics::TextureGetBindlessHandle(id);
        terrainState.biomeTextures.Append(id);
        terrainState.biomeLowresGenerated[i] = false;
        terrainState.biomeLoaded[i][layer] |= BiomeLoadBits::AlbedoLoaded;
    }, nullptr, false, false);

    terrainState.biomeResources[biomeId.id][layer].normalRes = Resources::CreateResource(terrainState.biomeResources[biomeId.id][layer].normal.Value(), "terrain", [layer, i = biomeId.id](Resources::ResourceId id)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        CoreGraphics::TextureIdLock _0(id);
        terrainState.biomeMaterials.MaterialNormals[i][layer] = CoreGraphics::TextureGetBindlessHandle(id);
        terrainState.biomeTextures.Append(id);
        terrainState.biomeLowresGenerated[i] = false;
        terrainState.biomeLoaded[i][layer] |= BiomeLoadBits::NormalLoaded;
    }, nullptr, false, false);

    terrainState.biomeResources[biomeId.id][layer].materialRes = Resources::CreateResource(terrainState.biomeResources[biomeId.id][layer].material.Value(), "terrain", [layer, i = biomeId.id](Resources::ResourceId id)
    {
        Threading::CriticalScope scope(&terrainState.syncPoint);
        CoreGraphics::TextureIdLock _0(id);
        terrainState.biomeMaterials.MaterialPBRs[i][layer] = CoreGraphics::TextureGetBindlessHandle(id);
        terrainState.biomeTextures.Append(id);
        terrainState.biomeLowresGenerated[i] = false;
        terrainState.biomeLoaded[i][layer] |= BiomeLoadBits::MaterialLoaded;
    }, nullptr, false, false);

    const auto& instances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    for (auto& instance : instances)
    {
        instance.updateLowres = true;
    }
    terrainState.biomeLowresGenerated[biomeId.id] = false;
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::SetBiomeRules(TerrainBiomeId biomeId, float slopeThreshold, float heightThreshold, float uvScalingFactor)
{
    Threading::CriticalScope scope(&terrainState.syncPoint);
    BiomeParameters& params = terrainBiomeAllocator.Get<TerrainBiome_Settings>(biomeId.id).biomeParameters;
    params.slopeThreshold = slopeThreshold;
    params.heightThreshold = heightThreshold;
    params.uvScaleFactor = uvScalingFactor;
    const auto& instances = terrainAllocator.GetArray<Terrain_InstanceInfo>();
    for (auto& instance : instances)
    {
        instance.updateLowres = true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TerrainContext::InvalidateTerrain(Graphics::GraphicsEntityId entity)
{
    Threading::CriticalScope scope(&terrainState.syncPoint);
    Graphics::ContextEntityId id = GetContextId(entity);
    TerrainInstanceInfo& instance = terrainAllocator.Get<Terrain_InstanceInfo>(id.id);
    instance.updateLowres = true;
    terrainState.shadowMapInvalid = true;
    //terrainState.invalidationFrame = CoreGraphics::GetNumBufferedFrames();

    //instance.indirectionOccupancy.Clear();
    instance.physicalTextureTileOccupancy.Clear();
    //instance.indirectionBuffer.Fill(IndirectionEntry{ 0xF, 0x3FFF, 0x3FFF });

    /*
    for (int i = 0; i < instance.subTextures.Size(); i++)
    {
        instance.subTextures[i].numTiles = 0;
        instance.subTextures[i].maxMip = 0;
        instance.subTextures[i].indirectionOffset.x = UINT32_MAX;
        instance.subTextures[i].indirectionOffset.y = UINT32_MAX;
        instance.subTextures[i].mipBias = 0;
        PackSubTexture(instance.subTextures[i], instance.gpuSubTextures[i]);
    }
    */
}
#endif 

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
