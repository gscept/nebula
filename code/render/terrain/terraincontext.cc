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

} terrainState;

struct TerrainVert
{
	Math::float3 position;
	Math::float3 normal;
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
			SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			SetGraphicsPipeline();

			// set resources
			SetResourceTable(terrainState.resourceTable, NEBULA_BATCH_GROUP, GraphicsPipeline, nullptr);

			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GREEN, "Terrain Sections");

			// go through and render terrain instances
			Util::Array<TerrainRuntimeInfo>& runtimes = terrainAllocator.GetArray<Terrain_RuntimeInfo>();
			for (IndexT i = 0; i < runtimes.Size(); i++)
			{
				TerrainRuntimeInfo& rt = runtimes[i];
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
		VertexComponent{ VertexComponent::Normal, 0, VertexComponent::Format::Float3 },
		VertexComponent{ VertexComponent::TexCoord1, 0, VertexComponent::Format::Float2 },
	};

	terrainState.terrainShader = ShaderGet("shd:terrain.fxb");
	terrainState.terrainProgram = ShaderGetProgram(terrainState.terrainShader, ShaderFeatureFromString("Terrain"));
	terrainState.resourceTable = ShaderCreateResourceTable(terrainState.terrainShader, NEBULA_BATCH_GROUP);
	terrainState.constants = ShaderCreateConstantBuffer(terrainState.terrainShader, "TerrainUniforms");
	IndexT constantsSlot = ShaderGetResourceSlot(terrainState.terrainShader, "TerrainUniforms");

	Terrain::TerrainUniforms uniforms;
	Math::mat4().store(uniforms.Transform);
	ConstantBufferUpdate(terrainState.constants, uniforms, 0);

	ResourceTableSetConstantBuffer(terrainState.resourceTable, { terrainState.constants, constantsSlot, 0, false, false, NEBULA_WHOLE_BUFFER_SIZE, 0});
	ResourceTableCommitChanges(terrainState.resourceTable);

	// create vlo
	terrainState.vlo = CreateVertexLayout({ terrainState.components });
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
template<typename T> void
Read(const byte* pixels, const IndexT idx, T* out)
{
	memcpy(out, &pixels[idx], sizeof(T));
}

//------------------------------------------------------------------------------
/**
*/
template<typename T> void 
Triangulate(const byte* pixels, const SizeT stride, TerrainVert* verts, TerrainTri* tris, IndexT x, IndexT y, SizeT tileWidth, SizeT tileHeight, SizeT numTilesWidth, SizeT width, SizeT height, float norm)
{
	// if we are on the edge, generate no data
	if (x == width - 1)
		return;
	if (y == height - 1)
		return;

	// get pixel values
	IndexT myIdx = x + y * height;
	IndexT rightIdx = x + 1 + y * height;
	IndexT bottomIdx = x + (y + 1) * height;
	IndexT bottomRightIdx = x + 1 + (y + 1) * height;
	T p1, p2, p3, p4;
	Read(pixels, myIdx * stride, &p1);
	Read(pixels, rightIdx * stride, &p2);
	Read(pixels, bottomIdx * stride, &p3);
	Read(pixels, bottomRightIdx * stride, &p4);

	struct Vertex
	{
		Math::vec4 position;
		Math::vec2 uv;
		Math::vec4 normal;
	};
	Vertex v1, v2, v3, v4;

	// set terrain vertices
	v1.position.set(x - width / 2, p1 * norm, y - height / 2, 1);
	v1.uv = Math::vec2(x / float(width), y / float(height));

	v2.position.set(x + 1 - width / 2, p2 * norm, y - height / 2, 1);
	v2.uv = Math::vec2((x + 1) / float(width), y / float(height));
	
	v3.position.set(x - width / 2, p3 * norm, y + 1 - height / 2, 1);
	v3.uv = Math::vec2(x / float(width), (y + 1) / float(height));
	
	v4.position.set(x + 1 - width / 2, p4 * norm, y + 1 - height / 2, 1);
	v4.uv = Math::vec2((x + 1) / float(width), (y + 1) / float(height));

	// calculate normals
	Math::vec4 tangent1 = v2.position - v1.position;
	Math::vec4 tangent2 = v3.position - v1.position;
	v1.normal = Math::normalize(Math::cross3(tangent2, tangent1));

	Math::vec4 tangent3 = v4.position - v2.position;
	Math::vec4 tangent4 = v3.position - v2.position;
	v4.normal = Math::normalize(Math::cross3(tangent4, tangent3));

	v2.normal = Math::normalize(v1.normal + v4.normal);
	v3.normal = Math::normalize(v1.normal + v4.normal);

	// calculate tile local index, and offsets
	IndexT locX = x % tileWidth;
	IndexT locY = y % tileHeight;
	SizeT tileSize = tileWidth * tileHeight;
	SizeT tileRowSize = numTilesWidth * tileSize;
	IndexT myOffsetXTile = (x / tileWidth) * tileSize;
	IndexT myOffsetYTile = (y / tileHeight) * tileRowSize;
	IndexT myOffsetTile = myOffsetXTile + myOffsetYTile;
	IndexT rightOffsetTile = myOffsetTile;
	IndexT bottomOffsetTile = myOffsetTile;
	IndexT bottomRightOffsetTile = myOffsetTile;

	// if we are crossing over to another tile, just add some offset and reset the local index
	if (locX + 1 == tileWidth)
	{
		rightOffsetTile += tileSize;
		bottomRightOffsetTile += tileSize;
		locX = 0;
	}
	if (locY + 1 == tileHeight)
	{
		bottomOffsetTile += tileRowSize;
		bottomRightOffsetTile += tileRowSize;
		locY = 0;
	}

	// get buffer data so we can update it
	IndexT vidx1, vidx2, vidx3, vidx4;
	vidx1 = locX + locY * tileHeight + myOffsetTile;
	vidx2 = locX + 1 + locY * tileHeight + rightOffsetTile;
	vidx3 = locX + (locY + 1) * tileHeight + bottomOffsetTile;
	vidx4 = locX + 1 + (locY + 1) * tileHeight + bottomRightOffsetTile;

	TerrainVert& vt1 = verts[vidx1];
	v1.position.storeu3(vt1.position.v);
	vt1.uv.x = v1.uv.x;
	vt1.uv.y = v1.uv.y;
	v1.normal.storeu3(vt1.normal.v);

	TerrainVert& vt2 = verts[vidx2];
	v2.position.storeu3(vt2.position.v);
	vt2.uv.x = v2.uv.x;
	vt2.uv.y = v2.uv.y;
	v2.normal.storeu3(vt2.normal.v);

	TerrainVert& vt3 = verts[vidx3];
	v3.position.storeu3(vt3.position.v);
	vt3.uv.x = v3.uv.x;
	vt3.uv.y = v3.uv.y;
	v3.normal.storeu3(vt3.normal.v);

	TerrainVert& vt4 = verts[vidx4];
	v4.position.storeu3(vt4.position.v);
	vt4.uv.x = v4.uv.x;
	vt4.uv.y = v4.uv.y;
	v4.normal.storeu3(vt4.normal.v);

	// setup triangle tris
	TerrainTri& t1 = tris[vidx1 * 2];
	t1.a = vidx1;
	t1.b = vidx3;
	t1.c = vidx2;

	TerrainTri& t2 = tris[vidx1 * 2 + 1];
	t2.a = vidx2;
	t2.b = vidx3;
	t2.c = vidx4;
};

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::SetupTerrain(const Graphics::GraphicsEntityId entity, const Resources::ResourceName& texture, float minHeight, float maxHeight)
{
	using namespace CoreGraphics;
	const Graphics::ContextEntityId cid = GetContextId(entity);
	TerrainLoadInfo& loadInfo = terrainAllocator.Get<Terrain_LoadInfo>(cid.id);
	TerrainRuntimeInfo& runtimeInfo = terrainAllocator.Get<Terrain_RuntimeInfo>(cid.id);

	// create image from path, this allows us to sample from it
	ImageCreateInfoFile imgInf{ texture.AsString() };
	ImageId img = CreateImage(imgInf);

	// get image dimensions from image so we can divide 
	ImageDimensions dims = ImageGetDimensions(img);
	ImageChannelPrimitive prim = ImageGetChannelPrimitive(img);
	n_assert(dims.depth == 1); // would be wierd with a volume texture here...

	// get pointer to buffer and stride, let's just assume we're using a red-channel only texture
	const byte* buf = ImageGetBuffer(img);
	SizeT stride = ImageGetPixelStride(img);

	const int TileXSize = 128;
	const int TileYSize = 128;

	// divide image dimensions into tiles, let's say every tile is 32x32 meters
	SizeT numXTiles = dims.width / TileXSize;
	SizeT numYTiles = dims.height / TileYSize;
	SizeT height = maxHeight - minHeight;

	// allocate terrain vertex buffer
	TerrainVert* verts = (TerrainVert*)Memory::Alloc(Memory::ResourceHeap, dims.width * dims.height * sizeof(TerrainVert));

	// allocate terrain index buffer, every fourth pixel will generate two triangles 
	SizeT numTris = dims.width * dims.height * 2;
	TerrainTri* tris = (TerrainTri*)Memory::Alloc(Memory::ResourceHeap, numTris * sizeof(TerrainTri));

	// walk through and set up sections, oriented around origo, so half of the sections are in the negative
	for (IndexT y = 0; y < numYTiles; y++)
	{
		for (IndexT x = 0; x < numXTiles; x++)
		{
			Math::bbox box;
			box.set(
				Math::point((x - numXTiles / 2) * TileXSize + TileXSize / 2, minHeight, (y - numYTiles / 2) * TileYSize + TileYSize / 2), 
				Math::vector(TileXSize / 2, height, TileYSize / 2));
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
					IndexT myIdx = xPos + yPos * dims.height;
					
					switch (prim)
					{
					case Bit8Uint:
					{
						Triangulate<unsigned char>(buf, stride, verts, tris, xPos, yPos, TileXSize, TileYSize, numXTiles, dims.width, dims.height, height / 255.0f);
						break;
					}
					case Bit16Uint:
					{
						Triangulate<unsigned short>(buf, stride, verts, tris, xPos, yPos, TileXSize, TileYSize, numXTiles, dims.width, dims.height, height / 65535.0f);
						break;
					}
					case Bit16Float:
					{
						Triangulate<unsigned short>(buf, stride, verts, tris, xPos, yPos, TileXSize, TileYSize, numXTiles, dims.width, dims.height, height);
						break;
					}
					case Bit32Uint:
					{
						Triangulate<unsigned int>(buf, stride, verts, tris, xPos, yPos, TileXSize, TileYSize, numXTiles, dims.width, dims.height, height / UINT_MAX);
						break;
					}
					case Bit32Float:
					{
						Triangulate<float>(buf, stride, verts, tris, x, y, TileXSize, TileYSize, numXTiles, dims.width, dims.height, height);
						break;
					}
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
		GpuBufferTypes::SyncingManual, 
		dims.width * dims.height,  
		terrainState.components,
		verts, 
		dims.width * dims.height * sizeof(TerrainVert)
	};
	runtimeInfo.vbo = CreateVertexBuffer(vboInfo);

	// create ibo
	IndexBufferCreateInfo iboInfo
	{
		"terrain_ibo",
		"system"_atm,
		GpuBufferTypes::AccessRead,
		GpuBufferTypes::UsageImmutable,
		GpuBufferTypes::SyncingManual,
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
