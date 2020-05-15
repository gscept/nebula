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

#include "resources/resourceserver.h"

#include "terrain.h"

#define USE_SPARSE

namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
_ImplementContext(TerrainContext, TerrainContext::terrainAllocator);

struct
{
	CoreGraphics::ShaderId terrainShader;
	CoreGraphics::ShaderProgramId terrainProgram;
	CoreGraphics::ResourceTableId resourceTable;
	CoreGraphics::ConstantBufferId constants;
	Util::Array<CoreGraphics::VertexComponent> components;
	CoreGraphics::VertexLayoutId vlo;
	Util::FixedArray<Util::FixedArray<Util::FixedArray<uint>>> pageReferenceCount;

	CoreGraphics::TextureId virtualAlbedoMap;
	SizeT mips;
	SizeT layers;

} terrainState;

struct TerrainVert
{
	Math::float3 position;
	Math::float2 uv;
};

struct TerrainTri
{
	IndexT a, b, c;
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
TerrainContext::Create()
{
	_CreateContext();
	using namespace CoreGraphics;

	__bundle.OnPrepareView = TerrainContext::CullPatches;
	__bundle.OnBeforeView = TerrainContext::UpdateVirtualTexture;

#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Frame::AddCallback("TerrainContext - Render Terrain", [](IndexT time)
		{
			// setup shader state, set shader before we set the vertex layout
			SetShaderProgram(terrainState.terrainProgram);
			SetVertexLayout(terrainState.vlo);
			SetPrimitiveTopology(PrimitiveTopology::PatchList);
			SetGraphicsPipeline();

			// set resources
			SetResourceTable(terrainState.resourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, "Terrain Sections");

			// go through and render terrain instances
			Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
			for (IndexT i = 0; i < runtimes.Size(); i++)
			{
				TerrainRuntimeInfo& rt = runtimes[i];

				Terrain::TerrainUniforms uniforms;
				Math::mat4().store(uniforms.Transform);
				uniforms.TerrainDimensions[0] = rt.heightMapWidth;
				uniforms.TerrainDimensions[1] = rt.heightMapHeight;
				uniforms.HeightMap = TextureGetBindlessHandle(rt.heightMap);
				uniforms.NormalMap = TextureGetBindlessHandle(rt.normalMap);
				uniforms.DecisionMap = TextureGetBindlessHandle(rt.decisionMap);
#ifdef USE_SPARSE
				uniforms.AlbedoMap = TextureGetBindlessHandle(terrainState.virtualAlbedoMap);
#else
				uniforms.AlbedoMap = 0;
#endif
				uniforms.MinLODDistance = 1.0f;
				uniforms.MaxLODDistance = 100.0f;
				uniforms.MinTessellation = 1.0f;
				uniforms.MaxTessellation = 8.0f;
				uniforms.VirtualTextureMips = terrainState.mips;
				uniforms.MaxHeight = rt.maxHeight;
				uniforms.MinHeight = rt.minHeight;
				ConstantBufferUpdate(terrainState.constants, uniforms, 0);

				for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
				{
					if (rt.sectorVisible[j])
					{
						SetStreamVertexBuffer(0, rt.vbo, 0);
						SetIndexBuffer(rt.ibo, 0);
						SetPrimitiveGroup(rt.sectorPrimGroups[j]);
						SetResourceTable(rt.patchTable, NEBULA_DYNAMIC_OFFSET_GROUP, GraphicsPipeline, 1, &rt.sectorUniformOffsets[j]);
						Draw();
					}
				}
			}

			CommandBufferEndMarker(GraphicsQueueType);
		});

	Frame::AddCallback("TerrainContext - Terrain Shadows", [](IndexT time)
		{
		});


	// create vertex buffer
	terrainState.components =
	{
		VertexComponent{ VertexComponent::Position, 0, VertexComponent::Format::Float3 },
		VertexComponent{ VertexComponent::TexCoord1, 0, VertexComponent::Format::Float2 },
	};

	terrainState.terrainShader = ShaderGet("shd:terrain.fxb");
	terrainState.terrainProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("Terrain"));
	terrainState.resourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
	terrainState.constants = ShaderCreateConstantBuffer(terrainState.terrainShader, "TerrainUniforms");
	IndexT constantsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TerrainUniforms");

	ResourceTableSetConstantBuffer(terrainState.resourceTable, { terrainState.constants, constantsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0});
	ResourceTableCommitChanges(terrainState.resourceTable);

	// create vlo
	terrainState.vlo = CreateVertexLayout({ terrainState.components });
	terrainState.mips = 3;
	terrainState.layers = 1;

	// create virtual texture for albedo
	TextureCreateInfo info;
	info.name = "TerrainAlbedo"_atm;
	info.tag = "render"_atm;
	info.width = 1024;
	info.height = 1024;
	info.sparse = true;
	info.mips = terrainState.mips;
	info.layers = terrainState.layers;
		
#ifdef USE_SPARSE
	terrainState.virtualAlbedoMap = CreateTexture(info);

	// layers
	terrainState.pageReferenceCount.Resize(terrainState.layers);
	for (int i = 0; i < terrainState.layers; i++)
	{
		terrainState.pageReferenceCount[i].Resize(terrainState.mips);

		for (int j = 0; j < terrainState.mips; j++)
		{
			SizeT numPages = CoreGraphics::TextureSparseGetNumPages(terrainState.virtualAlbedoMap, i, j);
			terrainState.pageReferenceCount[i][j].Resize(numPages);
			terrainState.pageReferenceCount[i][j].Fill(0);
		}
	}
	
#endif
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
	const Resources::ResourceName& normalMap, 
	const Resources::ResourceName& decisionMap,
	const TerrainSetupSettings& settings)
{
	n_assert(settings.worldSizeX > 0);
	n_assert(settings.worldSizeZ > 0);
	using namespace CoreGraphics;
	const Graphics::ContextEntityId cid = GetContextId(entity);
	TerrainLoadInfo& loadInfo = terrainAllocator.Get<Terrain_LoadInfo>(cid.id);
	TerrainRuntimeInfo& runtimeInfo = terrainAllocator.Get<Terrain_RuntimeInfo>(cid.id);

	runtimeInfo.heightMap = Resources::CreateResource(heightMap, "terrain"_atm, nullptr, nullptr, true);
	runtimeInfo.normalMap = Resources::CreateResource(normalMap, "terrain"_atm, nullptr, nullptr, true);
	runtimeInfo.decisionMap = Resources::CreateResource(decisionMap, "terrain"_atm, nullptr, nullptr, true);
	Resources::SetMaxLOD(runtimeInfo.heightMap, 0.0f, false);
	Resources::SetMaxLOD(runtimeInfo.normalMap, 0.0f, false);
	Resources::SetMaxLOD(runtimeInfo.decisionMap, 0.0f, false);

	TextureDimensions tdims = TextureGetDimensions(runtimeInfo.heightMap);
	runtimeInfo.heightMapWidth = tdims.width;
	runtimeInfo.heightMapHeight = tdims.height;
	runtimeInfo.worldWidth = settings.worldSizeX;
	runtimeInfo.worldHeight = settings.worldSizeZ;
	runtimeInfo.maxHeight = settings.maxHeight;
	runtimeInfo.minHeight = settings.minHeight;

	// divide world dimensions into 
	SizeT numXTiles = settings.worldSizeX / settings.tileWidth;
	SizeT numYTiles = settings.worldSizeZ / settings.tileHeight;
	SizeT height = settings.maxHeight - settings.minHeight;
	SizeT numVertsX = settings.tileWidth / settings.vertexDensityX + 1;
	SizeT numVertsY = settings.tileHeight / settings.vertexDensityY + 1;
	SizeT vertDistanceX = settings.vertexDensityX;
	SizeT vertDistanceY = settings.vertexDensityY;

	CoreGraphics::ConstantBufferCreateInfo cboInfo =
	{
		"TerrainPatchConstants",
		numXTiles * numYTiles * sizeof(PatchUniforms),
		CoreGraphics::HostWriteable
	};
	runtimeInfo.patchConstants = CreateConstantBuffer(cboInfo);
	runtimeInfo.patchTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_DYNAMIC_OFFSET_GROUP);
	IndexT slot = ShaderGetResourceSlot(terrainState.terrainShader, "PatchUniforms");
	ResourceTableSetConstantBuffer(runtimeInfo.patchTable, { runtimeInfo.patchConstants, slot, 0, true, false, NEBULA_WHOLE_BUFFER_SIZE, 0 });
	ResourceTableCommitChanges(runtimeInfo.patchTable);

	// allocate a tile vertex buffer
	TerrainVert* verts = (TerrainVert*)Memory::Alloc(Memory::ResourceHeap, numVertsX * numVertsY * sizeof(TerrainVert));

	// allocate terrain index buffer, every fourth pixel will generate two triangles 
	SizeT numTris = numVertsX * numVertsY * 2;
	TerrainTri* tris = (TerrainTri*)Memory::Alloc(Memory::ResourceHeap, numTris * sizeof(TerrainTri));

	// setup sections
	for (IndexT y = 0; y < numYTiles; y++)
	{
		for (IndexT x = 0; x < numXTiles; x++)
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
			runtimeInfo.sectorLod.Append((float)terrainState.mips);
			runtimeInfo.sectorLodResidency.Append(Util::FixedArray<bool>(terrainState.mips, false));
			
			uint uniformOffset = (x + y * numXTiles) * sizeof(PatchUniforms);
			PatchUniforms uniforms;
			uniforms.offsetPatchPos[0] = box.pmin.x;
			uniforms.offsetPatchPos[1] = box.pmin.z;
			uniforms.offsetPatchUV[0] = x * settings.tileWidth / settings.worldSizeX;
			uniforms.offsetPatchUV[1] = y * settings.tileHeight / settings.worldSizeZ;
			ConstantBufferUpdate(runtimeInfo.patchConstants, uniforms, uniformOffset);
			runtimeInfo.sectorUniformOffsets.Append(uniformOffset);

			CoreGraphics::PrimitiveGroup group;
			group.SetBaseIndex(0);
			group.SetBaseVertex(0);
			group.SetBoundingBox(box);
			group.SetNumIndices(numTris * 3);
			group.SetNumVertices(0);
			runtimeInfo.sectorPrimGroups.Append(group);
		}
	}
	ConstantBufferFlush(runtimeInfo.patchConstants);

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
			v1.uv = Math::vec2(x * vertDistanceX / float(settings.worldSizeX), y * vertDistanceY / float(settings.worldSizeZ));

			v2.position.set((x + 1) * vertDistanceX, 0, y * vertDistanceY, 1);
			v2.uv = Math::vec2((x + 1) * vertDistanceX / float(settings.worldSizeX), y * vertDistanceY / float(settings.worldSizeZ));

			v3.position.set(x * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
			v3.uv = Math::vec2(x * vertDistanceX / float(settings.worldSizeX), (y + 1) * vertDistanceY / float(settings.worldSizeZ));

			v4.position.set((x + 1) * vertDistanceX, 0, (y + 1) * vertDistanceY, 1);
			v4.uv = Math::vec2((x + 1) * vertDistanceX / float(settings.worldSizeX), (y + 1) * vertDistanceY / float(settings.worldSizeX));

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

			TerrainVert& vt2 = verts[vidx2];
			v2.position.storeu3(vt2.position.v);
			vt2.uv.x = v2.uv.x;
			vt2.uv.y = v2.uv.y;

			TerrainVert& vt3 = verts[vidx3];
			v3.position.storeu3(vt3.position.v);
			vt3.uv.x = v3.uv.x;
			vt3.uv.y = v3.uv.y;

			TerrainVert& vt4 = verts[vidx4];
			v4.position.storeu3(vt4.position.v);
			vt4.uv.x = v4.uv.x;
			vt4.uv.y = v4.uv.y;

			// setup triangle tris
			TerrainTri& t1 = tris[vidx1 * 2];
			t1.a = vidx1;
			t1.b = vidx3;
			t1.c = vidx2;

			TerrainTri& t2 = tris[vidx1 * 2 + 1];
			t2.a = vidx2;
			t2.b = vidx3;
			t2.c = vidx4;

			if (x % 2 ^ y % 2)
			{
				t1.a = vidx1;
				t1.b = vidx3;
				t1.c = vidx4;

				t2.a = vidx1;
				t2.b = vidx4;
				t2.c = vidx2;
			}
		}
	}

	// create vbo
	VertexBufferCreateInfo vboInfo
	{ 
		"terrain_vbo", 
		GpuBufferTypes::AccessRead, 
		GpuBufferTypes::UsageImmutable, 
		HostWriteable, 
		numVertsX * numVertsY,
		terrainState.components,
		verts, 
		numVertsX * numVertsY * sizeof(TerrainVert)
	};
	runtimeInfo.vbo = CreateVertexBuffer(vboInfo);

	// create ibo
	IndexBufferCreateInfo iboInfo
	{
		"terrain_ibo",
		"system"_atm,
		GpuBufferTypes::AccessRead,
		GpuBufferTypes::UsageImmutable,
		HostWriteable,
		CoreGraphics::IndexType::Index32,
		numTris * 3,
		tris,
		numTris * sizeof(TerrainTri)
	};
	runtimeInfo.ibo = CreateIndexBuffer(iboInfo);

	Memory::Free(Memory::ResourceHeap, verts);
	Memory::Free(Memory::ResourceHeap, tris);
}

//------------------------------------------------------------------------------
/**
*/
void
UpdateSparseTexture(const CoreGraphics::TextureId tex, Math::rectangle<uint> section, float oldMip, float newMip, CoreGraphics::TextureSparsePageSize pageSize)
{
	uint oldMipFloored = Math::n_floor(oldMip);
	uint newMipFloored = Math::n_floor(newMip);

	// if lods don't match, evaluate if we should update the sparse texture
	if (oldMipFloored != newMipFloored)
	{
		// if the mip we are loading is bigger than the one we are using, evict the previous mip
		if (oldMipFloored != terrainState.mips && newMipFloored > oldMipFloored)
		{
			// calculate page ranges offset by mip
			uint offsetX = section.left >> oldMipFloored;
			uint rangeX = Math::n_min(section.width() >> oldMipFloored, pageSize.width);
			uint endX = offsetX + (section.width() >> oldMipFloored);

			uint offsetY = section.top >> oldMipFloored;
			uint rangeY = Math::n_min(section.height() >> oldMipFloored, pageSize.height);
			uint endY = offsetY + (section.height() >> oldMipFloored);

			// go through pages and evict
			for (uint y = offsetY; y < endY; y += rangeY)
			{
				for (uint x = offsetX; x < endX; x += rangeX)
				{
					uint pageIndex = CoreGraphics::TextureSparseGetPageIndex(tex, 0, oldMipFloored, x, y, 0);

					// decrease the reference count
					if (terrainState.pageReferenceCount[0][oldMipFloored][pageIndex] > 0)
					{
						terrainState.pageReferenceCount[0][oldMipFloored][pageIndex]--;

						// if reference count for this page is 0
						if (terrainState.pageReferenceCount[0][oldMipFloored][pageIndex] == 0)
							CoreGraphics::TextureSparseEvict(tex, 0, oldMipFloored, pageIndex);
					}
				}
			}
		}

		// if we are requesting a higher mip, or we have no mips, make mip resident and update
		if (newMipFloored < oldMipFloored || oldMipFloored == terrainState.mips)
		{
			CoreGraphics::TextureId fillTex;
			switch (newMipFloored % 3)
			{
			case 0:
				fillTex = CoreGraphics::Green2D;
				break;
			case 1:
				fillTex = CoreGraphics::Blue2D;
				break;
			case 2:
				fillTex = CoreGraphics::Red2D;
				break;
			}

			// calculate page ranges offset by mip
			uint offsetX = section.left >> newMipFloored;
			uint rangeX = Math::n_min(section.width() >> newMipFloored, pageSize.width);
			uint endX = offsetX + (section.width() >> newMipFloored);

			uint offsetY = section.top >> newMipFloored;
			uint rangeY = Math::n_min(section.height() >> newMipFloored, pageSize.height);
			uint endY = offsetY + (section.height() >> newMipFloored);

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
						fillTex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1 },
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::ImageLayout::TransferSource,
						CoreGraphics::BarrierAccess::ShaderRead,
						CoreGraphics::BarrierAccess::TransferRead,
					},
					CoreGraphics::TextureBarrier
					{
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, newMipFloored, 1, 0, 1 },
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
					uint pageIndex = CoreGraphics::TextureSparseGetPageIndex(tex, 0, newMipFloored, x, y, 0);

					// if this will be our first reference, make the page resident
					if (terrainState.pageReferenceCount[0][newMipFloored][pageIndex] == 0)
					{
						CoreGraphics::TextureSparseMakeResident(tex, 0, newMipFloored, pageIndex);
						const CoreGraphics::TextureSparsePage& page = CoreGraphics::TextureSparseGetPage(tex, 0, newMipFloored, pageIndex);

						// if we are allocating the page, fill it immediately because it may contain old data
						section.left = page.offset.x;
						section.right = page.offset.x + page.extent.width;
						section.top = page.offset.y;
						section.bottom = page.offset.y + page.extent.height;
					}

					// add a reference count
					terrainState.pageReferenceCount[0][newMipFloored][pageIndex]++;

					// update the texture
					CoreGraphics::TextureSparseUpdate(tex, section, newMipFloored, fillTex, sub);
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
						fillTex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, 0, 1, 0, 1 },
						CoreGraphics::ImageLayout::TransferSource,
						CoreGraphics::ImageLayout::ShaderRead,
						CoreGraphics::BarrierAccess::TransferRead,
						CoreGraphics::BarrierAccess::ShaderRead,
					},
					CoreGraphics::TextureBarrier
					{
						tex,
						CoreGraphics::ImageSubresourceInfo { CoreGraphics::ImageAspect::ColorBits, newMipFloored, 1, 0, 1 },
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
		}
	}
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
	CoreGraphics::TextureSparsePageSize pageSize = CoreGraphics::TextureSparseGetPageSize(terrainState.virtualAlbedoMap);
	for (IndexT i = 0; i < runtimes.Size(); i++)
	{
		TerrainRuntimeInfo& rt = runtimes[i];
		for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
		{
			Math::ClipStatus::Type flag = rt.sectionBoxes[j].clipstatus(viewProj);
			rt.sectorVisible[j] = !(flag == Math::ClipStatus::Outside);
#ifdef USE_SPARSE
			float& currentLod = rt.sectorLod[j];

			Math::rectangle<uint> region;
			region.left = rt.sectionBoxes[j].pmin.x + rt.worldWidth / 2;
			region.top = rt.sectionBoxes[j].pmin.z + rt.worldHeight / 2;
			region.right = rt.sectionBoxes[j].pmax.x + rt.worldWidth / 2;
			region.bottom = rt.sectionBoxes[j].pmax.z + rt.worldHeight / 2;

			Math::vec4 nearestPoint = Math::clamp(cameraTransform.position, rt.sectionBoxes[j].pmin, rt.sectionBoxes[j].pmax);
			Math::vec4 eyeToBox = cameraTransform.position - nearestPoint;

			// calculate, assume that at distance 250 we use the lowest lod
			float dist = Math::n_saturate(length(eyeToBox) / 500.0f);
			float newLod = dist * (terrainState.mips - 1);

			// get page size and update sparse texture
			CoreGraphics::TextureSparsePageSize pageSize = CoreGraphics::TextureSparseGetPageSize(terrainState.virtualAlbedoMap);
			UpdateSparseTexture(terrainState.virtualAlbedoMap, region, currentLod, newLod, pageSize);

			currentLod = newLod;

			// if we can figure out a way to evict memory for sections that are not in the LOS
			// and also somehow make them load the right lods when we eventually turn back to look at them
			// that would be great, but for now, it causes major issues, so lets not do it.
			/*
			if (!rt.sectorVisible[j])
			{
				if (currentLod != terrainState.mips)
				{
					for (int i = 0; i < terrainState.mips; i++)
					{
						if (rt.sectorLodResidency[j][i])
						{
							CoreGraphics::SparseTextureEvict(terrainState.virtualAlbedoMap, currentLod, region);
							rt.sectorLodResidency[j][i] = false;
						}
					}
				}

				// reset lod
				currentLod = (float)terrainState.mips;
			}
			else
			{
				// calculate nearest point in box for camera
				Math::vec4 nearestPoint = Math::clamp(cameraTransform.position, rt.sectionBoxes[j].pmin, rt.sectionBoxes[j].pmax);
				Math::vec4 eyeToBox = cameraTransform.position - nearestPoint;

				// calculate, assume that at distance 250 we use the lowest lod
				float dist = Math::n_saturate(length(eyeToBox) / 500.0f);
				float newLod = dist * (terrainState.mips - 1);

				uint32_t currentLodFloored = Math::n_floor(currentLod);
				uint32_t newLodFloored = Math::n_floor(newLod);

				// if lods don't match, evict previous lod
				if (currentLodFloored != newLodFloored)
				{
					// if the mip we are loading is bigger than the one we are using, evict the previous mip
					if (currentLod != terrainState.mips && newLodFloored > currentLodFloored)
					{
						CoreGraphics::SparseTextureEvict(terrainState.virtualAlbedoMap, currentLodFloored, region);
					}

					CoreGraphics::TextureId tex;
					switch (newLodFloored % 3)
					{
					case 0:
						tex = CoreGraphics::Green2D;
						break;
					case 1:
						tex = CoreGraphics::Blue2D;
						break;
					case 2:
						tex = CoreGraphics::Red2D;
						break;
					}

					// if we are requesting a higher mip, or we have no mips, make mip resident and update
					if (newLodFloored < currentLodFloored || currentLod == terrainState.mips)
					{
						CoreGraphics::SparseTextureMakeResident(terrainState.virtualAlbedoMap, region, newLodFloored, tex, 0);
						rt.sectorLodResidency[j][i] = true;
					}
				}
				currentLod = newLod;
			}
			*/
#endif
		}
	}

#ifdef USE_SPARSE
	CoreGraphics::TextureSparseCommitChanges(terrainState.virtualAlbedoMap);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::UpdateVirtualTexture(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
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
