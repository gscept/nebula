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
#include "coregraphics/sparsetexture.h"

#include "resources/resourceserver.h"

#include "terrain.h"

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

	CoreGraphics::SparseTextureId virtualHeightMap;

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
				uniforms.MinLODDistance = 1.0f;
				uniforms.MaxLODDistance = 100.0f;
				uniforms.MinTessellation = 1.0f;
				uniforms.MaxTessellation = 8.0f;
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

	SparseTexureCreateInfo info =
	{
		32768, 32768, 1,
		Texture2D, PixelFormat::R8G8B8A8,
		8, 1, 1,
		true
	};
	terrainState.virtualHeightMap = CreateSparseTexture(info);
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
	runtimeInfo.maxHeight = settings.maxHeight;
	runtimeInfo.minHeight = settings.minHeight;

	const int TileXSize = 32;
	const int TileYSize = 32;
	const float scaleX = settings.worldSizeX / tdims.width;
	const float scaleY = settings.worldSizeZ / tdims.height;
	const int TileXScaled = TileXSize * scaleX;
	const int TileYScaled = TileYSize * scaleY;

	// divide world dimensions into 
	SizeT numXTiles = tdims.width / TileXSize;
	SizeT numYTiles = tdims.height / TileYSize;
	SizeT height = settings.maxHeight - settings.minHeight;

	// allocate terrain vertex buffer
	TerrainVert* verts = (TerrainVert*)Memory::Alloc(Memory::ResourceHeap, tdims.width * tdims.height * sizeof(TerrainVert));

	// allocate terrain index buffer, every fourth pixel will generate two triangles 
	SizeT numTris = tdims.width * tdims.height * 2;
	TerrainTri* tris = (TerrainTri*)Memory::Alloc(Memory::ResourceHeap, numTris * sizeof(TerrainTri));

	// walk through and set up sections, oriented around origo, so half of the sections are in the negative
	for (IndexT y = 0; y < numYTiles; y++)
	{
		for (IndexT x = 0; x < numXTiles; x++)
		{
			Math::bbox box;
			box.set(
				Math::point((x - numXTiles / 2) * TileXScaled + TileXScaled / 2, settings.minHeight, (y - numYTiles / 2) * TileYScaled + TileYScaled / 2),
				Math::vector(TileXScaled / 2, height, TileYScaled / 2));
			runtimeInfo.sectionBoxes.Append(box);
			runtimeInfo.sectorVisible.Append(true);

			IndexT groupIndex = x + y * numYTiles;
			CoreGraphics::PrimitiveGroup group;
			group.SetBaseIndex(groupIndex * TileXSize * TileYSize * 2 * 3); // every tile contains 32 * 32 * 2 triangles, and each has 3 indices
			group.SetBaseVertex(0);
			group.SetBoundingBox(box);
			group.SetNumIndices(TileXSize * TileYSize * 2 * 3);
			group.SetNumVertices(0);
			runtimeInfo.sectorPrimGroups.Append(group);

			// triangulate tile
			for (IndexT i = 0; i < TileYSize; i++)
			{
				for (IndexT j = 0; j < TileXSize; j++)
				{
					IndexT xPos = x * TileXSize + j;
					IndexT yPos = y * TileYSize + i;
					IndexT myIdx = xPos + yPos * tdims.height;

					// if we are on the edge, generate no data
					if (xPos == tdims.width - 1)
						continue;
					if (yPos == tdims.height - 1)
						continue;
					
					struct Vertex
					{
						Math::vec4 position;
						Math::vec2 uv;
					};
					Vertex v1, v2, v3, v4;

					// set terrain vertices
					v1.position.set(xPos - tdims.width / 2.0f, 0, yPos - tdims.height / 2.0f, 1);
					v1.position *= Math::vector(scaleX, 1, scaleY);
					v1.uv = Math::vec2(xPos / float(tdims.width), yPos / float(tdims.height));

					v2.position.set(xPos + 1 - tdims.width / 2.0f, 0, yPos - tdims.height / 2.0f, 1);
					v2.position *= Math::vector(scaleX, 1, scaleY);
					v2.uv = Math::vec2((xPos + 1) / float(tdims.width), yPos / float(tdims.height));

					v3.position.set(xPos - tdims.width / 2.0f, 0, yPos + 1 - tdims.height / 2.0f, 1);
					v3.position *= Math::vector(scaleX, 1, scaleY);
					v3.uv = Math::vec2(xPos / float(tdims.width), (yPos + 1) / float(tdims.height));

					v4.position.set(xPos + 1 - tdims.width / 2.0f, 0, yPos + 1 - tdims.height / 2.0f, 1);
					v4.position *= Math::vector(scaleX, 1, scaleY);
					v4.uv = Math::vec2((xPos + 1) / float(tdims.width), (yPos + 1) / float(tdims.height));

					// calculate tile local index, and offsets
					IndexT locX = j;
					IndexT locY = i;
					IndexT nextX = j + 1;
					IndexT nextY = i + 1;
					SizeT tileSize = TileXSize * TileYSize;
					SizeT tileRowSize = numXTiles * tileSize;
					IndexT myOffsetXTile = x * tileSize;
					IndexT myOffsetYTile = y * tileRowSize;
					IndexT myOffsetTile = myOffsetXTile + myOffsetYTile;
					IndexT rightOffsetTile = myOffsetTile;
					IndexT bottomOffsetTile = myOffsetTile;
					IndexT bottomRightOffsetTile = myOffsetTile;

					// if we are crossing over to another tile, just add some offset and reset the local index
					if (nextX == TileXSize)
					{
						rightOffsetTile += tileSize;
						bottomRightOffsetTile += tileSize;
						nextX = 0;
					}
					if (nextY == TileYSize)
					{
						bottomOffsetTile += tileRowSize;
						bottomRightOffsetTile += tileRowSize;
						nextY = 0;
					}

					// get buffer data so we can update it
					IndexT vidx1, vidx2, vidx3, vidx4;
					vidx1 = locX + locY * TileYSize + myOffsetTile;
					vidx2 = nextX + locY * TileYSize + rightOffsetTile;
					vidx3 = locX + nextY * TileYSize + bottomOffsetTile;
					vidx4 = nextX + nextY * TileYSize + bottomRightOffsetTile;

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

					if (j % 2 ^ i % 2)
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
		}
	}

	// create vbo
	VertexBufferCreateInfo vboInfo
	{ 
		"terrain_vbo", 
		GpuBufferTypes::AccessRead, 
		GpuBufferTypes::UsageImmutable, 
		HostWriteable, 
		tdims.width * tdims.height,
		terrainState.components,
		verts, 
		tdims.width * tdims.height * sizeof(TerrainVert)
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
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::CullPatches(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	N_SCOPE(CullPatches, Terrain);
	const Math::mat4& viewProj = Graphics::CameraContext::GetViewProjection(view->GetCamera());
	Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
	for (IndexT i = 0; i < runtimes.Size(); i++)
	{
		TerrainRuntimeInfo& rt = runtimes[i];
		for (IndexT j = 0; j < rt.sectionBoxes.Size(); j++)
		{
			Math::ClipStatus::Type flag = rt.sectionBoxes[j].clipstatus(viewProj);
			rt.sectorVisible[j] = !(flag == Math::ClipStatus::Outside);
		}
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
