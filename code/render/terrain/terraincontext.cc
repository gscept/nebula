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

#include "resources/resourceserver.h"
#include "gliml.h"

#include "terrain.h"

N_DECLARE_COUNTER(N_TERRAIN_TOTAL_AVAILABLE_DATA, Terrain Total Data Size);

#define USE_SPARSE

namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
TerrainContext::TerrainBiomeAllocator TerrainContext::terrainBiomeAllocator;

extern void TerrainCullJob(const Jobs::JobFuncContext& ctx);
Util::Queue<Jobs::JobId> TerrainContext::runningJobs;
Jobs::JobSyncId TerrainContext::jobHostSync;


const uint IndirectionTextureSize = 2048;
const uint SubTextureTileWorldSize = 64;
const uint SubTextureMaxTiles = 256;
const uint IndirectionNumMips = Math::n_log2(SubTextureMaxTiles) + 1;

const uint LowresFallbackSize = 4096;
const uint LowresFallbackMips = Math::n_log2(LowresFallbackSize) + 1;

const uint PhysicalTextureTileSize = 256;
const uint PhysicalTextureTilePadding = 8;
const uint PhysicalTextureNumTiles = 32;
const uint PhysicalTextureTilePaddedSize = PhysicalTextureTileSize + PhysicalTextureTilePadding;
const uint PhysicalTextureSize = (PhysicalTextureTileSize) * PhysicalTextureNumTiles;
const uint PhysicalTexturePaddedSize = (PhysicalTextureTilePaddedSize) * PhysicalTextureNumTiles;

_ImplementContext(TerrainContext, TerrainContext::terrainAllocator);


struct
{
	CoreGraphics::ShaderId terrainShader;
	CoreGraphics::ShaderProgramId terrainZProgram;
	CoreGraphics::ShaderProgramId terrainPrepassProgram;
	CoreGraphics::ShaderProgramId terrainProgram;
	CoreGraphics::ShaderProgramId terrainScreenPass;
	CoreGraphics::ResourceTableId resourceTable;
	CoreGraphics::BufferId systemConstants;
	Util::Array<CoreGraphics::VertexComponent> components;
	CoreGraphics::VertexLayoutId vlo;
	Util::Array<TerrainMaterial> materials;

	CoreGraphics::WindowId wnd;

	CoreGraphics::TextureId biomeAlbedoArray[Terrain::MAX_BIOMES];
	CoreGraphics::TextureId biomeNormalArray[Terrain::MAX_BIOMES];
	CoreGraphics::TextureId biomeMaterialArray[Terrain::MAX_BIOMES];
	CoreGraphics::TextureId biomeMasks[Terrain::MAX_BIOMES];
	IndexT biomeCounter;

	RenderUtil::DrawFullScreenQuad fsq;
	CoreGraphics::TextureId terrainDataBuffer;
	CoreGraphics::TextureId terrainNormalBuffer;
	CoreGraphics::TextureId terrainPosBuffer;
	
	IndexT albedoSlot;
	IndexT normalsSlot;
	IndexT pbrSlot;
	IndexT maskSlot;

	float mipLoadDistance = 1500.0f;
	float mipRenderPadding = 150.0f;
	SizeT layers;

	bool debugRender;

} terrainState;

struct PhysicalPageUpdate
{
	uint constantBufferOffsets[2];
};

struct IndirectionEntry
{
	uint mip : 4;
	uint physicalOffsetX : 14;
	uint physicalOffsetY : 14;
};

struct IndirectionCopy
{
	SizeT bufferOffset;
	Math::uint2 pixelOffset;
};

struct
{
	CoreGraphics::ShaderProgramId terrainPrepassProgram;
	CoreGraphics::ShaderProgramId terrainPageClearUpdateBufferProgram;
	CoreGraphics::ShaderProgramId terrainPageExtractionBufferProgram;
	CoreGraphics::ShaderProgramId terrainTileSplattingProgram;
	CoreGraphics::ShaderProgramId terrainScreenspacePass;
	CoreGraphics::ShaderProgramId terrainLowresGeneratePass;
	CoreGraphics::BufferId virtualSubtextureBuffer;

	CoreGraphics::BufferId pageUpdateReadbackBuffer;
	CoreGraphics::TextureId indirectionTexture;
	Util::FixedArray<CoreGraphics::TextureViewId> indirectionTextureViews;
	CoreGraphics::BufferId pageUpdateListBuffer;
	CoreGraphics::BufferId pageUpdateBuffer;
	Util::FixedArray<Util::FixedArray<CoreGraphics::BufferId>> indirectionUploadBuffers;
	Util::FixedArray<bool> indirectionTextureMipNeedsUpdate;

	Util::Array<IndirectionCopy> indirectionTextureCopies;

	CoreGraphics::BufferId runtimeConstants;

	CoreGraphics::TextureId physicalAlbedoCache;
	CoreGraphics::TextureId physicalNormalCache;
	CoreGraphics::TextureId physicalMaterialCache;
	CoreGraphics::TextureId lowresAlbedo;
	CoreGraphics::TextureId lowresNormal;
	CoreGraphics::TextureId lowresMaterial;

	CoreGraphics::EventId biomeUpdatedEvent;
	bool updateLowres = false;

	CoreGraphics::ResourceTableId virtualTerrainSystemResourceTable;
	CoreGraphics::ResourceTableId virtualTerrainRuntimeResourceTable;
	CoreGraphics::ResourceTableId virtualTerrainDynamicResourceTable;

	SizeT numPageBufferUpdateEntries;

	Util::FixedArray<Terrain::VirtualTerrainSubTexture>		virtualSubtextures;
	OccupancyQuadTree										indirectionOccupancy;
	OccupancyQuadTree										physicalTextureTileOccupancy;

	Util::FixedArray<Util::FixedArray<IndirectionEntry>>	indirection;
	Util::FixedArray<uint>									indirectionMipOffsets;
	Util::FixedArray<uint>									indirectionMipSizes;

	Util::Array<Terrain::TerrainTileUpdateUniforms> pageUniforms;
	Util::Array<PhysicalPageUpdate> pageUpdatesThisFrame;

	RenderUtil::DrawFullScreenQuad fsq;

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
TerrainContext::Create(const CoreGraphics::WindowId wnd)
{
	_CreateContext();
	using namespace CoreGraphics;

	__bundle.OnPrepareView = TerrainContext::CullPatches;
	__bundle.OnUpdateViewResources = TerrainContext::UpdateLOD;
	__bundle.OnBegin = TerrainContext::RenderUI;

	TerrainContext::jobHostSync = Jobs::CreateJobSync({ false });

#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Frame::AddCallback("TerrainContext - Render Terrain GBuffer", [](const IndexT frame, const IndexT frameBufferIndex)
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

	Frame::AddCallback("TerrainContext - Render Terrain Screenspace", [](const IndexT frame, const IndexT frameBufferIndex)
		{
			Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
			if (runtimes.Size() > 0)
			{
				// set shader and draw fsq
				SetShaderProgram(terrainState.terrainScreenPass);
				CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
				terrainState.fsq.ApplyMesh();
				SetResourceTable(terrainState.resourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
				CoreGraphics::Draw();
				CoreGraphics::EndBatch();
			}
		});

	Frame::AddCallback("TerrainContext - Terrain Shadows", [](const IndexT frame, const IndexT frameBufferIndex)
		{
		});

	// setup fsq
	CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(wnd);
	terrainState.fsq.Setup(mode.GetWidth(), mode.GetHeight());
	terrainState.wnd = wnd;

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
		3. Extract new page mips to readback buffer.
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
	terrainVirtualTileState.terrainPageExtractionBufferProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainExtractPageBuffer"));
	terrainVirtualTileState.terrainTileSplattingProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainTileSplatting"));
	terrainVirtualTileState.terrainScreenspacePass = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainVirtualScreenSpace"));
	terrainVirtualTileState.terrainLowresGeneratePass = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("TerrainGenerateLowresFallback"));
	terrainVirtualTileState.virtualTerrainSystemResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_SYSTEM_GROUP);
	terrainVirtualTileState.virtualTerrainRuntimeResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
	terrainVirtualTileState.virtualTerrainDynamicResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);

	terrainVirtualTileState.fsq.Setup(mode.GetWidth(), mode.GetHeight());

	CoreGraphics::BufferCreateInfo bufInfo;
	bufInfo.name = "VirtualRuntimeBuffer"_atm;
	bufInfo.size = 1;
	bufInfo.elementSize = sizeof(Terrain::TerrainRuntimeUniforms);
	bufInfo.mode = CoreGraphics::HostToDevice;
	bufInfo.usageFlags = CoreGraphics::ConstantBuffer;
	terrainVirtualTileState.runtimeConstants = CoreGraphics::CreateBuffer(bufInfo);

	uint size = PhysicalTextureSize / SubTextureTileWorldSize;

	bufInfo.name = "VirtualSubTextureBuffer"_atm;
	bufInfo.size = size * size * CoreGraphics::GetNumBufferedFrames();
	bufInfo.elementSize = sizeof(Terrain::VirtualTerrainSubTexture);
	bufInfo.mode = BufferAccessMode::HostToDevice;
	bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
	terrainVirtualTileState.virtualSubtextureBuffer = CoreGraphics::CreateBuffer(bufInfo);

	CoreGraphics::TextureCreateInfo texInfo;
	texInfo.name = "IndirectionTexture"_atm;
	texInfo.width = IndirectionTextureSize;
	texInfo.height = IndirectionTextureSize;
	texInfo.format = CoreGraphics::PixelFormat::R32;
	texInfo.usage = CoreGraphics::SampleTexture;
	texInfo.mips = IndirectionNumMips;
	terrainVirtualTileState.indirectionTexture = CoreGraphics::CreateTexture(texInfo);

	terrainVirtualTileState.indirectionTextureViews.Resize(IndirectionNumMips);
	for (uint i = 0; i < IndirectionNumMips; i++)
	{
		CoreGraphics::TextureViewCreateInfo viewInfo;
		viewInfo.tex = terrainVirtualTileState.indirectionTexture;
		viewInfo.startMip = i;
		viewInfo.numMips = 1;
		viewInfo.startLayer = 0;
		viewInfo.numLayers = 1;
		viewInfo.format = texInfo.format;
		terrainVirtualTileState.indirectionTextureViews[i] = CoreGraphics::CreateTextureView(viewInfo);
	}

	// setup indirection texture upload buffer
	terrainVirtualTileState.indirectionUploadBuffers.Resize(IndirectionNumMips);
	terrainVirtualTileState.indirectionTextureMipNeedsUpdate.Resize(IndirectionNumMips);
	for (IndexT i = 0; i < terrainVirtualTileState.indirectionUploadBuffers.Size(); i++)
	{
		terrainVirtualTileState.indirectionUploadBuffers[i].Resize(CoreGraphics::GetNumBufferedFrames());
		uint size = IndirectionTextureSize >> i;
		SizeT bufferSize = size * size * sizeof(uint);
		char* buf = n_new_array(char, bufferSize);
		memset(buf, 0xFF, bufferSize);

		for (SizeT j = 0; j < terrainVirtualTileState.indirectionUploadBuffers[i].Size(); j++)
		{

			CoreGraphics::BufferCreateInfo bufInfo;
			bufInfo.name = "IndirectionUploadBuffer"_atm;
			bufInfo.elementSize = sizeof(IndirectionEntry);
			bufInfo.size = size * size;
			bufInfo.mode = CoreGraphics::HostLocal;
			bufInfo.usageFlags = CoreGraphics::TransferBufferSource;
			bufInfo.data = buf;
			bufInfo.dataSize = bufferSize;
			terrainVirtualTileState.indirectionUploadBuffers[i][j] = CoreGraphics::CreateBuffer(bufInfo);
		}

		n_delete_array(buf);
	}

	terrainVirtualTileState.indirection.Resize(IndirectionNumMips);
	uint offset = 0;

	terrainVirtualTileState.indirectionMipOffsets.Resize(IndirectionNumMips);
	terrainVirtualTileState.indirectionMipSizes.Resize(IndirectionNumMips);
	for (uint i = 0; i < IndirectionNumMips; i++)
	{
		uint width = IndirectionTextureSize >> i;
		uint height = IndirectionTextureSize >> i;
		terrainVirtualTileState.indirection[i].Resize(width * height);
		terrainVirtualTileState.indirection[i].Fill(IndirectionEntry{ 0xF, 0xFFFFFFFF, 0xFFFFFFFF });

		terrainVirtualTileState.indirectionMipOffsets[i] = offset;
		offset += width * height;
		terrainVirtualTileState.indirectionMipSizes[i] = width;
	}

	bufInfo.name = "PageUpdateBuffer"_atm;
	bufInfo.elementSize = sizeof(Math::uint4);
	bufInfo.size = offset;
	bufInfo.mode = BufferAccessMode::DeviceLocal;
	bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferDestination;
	bufInfo.data = nullptr;
	bufInfo.dataSize = 0;
	terrainVirtualTileState.pageUpdateBuffer = CoreGraphics::CreateBuffer(bufInfo);

	CoreGraphics::LockResourceSubmission();

	// clear the buffer
	CoreGraphics::SubmissionContextId sub = CoreGraphics::GetResourceSubmissionContext();
	CoreGraphics::BufferFill(terrainVirtualTileState.pageUpdateBuffer, 0x0, sub);

	bufInfo.name = "PageUpdateListBuffer"_atm;
	bufInfo.elementSize = sizeof(Terrain::PageUpdateList);
	bufInfo.size = 1;
	bufInfo.mode = BufferAccessMode::DeviceLocal;
	bufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource | CoreGraphics::TransferBufferDestination;
	bufInfo.data = nullptr;
	bufInfo.dataSize = 0;
	terrainVirtualTileState.pageUpdateListBuffer = CoreGraphics::CreateBuffer(bufInfo);

	// clear the buffer
	CoreGraphics::BufferFill(terrainVirtualTileState.pageUpdateListBuffer, 0x0, sub);

	bufInfo.name = "PageUpdateReadbackBuffer"_atm;
	bufInfo.elementSize = sizeof(Terrain::PageUpdateList);
	bufInfo.size = CoreGraphics::GetNumBufferedFrames();
	bufInfo.usageFlags = CoreGraphics::TransferBufferDestination;
	bufInfo.mode = BufferAccessMode::DeviceToHost;
	bufInfo.data = nullptr;
	bufInfo.dataSize = 0;
	terrainVirtualTileState.pageUpdateReadbackBuffer = CoreGraphics::CreateBuffer(bufInfo);

	// clear the buffer
	CoreGraphics::BufferFill(terrainVirtualTileState.pageUpdateReadbackBuffer, 0x0, sub);

	CoreGraphics::UnlockResourceSubmission();

	CoreGraphics::TextureCreateInfo albedoCacheInfo;
	albedoCacheInfo.name = "AlbedoPhysicalCache"_atm;
	albedoCacheInfo.width = PhysicalTexturePaddedSize;
	albedoCacheInfo.height = PhysicalTexturePaddedSize;
	albedoCacheInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
	albedoCacheInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
	terrainVirtualTileState.physicalAlbedoCache = CoreGraphics::CreateTexture(albedoCacheInfo);

	CoreGraphics::TextureCreateInfo normalCacheInfo;
	normalCacheInfo.name = "NormalPhysicalCache"_atm;
	normalCacheInfo.width = PhysicalTexturePaddedSize;
	normalCacheInfo.height = PhysicalTexturePaddedSize;
	normalCacheInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
	normalCacheInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
	terrainVirtualTileState.physicalNormalCache = CoreGraphics::CreateTexture(normalCacheInfo);

	CoreGraphics::TextureCreateInfo materialCacheInfo;
	materialCacheInfo.name = "MaterialPhysicalCache"_atm;
	materialCacheInfo.width = PhysicalTexturePaddedSize;
	materialCacheInfo.height = PhysicalTexturePaddedSize;
	materialCacheInfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
	materialCacheInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture;
	terrainVirtualTileState.physicalMaterialCache = CoreGraphics::CreateTexture(materialCacheInfo);

	CoreGraphics::TextureCreateInfo lowResAlbedoInfo;
	lowResAlbedoInfo.name = "AlbedoLowres"_atm;
	lowResAlbedoInfo.mips = LowresFallbackMips;
	lowResAlbedoInfo.width = LowresFallbackSize;
	lowResAlbedoInfo.height = LowresFallbackSize;
	lowResAlbedoInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
	lowResAlbedoInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture | CoreGraphics::TextureUsage::TransferTextureSource | CoreGraphics::TextureUsage::TransferTextureDestination;
	terrainVirtualTileState.lowresAlbedo = CoreGraphics::CreateTexture(lowResAlbedoInfo);

	CoreGraphics::TextureCreateInfo lowResNormalInfo;
	lowResNormalInfo.name = "NormalLowres"_atm;
	lowResNormalInfo.mips = LowresFallbackMips;
	lowResNormalInfo.width = LowresFallbackSize;
	lowResNormalInfo.height = LowresFallbackSize;
	lowResNormalInfo.format = CoreGraphics::PixelFormat::R11G11B10F;
	lowResNormalInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture | CoreGraphics::TextureUsage::TransferTextureSource | CoreGraphics::TextureUsage::TransferTextureDestination;
	terrainVirtualTileState.lowresNormal = CoreGraphics::CreateTexture(lowResNormalInfo);

	CoreGraphics::TextureCreateInfo lowResMaterialInfo;
	lowResMaterialInfo.name = "MaterialLowres"_atm;
	lowResMaterialInfo.mips = LowresFallbackMips;
	lowResMaterialInfo.width = LowresFallbackSize;
	lowResMaterialInfo.height = LowresFallbackSize;
	lowResMaterialInfo.format = CoreGraphics::PixelFormat::R16G16B16A16F;
	lowResMaterialInfo.usage = CoreGraphics::TextureUsage::ReadWriteTexture | CoreGraphics::TextureUsage::TransferTextureSource | CoreGraphics::TextureUsage::TransferTextureDestination;
	terrainVirtualTileState.lowresMaterial = CoreGraphics::CreateTexture(lowResMaterialInfo);

#ifdef CreateEvent
#undef CreateEvent
#endif
	CoreGraphics::EventCreateInfo eventInfo{ "Biome Update Finished Event"_atm, true, nullptr, nullptr, nullptr };
	terrainVirtualTileState.biomeUpdatedEvent = CoreGraphics::CreateEvent(eventInfo);

	// setup virtual sub textures buffer
	terrainVirtualTileState.virtualSubtextures.Resize(size * size);
	for (uint y = 0; y < size; y++)
	{
		for (uint x = 0; x < size; x++)
		{
			uint index = x + y * size;
			terrainVirtualTileState.virtualSubtextures[index].size = UINT32_MAX;
			terrainVirtualTileState.virtualSubtextures[index].indirectionOffset[0] = UINT32_MAX;
			terrainVirtualTileState.virtualSubtextures[index].indirectionOffset[1] = UINT32_MAX;

			// position is calculated as the center of each cell, offset by half of the world size (so we are oriented around 0)
			float xPos = x * SubTextureTileWorldSize - PhysicalTextureSize / 2.0f;
			float yPos = y * SubTextureTileWorldSize - PhysicalTextureSize / 2.0f;
			terrainVirtualTileState.virtualSubtextures[index].worldCoordinate[0] = xPos;
			terrainVirtualTileState.virtualSubtextures[index].worldCoordinate[1] = yPos;
		}
	}

	// setup indirection occupancy, the indirection texture is 2048, and the maximum allocation size is 256 indirection pixels
	terrainVirtualTileState.indirectionOccupancy.Setup(IndirectionTextureSize, 256, 1);

	// setup the physical texture occupancy, the texture is 8192x8192 pixels, and the page size is 256, so effectively make this a quad tree that ends at individual pages
	terrainVirtualTileState.physicalTextureTileOccupancy.Setup(PhysicalTexturePaddedSize, PhysicalTexturePaddedSize, PhysicalTextureTilePaddedSize);

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
			terrainVirtualTileState.virtualSubtextureBuffer,
			ShaderGetResourceSlot(terrainState.terrainShader, "VirtualTerrainSubTextures"),
			0, 
			false, false,
			NEBULA_WHOLE_BUFFER_SIZE,
			0
		});

	ResourceTableSetRWBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.pageUpdateListBuffer,
			ShaderGetResourceSlot(terrainState.terrainShader, "VirtualTerrainPageUpdateList"),
			0,
			false, false,
			NEBULA_WHOLE_BUFFER_SIZE,
			0
		});

	ResourceTableSetRWBuffer(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.pageUpdateBuffer,
			ShaderGetResourceSlot(terrainState.terrainShader, "PageUpdateBuffer"),
			0,
			false, false,
			NEBULA_WHOLE_BUFFER_SIZE,
			0
		});

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.physicalAlbedoCache,
			ShaderGetResourceSlot(terrainState.terrainShader, "VirtualAlbedoOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
		});

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.physicalNormalCache,
			ShaderGetResourceSlot(terrainState.terrainShader, "VirtualNormalOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
		});

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.physicalMaterialCache,
			ShaderGetResourceSlot(terrainState.terrainShader, "VirtualMaterialOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
		});

	for (IndexT i = 0; i < terrainVirtualTileState.indirectionTextureViews.Size(); i++)
	{
		ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
			{
				terrainVirtualTileState.indirectionTextureViews[i],
				ShaderGetResourceSlot(terrainState.terrainShader, "IndirectionTextureOutput"),
				i,
				CoreGraphics::SamplerId::Invalid(),
				false, false,
			});
	}

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.lowresAlbedo,
			ShaderGetResourceSlot(terrainState.terrainShader, "LowresAlbedoOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
		});

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.lowresNormal,
			ShaderGetResourceSlot(terrainState.terrainShader, "LowresNormalOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
		});

	ResourceTableSetRWTexture(terrainVirtualTileState.virtualTerrainSystemResourceTable,
		{
			terrainVirtualTileState.lowresMaterial,
			ShaderGetResourceSlot(terrainState.terrainShader, "LowresMaterialOutput"),
			0,
			CoreGraphics::SamplerId::Invalid(),
			false, false,
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
			true, false,
			sizeof(Terrain::TerrainTileUpdateUniforms),
			0
		});
	ResourceTableCommitChanges(terrainVirtualTileState.virtualTerrainDynamicResourceTable);

	Frame::AddCallback("TerrainContext - Clear Page Update Buffer", [](const IndexT frame, const IndexT frameBufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Clear Page Update Buffer");
			BarrierInsert(GraphicsQueueType,
				BarrierStage::ComputeShader,
				BarrierStage::ComputeShader,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						terrainVirtualTileState.pageUpdateBuffer,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				},
				"Virtual Page Buffer Clear Barrier");

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

			// for each runtime, clear their respective update buffer
			for (IndexT i = 0; i < runtimes.Size(); i++)
			{
				TerrainRuntimeInfo& rt = runtimes[i];
				SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
				const SizeT size = BufferGetSize(terrainVirtualTileState.pageUpdateBuffer);

				uint numWorkGroupsY = Math::n_divandroundup(size, 2048);
				uint numWorkGroupsX = 2048 / 64;

				// run the compute shader over all page buffer update entries
				Compute(numWorkGroupsX, numWorkGroupsY, 1);
			}

			BarrierInsert(GraphicsQueueType,
				BarrierStage::ComputeShader,
				BarrierStage::PixelShader,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						terrainVirtualTileState.pageUpdateBuffer,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				},
				"Virtual Page Buffer Clear Barrier");

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

	Frame::AddCallback("TerrainContext - Render Terrain Prepass", [](const IndexT frame, const IndexT frameBufferIndex)
		{
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

	Frame::AddCallback("TerrainContext - Extract Readback Data Buffer", [](const IndexT frame, const IndexT frameBufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Extract Readback Data Buffer");

			BarrierInsert(GraphicsQueueType,
				BarrierStage::PixelShader,
				BarrierStage::ComputeShader,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						terrainVirtualTileState.pageUpdateBuffer,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderRead,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				},
				"Virtual Page Texture Clear Barrier");

			SetShaderProgram(terrainVirtualTileState.terrainPageExtractionBufferProgram);
			SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
			Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

			// for each runtime, clear their respective update buffer
			for (IndexT i = 0; i < runtimes.Size(); i++)
			{
				TerrainRuntimeInfo& rt = runtimes[i];
				SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);
				const SizeT size = BufferGetSize(terrainVirtualTileState.pageUpdateBuffer);

				uint numWorkGroupsY = Math::n_divandroundup(size, 2048);
				uint numWorkGroupsX = 2048 / 64;

				// run the compute shader over all page buffer update entries
				Compute(numWorkGroupsX, numWorkGroupsY, 1);
			}

			CommandBufferEndMarker(GraphicsQueueType);

			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Terrain Copy Readback Buffer");

			BarrierInsert(GraphicsQueueType,
				BarrierStage::ComputeShader,
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
				},
				"Page Readback Buffer Barrier");

			BarrierInsert(GraphicsQueueType,
				BarrierStage::Host,
				BarrierStage::Transfer,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						terrainVirtualTileState.pageUpdateReadbackBuffer,
						BarrierAccess::HostRead,
						BarrierAccess::TransferWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				},
				"Page Readback Buffer Barrier");

			Copy(
				GraphicsQueueType,
				terrainVirtualTileState.pageUpdateListBuffer,
				0,
				terrainVirtualTileState.pageUpdateReadbackBuffer,
				sizeof(Terrain::PageUpdateList) * CoreGraphics::GetBufferedFrameIndex(),
				sizeof(Terrain::PageUpdateList)
			);

			BarrierInsert(GraphicsQueueType,
				BarrierStage::Transfer,
				BarrierStage::Host,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						terrainVirtualTileState.pageUpdateReadbackBuffer,
						BarrierAccess::TransferWrite,
						BarrierAccess::HostRead,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				},
				"Page Readback Buffer Barrier");

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

			CommandBufferEndMarker(GraphicsQueueType);
		});

		{
			if (terrainVirtualTileState.updateLowres && CoreGraphics::EventPoll(terrainVirtualTileState.biomeUpdatedEvent))
		Frame::AddCallback("TerrainContext - Update Physical Texture Cache", [](const IndexT frame, const IndexT frameBufferIndex)
			{
				CoreGraphics::EventHostReset(terrainVirtualTileState.biomeUpdatedEvent);
				terrainVirtualTileState.updateLowres = false;

				CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Lowres Update");

				// setup state for update
				SetShaderProgram(terrainVirtualTileState.terrainLowresGeneratePass);
				SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);
				SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

				// update texture
				CoreGraphics::Compute(LowresFallbackSize / 8, LowresFallbackSize / 8, 1);

				BarrierInsert(GraphicsQueueType,
					BarrierStage::ComputeShader,
					BarrierStage::Transfer,
					BarrierDomain::Global,
					{
						TextureBarrier
						{
							terrainVirtualTileState.lowresAlbedo,
							ImageSubresourceInfo::ColorNoMipNoLayer(),
							ImageLayout::General,
							ImageLayout::TransferSource,
							BarrierAccess::ShaderWrite,
							BarrierAccess::TransferWrite
						},
						TextureBarrier
						{
							terrainVirtualTileState.lowresNormal,
							ImageSubresourceInfo::ColorNoMipNoLayer(),
							ImageLayout::General,
							ImageLayout::TransferSource,
							BarrierAccess::ShaderWrite,
							BarrierAccess::TransferWrite
						},
						TextureBarrier
						{
							terrainVirtualTileState.lowresMaterial,
							ImageSubresourceInfo::ColorNoMipNoLayer(),
							ImageLayout::General,
							ImageLayout::TransferSource,
							BarrierAccess::ShaderWrite,
							BarrierAccess::TransferWrite
						},
					},
					nullptr,
					"Lowres Fallback Barrier");

				// generate mipmaps for all the lowres textures
				CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresAlbedo);
				CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresNormal);
				CoreGraphics::TextureGenerateMipmaps(terrainVirtualTileState.lowresMaterial);

				CommandBufferEndMarker(GraphicsQueueType);
			}

			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Update Physical Texture Cache");

			SetShaderProgram(terrainVirtualTileState.terrainTileSplattingProgram);
			SetResourceTable(terrainVirtualTileState.virtualTerrainSystemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);
			Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

			BarrierInsert(GraphicsQueueType,
				BarrierStage::PixelShader,
				BarrierStage::ComputeShader,
				BarrierDomain::Global,
				{
					TextureBarrier
					{
						terrainVirtualTileState.physicalAlbedoCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::ShaderRead,
						ImageLayout::General,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite
					},
					TextureBarrier
					{
						terrainVirtualTileState.physicalNormalCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::ShaderRead,
						ImageLayout::General,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite
					},
					TextureBarrier
					{
						terrainVirtualTileState.physicalMaterialCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::ShaderRead,
						ImageLayout::General,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite
					},
				},
				nullptr,
				"Virtual Physical Texture Cache Barrier");

			// go through pending page updates and render into the physical texture caches
			for (IndexT i = 0; i < runtimes.Size(); i++)
			{
				TerrainRuntimeInfo& rt = runtimes[i];
				SetResourceTable(terrainVirtualTileState.virtualTerrainRuntimeResourceTable, NEBULA_BATCH_GROUP, ComputePipeline, nullptr);

				for (int j = 0; j < terrainVirtualTileState.pageUpdatesThisFrame.Size(); j++)
				{
					PhysicalPageUpdate& pageUpdate = terrainVirtualTileState.pageUpdatesThisFrame[j];
					SetResourceTable(terrainVirtualTileState.virtualTerrainDynamicResourceTable, NEBULA_DYNAMIC_OFFSET_GROUP, ComputePipeline, 2, pageUpdate.constantBufferOffsets);

					// run the compute shader which updates the tiles
					Compute(PhysicalTextureTilePaddedSize / 8, PhysicalTextureTilePaddedSize / 8, 1);
				}
				terrainVirtualTileState.pageUpdatesThisFrame.Clear();
			}

			BarrierInsert(GraphicsQueueType,
				BarrierStage::ComputeShader,
				BarrierStage::PixelShader,
				BarrierDomain::Global,
				{
					TextureBarrier
					{
						terrainVirtualTileState.physicalAlbedoCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::General,
						ImageLayout::ShaderRead,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderRead
					},
					TextureBarrier
					{
						terrainVirtualTileState.physicalNormalCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::General,
						ImageLayout::ShaderRead,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderRead
					},
					TextureBarrier
					{
						terrainVirtualTileState.physicalMaterialCache,
						ImageSubresourceInfo::ColorNoMipNoLayer(),
						ImageLayout::General,
						ImageLayout::ShaderRead,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderRead
					},
				},
				nullptr,
				"Virtual Physical Texture Cache Barrier");

			CommandBufferEndMarker(GraphicsQueueType);

			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_RED, "Terrain Update Indirection Texture");

			/*
			BarrierInsert(GraphicsQueueType,
				BarrierStage::PixelShader,
				BarrierStage::PixelShader,
				BarrierDomain::Global,
				{
					TextureBarrier
					{
						terrainVirtualTileState.indirectionTexture,
						ImageSubresourceInfo{ImageAspect::ColorBits, 0, IndirectionNumMips, 0, 1},
						ImageLayout::ShaderRead,
						ImageLayout::TransferDestination,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite
					},
				},
				nullptr,
				"Indirection Texture Update Barrier");

			Math::rectangle<SizeT> copyArea;
			copyArea.left = 0;
			copyArea.top = 0;
			CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);
			for (IndexT i = 0; i < terrainVirtualTileState.indirectionTextureMipNeedsUpdate.Size(); i++)
			{
				copyArea.right = dims.width >> i;
				copyArea.bottom = dims.height >> i;
				if (terrainVirtualTileState.indirectionTextureMipNeedsUpdate[i])
				{
					Copy(GraphicsQueueType,
						terrainVirtualTileState.indirectionUploadBuffers[i][CoreGraphics::GetBufferedFrameIndex()], 
						0, 
						terrainVirtualTileState.indirectionTexture, 
						copyArea, 
						i, 0);
				}
			}

			BarrierInsert(GraphicsQueueType,
				BarrierStage::PixelShader,
				BarrierStage::PixelShader,
				BarrierDomain::Global,
				{
					TextureBarrier
					{
						terrainVirtualTileState.indirectionTexture,
						ImageSubresourceInfo{ImageAspect::ColorBits, 0, IndirectionNumMips, 0, 1},
						ImageLayout::TransferDestination,
						ImageLayout::ShaderRead,
						BarrierAccess::ShaderWrite,
						BarrierAccess::ShaderRead
					},
				},
				nullptr,
				"Indirection Texture Update Barrier");
				*/

			CommandBufferEndMarker(GraphicsQueueType);
		});

	Frame::AddCallback("TerrainContext - Screen Space Resolve", [](const IndexT frame, const IndexT frameBufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_BLUE, "Terrain Screenspace Pass");
			SetShaderProgram(terrainVirtualTileState.terrainScreenspacePass);

			CoreGraphics::BeginBatch(Frame::FrameBatchType::System);
			terrainVirtualTileState.fsq.ApplyMesh();
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
	terrainState.biomeCounter = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetupTerrain(
	const Graphics::GraphicsEntityId entity, 
	const Resources::ResourceName& heightMap, 
	const Resources::ResourceName& decisionMap,
	const Resources::ResourceName& albedoMap,
	const TerrainSetupSettings& settings)
{
	n_assert(settings.worldSizeX > 0);
	n_assert(settings.worldSizeZ > 0);
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

	runtimeInfo.worldWidth = settings.worldSizeX;
	runtimeInfo.worldHeight = settings.worldSizeZ;
	runtimeInfo.maxHeight = settings.maxHeight;
	runtimeInfo.minHeight = settings.minHeight;
	//runtimeInfo.albedoSource.scaleFactorX = TextureGetDimensions(runtimeInfo.albedoSource.tex).width / settings.worldSizeX;
	//runtimeInfo.albedoSource.scaleFactorY = TextureGetDimensions(runtimeInfo.albedoSource.tex).height / settings.worldSizeZ;
	//runtimeInfo.heightSource.scaleFactorX = TextureGetDimensions(runtimeInfo.heightSource.tex).width / settings.worldSizeX;
	//runtimeInfo.heightSource.scaleFactorY = TextureGetDimensions(runtimeInfo.heightSource.tex).height / settings.worldSizeZ;


	// divide world dimensions into 
	runtimeInfo.numTilesX = settings.worldSizeX / settings.tileWidth;
	runtimeInfo.numTilesY = settings.worldSizeZ / settings.tileHeight;
	runtimeInfo.tileWidth = settings.tileWidth;
	runtimeInfo.tileHeight = settings.tileHeight;
	SizeT height = settings.maxHeight - settings.minHeight;
	SizeT numVertsX = settings.tileWidth / settings.vertexDensityX + 1;
	SizeT numVertsY = settings.tileHeight / settings.vertexDensityY + 1;
	SizeT vertDistanceX = settings.vertexDensityX;
	SizeT vertDistanceY = settings.vertexDensityY;
	SizeT numBufferedFrame = CoreGraphics::GetNumBufferedFrames();

	// setup resource tables, one for the per-chunk draw arguments, and one for the whole terrain 
	runtimeInfo.patchTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);
	runtimeInfo.terrainConstants = ShaderCreateConstantBuffer(terrainState.terrainShader, "TerrainRuntimeUniforms");
	runtimeInfo.terrainResourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
	IndexT constantsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TerrainRuntimeUniforms");
	IndexT feedbackSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TextureLODReadback");

	IndexT slot = ShaderGetResourceSlot(terrainState.terrainShader, "PatchUniforms");
	ResourceTableSetConstantBuffer(runtimeInfo.terrainResourceTable, { runtimeInfo.terrainConstants, constantsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
	ResourceTableSetConstantBuffer(runtimeInfo.patchTable, { CoreGraphics::GetGraphicsConstantBuffer(CoreGraphics::MainThreadConstantBuffer), slot, 0, true, false, sizeof(Terrain::PatchUniforms), 0 });
	ResourceTableCommitChanges(runtimeInfo.patchTable);
	ResourceTableCommitChanges(runtimeInfo.terrainResourceTable);

	// allocate a tile vertex buffer
	TerrainVert* verts = (TerrainVert*)Memory::Alloc(Memory::ResourceHeap, numVertsX * numVertsY * sizeof(TerrainVert));

	// allocate terrain index buffer, every fourth pixel will generate two triangles 
	SizeT numTris = numVertsX * numVertsY;
	TerrainQuad* quads = (TerrainQuad*)Memory::Alloc(Memory::ResourceHeap, numTris * sizeof(TerrainQuad));

	// setup sections
	for (uint y = 0; y < runtimeInfo.numTilesY; y++)
	{
		for (uint x = 0; x < runtimeInfo.numTilesX; x++)
		{
			Math::bbox box;
			box.set(
				Math::point(
					x * settings.tileWidth - settings.worldSizeX / 2 + settings.tileWidth / 2,
					settings.minHeight, 
					y * settings.tileHeight - settings.worldSizeZ / 2 + settings.tileHeight / 2
				),
				Math::vector(settings.tileWidth / 2, height, settings.tileHeight / 2));
			runtimeInfo.sectionBoxes.Append(box);
			runtimeInfo.sectorVisible.Append(true);
			runtimeInfo.sectorLod.Append(1000.0f);
			runtimeInfo.sectorUpdateTextureTile.Append(false);
			runtimeInfo.sectorUniformOffsets.Append(Util::FixedArray<uint>(2, 0));
			runtimeInfo.sectorTileOffsets.Append(Util::FixedArray<uint>(2, 0));
			runtimeInfo.sectorAllocatedTile.Append(Math::uint3{ UINT32_MAX, UINT32_MAX, UINT32_MAX });
			runtimeInfo.sectorTextureTileSize.Append(Math::uint2{ 0, 0 });
			runtimeInfo.sectorUv.Append({ x * settings.tileWidth / settings.worldSizeX, y * settings.tileHeight / settings.worldSizeZ });

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
			v1.uv = Math::vec2(x * vertDistanceX / float(settings.tileWidth), y * vertDistanceY / float(settings.tileHeight));

			v2.position.set((x + 1) * vertDistanceX, 0, y * vertDistanceY, 1);
			v2.uv = Math::vec2((x + 1) * vertDistanceX / float(settings.tileWidth), y * vertDistanceY / float(settings.tileHeight));

			v3.position.set(x * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
			v3.uv = Math::vec2(x * vertDistanceX / float(settings.tileWidth), (y + 1) * vertDistanceY / float(settings.tileHeight));

			v4.position.set((x + 1) * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
			v4.uv = Math::vec2((x + 1) * vertDistanceX / float(settings.tileWidth), (y + 1) * vertDistanceY / float(settings.tileHeight));

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
	vboInfo.data = verts;
	vboInfo.dataSize = numVertsX * numVertsY * sizeof(TerrainVert);
	runtimeInfo.vbo = CreateBuffer(vboInfo);

	// create ibo
	BufferCreateInfo iboInfo;
	iboInfo.name = "terrain_ibo"_atm;
	iboInfo.size = numTris * 4;
	iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
	iboInfo.mode = CoreGraphics::HostToDevice;
	iboInfo.usageFlags = CoreGraphics::IndexBuffer;
	iboInfo.data = quads;
	iboInfo.dataSize = numTris * sizeof(TerrainQuad);
	runtimeInfo.ibo = CreateBuffer(iboInfo);

	Memory::Free(Memory::ResourceHeap, verts);
	Memory::Free(Memory::ResourceHeap, quads);
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
	arrayTexInfo.mips = CoreGraphics::TextureAutoMips;	// let coregraphics figure out how many mips this is
	arrayTexInfo.layers = 4;								// use 4 layers, one for each material

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
				CoreGraphics::TextureCopy(terrainState.biomeAlbedoArray[terrainState.biomeCounter], to, j, i, albedo, from, j, 0, sub);
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
				CoreGraphics::TextureCopy(terrainState.biomeNormalArray[terrainState.biomeCounter], to, j, i, normal, from, j, 0, sub);
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
				CoreGraphics::TextureCopy(terrainState.biomeMaterialArray[terrainState.biomeCounter], to, j, i, material, from, j, 0, sub);
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
	Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetTransform(view->GetCamera()));
	const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
	Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();

	for (IndexT i = 0; i < runtimes.Size(); i++)
	{
		TerrainRuntimeInfo& rt = runtimes[i];

		// start job for sections
		Jobs::JobContext ctx;

		// uniform data is the observer transform
		ctx.uniform.numBuffers = 1;
		ctx.uniform.data[0] = (unsigned char*)&viewProj;
		ctx.uniform.dataSize[0] = sizeof(Math::mat4);
		ctx.uniform.scratchSize = 0;

		// just one input buffer of all the transforms
		ctx.input.numBuffers = 1;
		ctx.input.data[0] = (unsigned char*)rt.sectionBoxes.Begin();
		ctx.input.dataSize[0] = sizeof(Math::bbox) * rt.sectionBoxes.Size();
		ctx.input.sliceSize[0] = sizeof(Math::bbox);

		// the output is the visibility result
		ctx.output.numBuffers = 1;
		ctx.output.data[0] = (unsigned char*)rt.sectorVisible.Begin();
		ctx.output.dataSize[0] = sizeof(bool) * rt.sectorVisible.Size();
		ctx.output.sliceSize[0] = sizeof(bool);

		// create and run job
		Jobs::JobId job = Jobs::CreateJob({ TerrainCullJob });
		Jobs::JobSchedule(job, Graphics::GraphicsServer::renderSystemsJobPort, ctx);
		Jobs::JobSyncSignal(TerrainContext::jobHostSync, Graphics::GraphicsServer::renderSystemsJobPort);

		TerrainContext::runningJobs.Enqueue(job);
	}
}


//------------------------------------------------------------------------------
/**
	Unpack from packed ushort vectors to full size
*/
void
UnpackPageDataEntry(uint* packed, uint& status, uint& subTextureTile, uint& mip, uint& pageCoordX, uint& pageCoordY, uint& subTextureTileX, uint& subTextureTileY)
{
	status = packed[0] & 0x3;
	subTextureTile = packed[0] >> 2;
	mip = packed[1];
	pageCoordX = packed[2] & 0xFFFF;
	pageCoordY = packed[2] >> 16;
	subTextureTileX = packed[3] & 0xFFFF;
	subTextureTileY = packed[3] >> 16;
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

	CoreGraphics::TextureId dataBuffer = view->GetFrameScript()->GetTexture("TerrainDataBuffer");
	CoreGraphics::TextureId normalBuffer = view->GetFrameScript()->GetTexture("TerrainNormalBuffer");
	CoreGraphics::TextureId posBuffer = view->GetFrameScript()->GetTexture("TerrainPosBuffer");

	Terrain::TerrainSystemUniforms systemUniforms;
	systemUniforms.TerrainDataBuffer = CoreGraphics::TextureGetBindlessHandle(dataBuffer);
	systemUniforms.TerrainNormalBuffer = CoreGraphics::TextureGetBindlessHandle(normalBuffer);
	systemUniforms.TerrainPosBuffer = CoreGraphics::TextureGetBindlessHandle(posBuffer);
	systemUniforms.MinLODDistance = 0.0f;
	systemUniforms.MaxLODDistance = terrainState.mipLoadDistance;
	systemUniforms.VirtualLodDistance = terrainState.mipLoadDistance;
	systemUniforms.MinTessellation = 1.0f;
	systemUniforms.MaxTessellation = 32.0f;
	systemUniforms.Debug = terrainState.debugRender;
	systemUniforms.NumLayers = terrainState.materials.Size();
	systemUniforms.NumBiomes = terrainState.biomeCounter;
	systemUniforms.FrameIndex = CoreGraphics::GetBufferedFrameIndex();
	systemUniforms.UpdateIndex = CoreGraphics::GetBufferedFrameIndex() % 2;
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

	// evaluate tile updates for this frame
	for (IndexT j = 0; j < terrainVirtualTileState.virtualSubtextures.Size(); j++)
	{
		Terrain::VirtualTerrainSubTexture& subTex = terrainVirtualTileState.virtualSubtextures[j];

		// control the maximum resolution as such, to get 10.24 texels/cm, we need to have 65535 pixels (theoretical) for a 64 meter region
		const uint maxResolution = 64 * 1024;

		// distance where we should switch lods, set it to every 10 meters
		const float switchDistance = 2.0f;

		// mask out y coordinate by multiplying result with, 1, 0 ,1
		Math::vec4 min = Math::vec4(subTex.worldCoordinate[0], 0, subTex.worldCoordinate[1], 0);
		Math::vec4 max = min + Math::vec4(64.0f, 0.0f, 64.0f, 0.0f);
		Math::vec4 cameraXZ = Math::ceil(cameraTransform.position * Math::vec4(1, 0, 1, 0));
		Math::vec4 nearestPoint = Math::minimize(Math::maximize(cameraXZ, min), max);
		float distance = length(nearestPoint - cameraXZ);
		uint t = (distance / switchDistance);
		uint lod = 0;
		if (t >= 1)
			lod = Math::n_log2(t) + 1;

		uint resolution = maxResolution >> lod;
		if (distance > 300)
			resolution = 0;
		uint tiles = resolution / 256;			// because every pixel in the indirection covers a 256x256 physical tile, this is our final pixels

		// if the resolution does not match, update
		bool deallocated = false;
		terrainVirtualTileState.indirectionTextureMipNeedsUpdate.Fill(false);
		if (subTex.size != tiles)
		{
			// if subtex has a valid size, deallocate it
			if (subTex.size != UINT32_MAX)
			{
				Math::uint2 coord = { subTex.indirectionOffset[0], subTex.indirectionOffset[1] };
				terrainVirtualTileState.indirectionOccupancy.Deallocate(coord, subTex.size);
				deallocated = true;
				subTex.size = UINT32_MAX;
				subTex.indirectionOffset[0] = 0xFFFFFFFF;
				subTex.indirectionOffset[1] = 0xFFFFFFFF;
			}

			// now allocate a new tile if within range
			if (resolution >= 1024)
			{
				Math::uint2 coord = terrainVirtualTileState.indirectionOccupancy.Allocate(tiles);
				Math::uint2 oldCoord = { subTex.indirectionOffset[0], subTex.indirectionOffset[1] };
				n_assert(coord.x != UINT32_MAX && coord.y != UINT32_MAX);
				subTex.indirectionOffset[0] = coord.x;
				subTex.indirectionOffset[1] = coord.y;

				//subTex.metersPerTile = SubTextureTileWorldSize / float(tiles);
				subTex.mips = Math::n_log2(tiles) + 1;
				subTex.size = tiles;

				// if deallocated, initialize a copy of the indirection pixels between mips
				if (deallocated)
				{
					terrainVirtualTileState.indirectionTextureMipNeedsUpdate[lod] = true;
					if (subTex.size > tiles)
					{
						// if downscale, copy whole mipchain of indirection shifted up, cutting off the last mip
						// this should prevent unnecessary tile updates for a lot of tiles
					}
					else
					{
						// if upscale, copy whole mipchain of indirection shifted down, leaving mip 0 to be updated later

					}
				}
			}
		}
	}

	// update virtual sub textures buffer
	BufferUpdateArray(
		terrainVirtualTileState.virtualSubtextureBuffer,
		terrainVirtualTileState.virtualSubtextures.Begin(),
		terrainVirtualTileState.virtualSubtextures.Size(),
		CoreGraphics::GetBufferedFrameIndex() * terrainVirtualTileState.virtualSubtextures.Size() * sizeof(Terrain::VirtualTerrainSubTexture));
	BufferFlush(
		terrainVirtualTileState.virtualSubtextureBuffer,
		CoreGraphics::GetBufferedFrameIndex() * terrainVirtualTileState.virtualSubtextures.Size() * sizeof(Terrain::VirtualTerrainSubTexture),
		terrainVirtualTileState.virtualSubtextures.Size() * sizeof(Terrain::VirtualTerrainSubTexture));

	// handle readback from the GPU
	uint frameIndex = CoreGraphics::GetBufferedFrameIndex();
	CoreGraphics::BufferInvalidate(terrainVirtualTileState.pageUpdateReadbackBuffer, frameIndex * sizeof(Terrain::PageUpdateList), sizeof(Terrain::PageUpdateList));
	Terrain::PageUpdateList* updateList = (Terrain::PageUpdateList*)CoreGraphics::BufferMap(terrainVirtualTileState.pageUpdateReadbackBuffer);
	CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);

	terrainVirtualTileState.numPixels = updateList[frameIndex].NumEntries;
	for (uint i = 0; i < updateList[frameIndex].NumEntries; i++)
	{
		uint status, subTextureTile, mip, pageCoordX, pageCoordY, subTextureTileX, subTextureTileY;
		UnpackPageDataEntry(updateList[frameIndex].Entry[i], status, subTextureTile, mip, pageCoordX, pageCoordY, subTextureTileX, subTextureTileY);

		// the update state is either 1 if the page is allocated, or 2 if it used to be allocated but has since been deallocated
		uint updateState = status;
		uint subTextureIndex = subTextureTile;
		uint index = pageCoordX + pageCoordY * (dims.width >> mip);
		IndirectionEntry& entry = terrainVirtualTileState.indirection[mip][index];

		// if updateState is 2, it means it was previously allocated and should be deallocated
		if (updateState == 2 && entry.mip != 0xF)
		{
			bool res = terrainVirtualTileState.physicalTextureTileOccupancy.Deallocate(Math::uint2{ entry.physicalOffsetX, entry.physicalOffsetY }, PhysicalTextureTilePaddedSize);
			n_assert(res);

			entry.mip = 0xF;
			entry.physicalOffsetX = 0xFFFFFFFF;
			entry.physicalOffsetY = 0xFFFFFFFF;
		}

		// if update state is 1, it means we should allocate a tile
		if (updateState == 1 && entry.mip == 0xF)
		{
			Math::uint2 physicalCoord = terrainVirtualTileState.physicalTextureTileOccupancy.Allocate(PhysicalTextureTilePaddedSize);
			if (physicalCoord.x != 0xFFFFFFFF && physicalCoord.y != 0xFFFFFFFF)
			{
				VirtualTerrainSubTexture& subTexture = terrainVirtualTileState.virtualSubtextures[subTextureIndex];
				if (subTexture.size == UINT32_MAX)
					continue;
				//n_assert(subTexture.size != UINT32_MAX);

				entry.mip = mip;
				entry.physicalOffsetX = physicalCoord.x;
				entry.physicalOffsetY = physicalCoord.y;

				Terrain::TerrainTileUpdateUniforms tileUpdateUniforms;

				// calculate the world space size of this tile
				float metersPerTile = SubTextureTileWorldSize / float(Math::n_max(1u, subTexture.size >> entry.mip));

				// pagePos is the relative page id into the subtexture, ranging from 0-subTexture.size
				tileUpdateUniforms.SparseTileWorldOffset[0] = subTexture.worldCoordinate[0] + subTextureTileX * metersPerTile;
				tileUpdateUniforms.SparseTileWorldOffset[1] = subTexture.worldCoordinate[1] + subTextureTileY * metersPerTile;
				tileUpdateUniforms.SparseTileOutputOffset[0] = entry.physicalOffsetX;
				tileUpdateUniforms.SparseTileOutputOffset[1] = entry.physicalOffsetY;
				tileUpdateUniforms.SparseTileIndirectionOffset[0] = pageCoordX;
				tileUpdateUniforms.SparseTileIndirectionOffset[1] = pageCoordY;
				tileUpdateUniforms.Mip = entry.mip;
					
				// divide by tile texture since since it's our output texture size
				// then multiply by 
				tileUpdateUniforms.PixelsPerMeter = (1.0f / PhysicalTextureTileSize) * metersPerTile;
					
				terrainVirtualTileState.pageUniforms.Append(tileUpdateUniforms);
			}
		}

	}
	CoreGraphics::BufferUnmap(terrainVirtualTileState.pageUpdateReadbackBuffer);

	// update indirection buffer, which is then uploaded to the texture 
	for (SizeT i = 0; i < terrainVirtualTileState.indirectionUploadBuffers.Size(); i++)
	{
		if (terrainVirtualTileState.indirectionTextureMipNeedsUpdate[i])
		{
			CoreGraphics::BufferUpdateArray(terrainVirtualTileState.indirectionUploadBuffers[i][frameIndex], terrainVirtualTileState.indirection[i].Begin(), terrainVirtualTileState.indirection[i].Size(), 0);
			CoreGraphics::BufferFlush(terrainVirtualTileState.indirectionUploadBuffers[i][frameIndex]);
		}
	}

	IndexT i;
	SizeT numPagesThisFrame = Math::n_min(16, terrainVirtualTileState.pageUniforms.Size());
	for (i = 0; i < numPagesThisFrame; i++)
	{
		PhysicalPageUpdate pageUpdate;
		pageUpdate.constantBufferOffsets[0] = CoreGraphics::SetGraphicsConstants(CoreGraphics::MainThreadConstantBuffer, terrainVirtualTileState.pageUniforms[i]);
		pageUpdate.constantBufferOffsets[1] = 0;
		terrainVirtualTileState.pageUpdatesThisFrame.Append(pageUpdate);
	}
	if (i > 0)
		terrainVirtualTileState.pageUniforms.EraseRange(0, numPagesThisFrame-1);

	// if we have running jobs for visibility, wait for them here
	if (TerrainContext::runningJobs.Size() > 0)
	{
		// wait for all jobs to finish
		Jobs::JobSyncHostWait(TerrainContext::jobHostSync);

		// destroy jobs
		while (!TerrainContext::runningJobs.IsEmpty())
			Jobs::DestroyJob(TerrainContext::runningJobs.Dequeue());
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
#ifdef USE_SPARSE
		//uniforms.AlbedoMap = TextureGetBindlessHandle(rt.albedoSource.tex);
		uniforms.AlbedoMap = TextureGetBindlessHandle(rt.lowResAlbedoMap);
		uniforms.HeightMap = TextureGetBindlessHandle(rt.heightMap);
#else
		uniforms.AlbedoMap = 0;
		uniforms.HeightMap = 0;
#endif

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
		uniforms.VirtualTerrainSubTextureSize[0] = SubTextureTileWorldSize;
		uniforms.VirtualTerrainSubTextureSize[1] = SubTextureTileWorldSize;
		uniforms.VirtualTerrainNumSubTextures[0] = PhysicalTextureSize / SubTextureTileWorldSize;
		uniforms.VirtualTerrainNumSubTextures[1] = PhysicalTextureSize / SubTextureTileWorldSize;
		uniforms.PhysicalInvPaddedTextureSize = 1.0f / PhysicalTexturePaddedSize;
		uniforms.PhysicalTileSize = PhysicalTextureTileSize;
		uniforms.PhysicalTilePadding = PhysicalTextureTilePadding / 2;

		CoreGraphics::TextureDimensions indirectionDims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.indirectionTexture);
		uniforms.VirtualTerrainNumPages[0] = indirectionDims.width;
		uniforms.VirtualTerrainNumPages[1] = indirectionDims.height;
		uniforms.VirtualTerrainNumMips = IndirectionNumMips;

		for (SizeT j = 0; j < terrainVirtualTileState.indirectionMipOffsets.Size(); j++)
		{
			uniforms.VirtualPageBufferMipOffsets[j / 4][j % 4] = terrainVirtualTileState.indirectionMipOffsets[j];
			uniforms.VirtualPageBufferMipSizes[j / 4][j % 4] = terrainVirtualTileState.indirectionMipSizes[j];
		}
		uniforms.VirtualPageBufferNumPages = CoreGraphics::BufferGetSize(terrainVirtualTileState.pageUpdateBuffer);

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
		}

		{
			CoreGraphics::TextureDimensions dims = CoreGraphics::TextureGetDimensions(terrainVirtualTileState.physicalAlbedoCache);

			ImVec2 imageSize = { (float)dims.width, (float)dims.height };

			static Dynui::ImguiTextureId textureInfo;
			textureInfo.nebulaHandle = terrainVirtualTileState.physicalAlbedoCache.HashCode64();
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

//------------------------------------------------------------------------------
/**
*/
void 
TerrainTextureSource::Setup(const Resources::ResourceName& path, bool manualRegister)
{
	this->stream = IO::IoServer::Instance()->CreateStream(path.Value());
	this->stream->SetAccessMode(IO::Stream::ReadAccess);
	if (this->stream->Open())
	{
		void* buf = this->stream->MemoryMap();
		SizeT size = this->stream->GetSize();

		if (this->source.load(buf, size))
		{
			// okay, we're loaded!
			CoreGraphics::PixelFormat::Code nebulaFormat = CoreGraphics::Gliml::ToPixelFormat(this->source);

#ifdef USE_SPARSE
			// create virtual texture for albedo
			CoreGraphics::TextureCreateInfo info;
			info.name = path;
			info.tag = "render"_atm;
			info.width = this->source.image_width(0, 0);
			info.height = this->source.image_height(0, 0);
			info.sparse = true;
			info.mips = Math::n_max(0, this->source.num_mipmaps(0) - 7);
			info.layers = this->source.num_faces();
			info.format = nebulaFormat;
			info.bindless = !manualRegister;

			this->tex = CreateTexture(info);
			IndexT maxMip = CoreGraphics::TextureSparseGetMaxMip(this->tex);

			// create pages
			this->pageReferenceCount.Resize(terrainState.layers);
			for (int i = 0; i < terrainState.layers; i++)
			{
				this->pageReferenceCount[i].Resize(maxMip);

				for (int j = 0; j < maxMip; j++)
				{
					SizeT numPages = CoreGraphics::TextureSparseGetNumPages(this->tex, i, j);
					this->pageReferenceCount[i][j].Resize(numPages);
					this->pageReferenceCount[i][j].Fill(0);
				}

				for (int j = 0; j < this->source.num_mipmaps(i); j++)
					N_COUNTER_INCR(N_TERRAIN_TOTAL_AVAILABLE_DATA, this->source.image_size(i, j));
			}

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
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, (uint)maxMip - 1, (uint)(info.mips - maxMip), 0, 1 },
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::ImageLayout::TransferDestination,
						CoreGraphics::BarrierAccess::ShaderRead,
						CoreGraphics::BarrierAccess::TransferWrite,
					}
				},
				nullptr,
				nullptr);

			// update mips which are not paged
			for (int i = maxMip; i < info.mips; i++)
			{
				CoreGraphics::TextureUpdate(tex, i, 0, (char*)this->source.image_data(0, i), sub);
			}

			// transfer textures back to being read by shaders
			CoreGraphics::BarrierInsert(
				CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				CoreGraphics::BarrierStage::Transfer,
				CoreGraphics::BarrierStage::AllGraphicsShaders,
				CoreGraphics::BarrierDomain::Global,
				{
					CoreGraphics::TextureBarrier
					{
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, (uint)maxMip - 1, (uint)(info.mips - maxMip), 0, 1 },
						CoreGraphics::ImageLayout::TransferDestination,
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::BarrierAccess::TransferWrite,
						CoreGraphics::BarrierAccess::ShaderRead,
					}
				},
				nullptr,
				nullptr);

			// unlock handover submission
			CoreGraphics::UnlockResourceSubmission();
#endif
		}
		else
		{
			this->stream->MemoryUnmap();
			this->stream->Close();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainMaterialSource::Setup(const Resources::ResourceName& path)
{
	this->stream = IO::IoServer::Instance()->CreateStream(path.Value());
	this->stream->SetAccessMode(IO::Stream::ReadAccess);
	if (this->stream->Open())
	{
		void* buf = this->stream->MemoryMap();
		SizeT size = this->stream->GetSize();

		if (this->source.load(buf, size))
		{
			// okay, we're loaded!
			CoreGraphics::PixelFormat::Code nebulaFormat = CoreGraphics::Gliml::ToPixelFormat(this->source);

#ifdef USE_SPARSE
			// create virtual texture for albedo
			CoreGraphics::TextureCreateInfo info;
			info.name = path;
			info.tag = "render"_atm;
			info.width = this->source.image_width(0, 0);
			info.height = this->source.image_height(0, 0);
			info.sparse = true;
			info.mips = this->source.num_mipmaps(0);
			info.layers = this->source.num_faces();
			info.format = nebulaFormat;
			info.bindless = false;


			this->mipReferenceCount.Resize(this->source.num_faces());
			for (int i = 0; i < this->source.num_faces(); i++)
			{
				this->mipReferenceCount[i].Resize(this->source.num_mipmaps(i));
				this->mipReferenceCount[i].Fill(0);

				for (int j = 0; j < this->source.num_mipmaps(i); j++)
					N_COUNTER_INCR(N_TERRAIN_TOTAL_AVAILABLE_DATA, this->source.image_size(i, j));
			}

			this->tex = CreateTexture(info);

			IndexT maxMip = CoreGraphics::TextureSparseGetMaxMip(this->tex);

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
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, (uint)maxMip, (uint)(info.mips - maxMip), 0, 1 },
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::ImageLayout::TransferDestination,
						CoreGraphics::BarrierAccess::ShaderRead,
						CoreGraphics::BarrierAccess::TransferWrite,
					}
				},
				nullptr,
				nullptr);

			// update unpaged mips directly
			for (int i = maxMip; i < info.mips; i++)
			{
				CoreGraphics::TextureUpdate(tex, i, 0, (char*)this->source.image_data(0, i), sub);
			}

			// transfer textures back to being read by shaders
			CoreGraphics::BarrierInsert(
				CoreGraphics::SubmissionContextGetCmdBuffer(sub),
				CoreGraphics::BarrierStage::Transfer,
				CoreGraphics::BarrierStage::AllGraphicsShaders,
				CoreGraphics::BarrierDomain::Global,
				{
					CoreGraphics::TextureBarrier
					{
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, (uint)maxMip, (uint)(info.mips - maxMip), 0, 1 },
						CoreGraphics::ImageLayout::TransferDestination,
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::BarrierAccess::TransferWrite,
						CoreGraphics::BarrierAccess::ShaderRead,
					}
				},
				nullptr,
				nullptr);

			// unlock handover submission
			CoreGraphics::UnlockResourceSubmission();
#endif
		}
		else
		{
			this->stream->MemoryUnmap();
			this->stream->Close();
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
EvictMip(
	const CoreGraphics::TextureId tex,
	uint mip,
	CoreGraphics::TextureSparsePageSize pageSize,
	TerrainMaterialSource& source,
	CoreGraphics::SubmissionContextId sub)
{
	source.mipReferenceCount[0][mip]--;
	if (source.mipReferenceCount[0][mip] == 0)
		CoreGraphics::TextureSparseEvictMip(source.tex, 0, mip);
}

//------------------------------------------------------------------------------
/**
*/
void
MakeResidentMip(
	const CoreGraphics::TextureId tex,
	uint mip,
	CoreGraphics::TextureSparsePageSize pageSize,
	TerrainMaterialSource& source,
	CoreGraphics::SubmissionContextId sub)
{
	// if ref count is 0, make the mip resident and stream data
	if (source.mipReferenceCount[0][mip] == 0)
	{
		CoreGraphics::TextureSparseMakeMipResident(source.tex, 0, mip);

		CoreGraphics::BarrierInsert(
			CoreGraphics::SubmissionContextGetCmdBuffer(sub),
			CoreGraphics::BarrierStage::AllGraphicsShaders,
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierDomain::Global,
			{
				CoreGraphics::TextureBarrier
				{
					source.tex,
					CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, mip, 1, 0, 1 },
					CoreGraphics::ImageLayout::ShaderRead,
					CoreGraphics::ImageLayout::TransferDestination,
					CoreGraphics::BarrierAccess::ShaderRead,
					CoreGraphics::BarrierAccess::TransferWrite,
				}
			},
			nullptr,
			nullptr);

		// update the texture
		CoreGraphics::TextureUpdate(tex, mip, 0, (char*)source.source.image_data(0, mip), sub);

		// transfer textures back to being read by shaders
		CoreGraphics::BarrierInsert(
			CoreGraphics::SubmissionContextGetCmdBuffer(sub),
			CoreGraphics::BarrierStage::Transfer,
			CoreGraphics::BarrierStage::AllGraphicsShaders,
			CoreGraphics::BarrierDomain::Global,
			{
				CoreGraphics::TextureBarrier
				{
					source.tex,
					CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, mip, 1, 0, 1 },
					CoreGraphics::ImageLayout::TransferDestination,
					CoreGraphics::ImageLayout::ShaderRead,
					CoreGraphics::BarrierAccess::TransferWrite,
					CoreGraphics::BarrierAccess::ShaderRead,
				}
			},
			nullptr,
			nullptr);
	}

	// then add the reference counter
	source.mipReferenceCount[0][mip]++;
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainMaterialSource::UpdateMip(float oldDistance, float newDistance, CoreGraphics::SubmissionContextId sub)
{
	float oldMip = oldDistance * (CoreGraphics::TextureSparseGetMaxMip(this->tex) - 1);
	float newMip = newDistance * (CoreGraphics::TextureSparseGetMaxMip(this->tex) - 1);

	uint texMips = CoreGraphics::TextureSparseGetMaxMip(this->tex);

	uint oldBottomMip = Math::n_floor(oldMip);
	uint oldTopMip = Math::n_ceil(oldMip);
	uint newBottomMip = Math::n_floor(newMip);
	uint newTopMip = Math::n_ceil(newMip);

	CoreGraphics::TextureSparsePageSize pageSize = CoreGraphics::TextureSparseGetPageSize(this->tex);

	// check if lower mip doesn't match
	if (oldBottomMip != newBottomMip)
	{
		// okay, it means we should unload the lower mip in case it's valid
		if (oldBottomMip < texMips && oldBottomMip != newTopMip)
			EvictMip(tex, oldBottomMip, pageSize, *this, sub);

		// and load in the new lower mip
		MakeResidentMip(tex, newBottomMip, pageSize, *this, sub);
	}

	// check if upper mip doesn't match
	if (oldTopMip != newTopMip && newBottomMip != newTopMip)
	{
		// okay, it means we should unload the upper mip in case it's valid
		if (oldTopMip < texMips && oldTopMip != newBottomMip)
			EvictMip(tex, oldTopMip, pageSize, *this, sub);

		// and load in the new upper mip
		MakeResidentMip(tex, newTopMip, pageSize, *this, sub);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainMaterial::UpdateMip(float oldDistance, float newDistance, CoreGraphics::SubmissionContextId sub)
{
	this->albedo.UpdateMip(oldDistance, newDistance, sub);
	this->normals.UpdateMip(oldDistance, newDistance, sub);
	this->material.UpdateMip(oldDistance, newDistance, sub);
}

//------------------------------------------------------------------------------
/**
*/
void
EvictSection(
	const CoreGraphics::TextureId tex,
	Math::rectangle<uint> section,
	uint mip,
	CoreGraphics::TextureSparsePageSize pageSize,
	TerrainTextureSource& source,
	CoreGraphics::SubmissionContextId sub)
{
	// calculate page ranges offset by mip
	uint offsetX = section.left >> mip;
	uint rangeX = Math::n_min(Math::n_max(section.width() >> mip, 1u), pageSize.width);
	uint endX = offsetX + Math::n_max(section.width() >> mip, 1u);

	uint offsetY = section.top >> mip;
	uint rangeY = Math::n_min(Math::n_max(section.height() >> mip, 1u), pageSize.height);
	uint endY = offsetY + Math::n_max(section.height() >> mip, 1u);

	// go through pages and evict
	for (uint y = offsetY; y < endY; y += rangeY)
	{
		for (uint x = offsetX; x < endX; x += rangeX)
		{
			uint pageIndex = CoreGraphics::TextureSparseGetPageIndex(tex, 0, mip, x, y, 0);

			// decrease the reference count
			if (pageIndex != InvalidIndex && source.pageReferenceCount[0][mip][pageIndex] > 0)
			{
				source.pageReferenceCount[0][mip][pageIndex]--;

				// if reference count for this page is 0
				if (source.pageReferenceCount[0][mip][pageIndex] == 0)
					CoreGraphics::TextureSparseEvict(tex, 0, mip, pageIndex);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MakeResidentSection(
	const CoreGraphics::TextureId tex,
	Math::rectangle<uint> section,
	uint mip,
	CoreGraphics::TextureSparsePageSize pageSize,
	TerrainTextureSource& source,
	CoreGraphics::SubmissionContextId sub)
{
	// calculate page ranges offset by mip
	uint offsetX = section.left >> mip;
	uint rangeX = Math::n_min(Math::n_max(section.width() >> mip, 1u), pageSize.width);
	uint endX = offsetX + Math::n_max(section.width() >> mip, 1u);

	uint offsetY = section.top >> mip;
	uint rangeY = Math::n_min(Math::n_max(section.height() >> mip, 1u), pageSize.height);
	uint endY = offsetY + Math::n_max(section.height() >> mip, 1u);

	// insert barrier before starting our blits
	CoreGraphics::BarrierInsert(
		CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierDomain::Global,
		{
			CoreGraphics::TextureBarrier
			{
				tex,
				CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, mip, 1, 0, 1 },
				CoreGraphics::ImageLayout::ShaderRead,
				CoreGraphics::ImageLayout::TransferDestination,
				CoreGraphics::BarrierAccess::ShaderRead,
				CoreGraphics::BarrierAccess::TransferWrite,
			}
		},
		nullptr,
		nullptr);

	// go through pages and make resident
	for (uint y = offsetY; y < endY; y += rangeY)
	{
		for (uint x = offsetX; x < endX; x += rangeX)
		{
			uint pageIndex = CoreGraphics::TextureSparseGetPageIndex(tex, 0, mip, x, y, 0);

			Math::rectangle<int> updateSection;
			updateSection.left = x;
			updateSection.right = x + rangeX;
			updateSection.top = y;
			updateSection.bottom = y + rangeY;

			// if this will be our first reference, make the page resident
			if (pageIndex != InvalidIndex)
			{
				if (source.pageReferenceCount[0][mip][pageIndex] == 0)
				{
					CoreGraphics::TextureSparseMakeResident(tex, 0, mip, pageIndex);
					const CoreGraphics::TextureSparsePage& page = CoreGraphics::TextureSparseGetPage(tex, 0, mip, pageIndex);

					// if we are allocating the page, fill it immediately because it may contain old data
					updateSection.left = Math::n_min(updateSection.left, (int32_t)page.offset.x);
					updateSection.right = Math::n_max((int32_t)(page.offset.x + page.extent.width), updateSection.right);
					updateSection.top = Math::n_min(updateSection.top, (int32_t)page.offset.y);
					updateSection.bottom = Math::n_max((int32_t)(page.offset.y + page.extent.height), updateSection.bottom);
				}

				// add a reference count
				source.pageReferenceCount[0][mip][pageIndex]++;
			}

			// update the texture
			CoreGraphics::TextureUpdate(tex, updateSection, mip, 0, (char*)source.source.image_data(0, mip), sub);
		}
	}

	// transfer textures back to being read by shaders
	CoreGraphics::BarrierInsert(
		CoreGraphics::SubmissionContextGetCmdBuffer(sub),
		CoreGraphics::BarrierStage::Transfer,
		CoreGraphics::BarrierStage::AllGraphicsShaders,
		CoreGraphics::BarrierDomain::Global,
		{
			CoreGraphics::TextureBarrier
			{
				tex,
				CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, mip, 1, 0, 1 },
				CoreGraphics::ImageLayout::TransferDestination,
				CoreGraphics::ImageLayout::ShaderRead,
				CoreGraphics::BarrierAccess::TransferWrite,
				CoreGraphics::BarrierAccess::ShaderRead,
			}
		},
		nullptr,
		nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateSparseTexture(
	const CoreGraphics::TextureId tex,
	Math::rectangle<uint> section,
	float oldDistance,
	float newDistance,
	CoreGraphics::TextureSparsePageSize pageSize,
	TerrainTextureSource& source,
	CoreGraphics::SubmissionContextId sub)
{
	float oldMip = oldDistance * (CoreGraphics::TextureSparseGetMaxMip(source.tex) - 1);
	float newMip = newDistance * (CoreGraphics::TextureSparseGetMaxMip(source.tex) - 1);

	uint texMips = CoreGraphics::TextureSparseGetMaxMip(source.tex);

	uint oldBottomMip = Math::n_floor(oldMip);
	uint oldTopMip = Math::n_ceil(oldMip);
	uint newBottomMip = Math::n_floor(newMip);
	uint newTopMip = Math::n_ceil(newMip);

	section.left *= source.scaleFactorX;
	section.right *= source.scaleFactorX;
	section.top *= source.scaleFactorY;
	section.bottom *= source.scaleFactorY;

	// check if lower mip doesn't match
	if (oldBottomMip != newBottomMip)
	{
		// okay, it means we should unload the lower mip in case it's valid
		if (oldBottomMip < texMips && oldBottomMip != newTopMip)
			EvictSection(tex, section, oldBottomMip, pageSize, source, sub);

		// and load in the new lower mip
		MakeResidentSection(tex, section, newBottomMip, pageSize, source, sub);
	}

	// check if upper mip doesn't match
	if (oldTopMip != newTopMip && newBottomMip != newTopMip)
	{
		// okay, it means we should unload the upper mip in case it's valid
		if (oldTopMip < texMips && oldTopMip != newBottomMip)
			EvictSection(tex, section, oldTopMip, pageSize, source, sub);

		// and load in the new upper mip
		MakeResidentSection(tex, section, newTopMip, pageSize, source, sub);
	}
}

} // namespace Terrain
