//------------------------------------------------------------------------------
//  terraincontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "terraincontext.h"
#include "coregraphics/image.h"
#include "graphics/graphicsserver.h"
#include "frame/frameplugin.h"
namespace Terrain
{
TerrainContext::TerrainAllocator TerrainContext::terrainAllocator;
_ImplementContext(TerrainContext, TerrainContext::terrainAllocator);

struct
{
	Util::Array<CoreGraphics::VertexComponent> components;
	CoreGraphics::VertexLayoutId vlo;

} terrainState;
struct TerrainVert
{
	Math::vec4 position;
	Math::vec2 uv;
	Math::vec4 normal;
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

#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = TerrainContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	Frame::AddCallback("TerrainContext - Render Terrain", [](IndexT time)
		{
			SetVertexLayout(terrainState.vlo);
			SetPrimitiveTopology(PrimitiveTopology::TriangleList);

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
		});


	// create vertex buffer
	terrainState.components =
	{
		VertexComponent{ VertexComponent::Position, 0, VertexComponent::Format::Float4 },
		VertexComponent{ VertexComponent::TexCoord1, 0, VertexComponent::Format::Float2 },
		VertexComponent{ VertexComponent::Normal, 0, VertexComponent::Format::Float4 },
	};

	// create vlo
	terrainState.vlo = CreateVertexLayout({ terrainState.components });
}

//------------------------------------------------------------------------------
/**
*/
void 
TerrainContext::Destroy()
{
}

//------------------------------------------------------------------------------
/**
*/
template<typename T> void
Read(const byte* pixels, const IndexT idx, T* out)
{
	memcpy(out, pixels[idx], sizeof(T));
}

//------------------------------------------------------------------------------
/**
*/
template<typename T> void 
Triangulate(const byte* pixels, const SizeT stride, TerrainVert* verts, TerrainTri* tris, IndexT x, IndexT y, SizeT width, SizeT height)
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
	Read(pixels, myIdx * stride, p1);
	Read(pixels, rightIdx * stride, p2);
	Read(pixels, bottomIdx * stride, p3);
	Read(pixels, bottomRightIdx * stride, p4);

	// set terrain vertices
	TerrainVert& v1 = verts[myIdx];
	v1.position.set(x, p1, y, 1);
	v1.uv = Math::vec2(x / float(width), y / float(height));
	TerrainVert& v2 = verts[rightIdx];
	v2.position.set(x + 1, p2, y, 1);
	v2.uv = Math::vec2((x + 1) / float(width), y / float(height));
	TerrainVert& v3 = verts[bottomIdx];
	v3.position.set(x, p3, y + 1, 1);
	v3.uv = Math::vec2(x / float(width), (y + 1) / float(height));
	TerrainVert& v4 = verts[bottomRightIdx];
	v4.position.set(x + 1, p4, y + 1, 1);
	v4.uv = Math::vec2((x + 1) / float(width), (y + 1) / float(height));

	// calculate normals
	Math::vector tangent1 = v2 - v1;
	Math::vector tangent2 = v3 - v1;
	v1.normal = Math::cross3(tangent1, tangent2);

	Math::vector tangent3 = v4 - v2;
	Math::vector tangent4 = v3 - v2;
	v4.normal = Math::cross3(tangent3, tangent4);

	v2.normal = (v1.normal + v4.normal) * 0.5f;
	v3.normal = (v1.normal + v4.normal) * 0.5f;

	// setup triangle tris
	TerrainTri& t1 = tris[myIdx * 2];
	t1.a = myIdx;
	t1.b = rightIdx;
	t1.c = bottomIdx;

	TerrainTri& t2 = tris[myIdx * 2 + 1];
	t2.a = rightIdx;
	t2.b = bottomRightIdx;
	t2.c = bottomIdx;
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

	static const int TileXSize = 32;
	static const int TileYSize = 32;

	// divide image dimensions into tiles, let's say every tile is 32x32 meters
	SizeT numXTiles = dims.width / TileXSize;
	SizeT numYTiles = dims.height / TileYSize;
	SizeT height = maxHeight - minHeight;

	// walk through and set up sections, oriented around origo, so half of the sections are in the negative
	for (IndexT y = -numYTiles / 2; y < numYTiles / 2; y++)
	{
		for (IndexT x = -numXTiles / 2; x < numXTiles / 2; x++)
		{
			Math::bbox box;
			box.set(Math::point(x, minHeight, y), Math::vector(32, height, 32));
			runtimeInfo.sectionBoxes.Append(box);
			runtimeInfo.sectorVisible.Append(false);

			IndexT groupIndex = x + y * numYTiles;
			CoreGraphics::PrimitiveGroup group;
			group.SetBaseIndex(groupIndex * TileXSize * TileYSize * 2 * 3); // every tile contains 32 * 32 * 2 triangles, and each has 3 indices
			group.SetBaseVertex(0);
			group.SetBoundingBox(box);
			group.SetNumIndices(TileXSize * TileYSize * 2 * 3);
			group.SetNumVertices(0);
			runtimeInfo.sectorPrimGroups.Append(group);
		}
	}

	// allocate terrain vertex buffer
	TerrainVert* verts = (TerrainVert*)Memory::Alloc(Memory::ResourceHeap, dims.width * dims.height * sizeof(TerrainVert));

	// allocate terrain index buffer, every four pixels will generate two triangles
	SizeT numTris = (dims.width - 1) * (dims.height - 1) / 2;
	TerrainTri* tris = (TerrainTri*)Memory::Alloc(Memory::ResourceHeap, numTris * sizeof(TerrainTri));
	for (IndexT y = 0; y < dims.height; y++)
	{
		for (IndexT x = 0; x < dims.width; x++)
		{
			switch (prim)
			{
			case Bit8Uint:
			{
				Triangulate<unsigned char>(buf, stride, verts, tris, x, y, dims.width, dims.height);
				break;
			}
			case Bit16Uint:
			case Bit16Float:
			{
				Triangulate<unsigned short>(buf, stride, verts, tris, x, y, dims.width, dims.height);
				break;
			}
			case Bit32Uint:
			{
				Triangulate<unsigned int>(buf, stride, verts, tris, x, y, dims.width, dims.height);
				break;
			}
			case Bit32Float:
			{
				Triangulate<float>(buf, stride, verts, tris, x, y, dims.width, dims.height);
				break;
			}		

			}
		}
	}

	// create vertex buffer
	Util::Array<VertexComponent> comps =
	{
		VertexComponent{ VertexComponent::Position, 0, VertexComponent::Format::Float4 },
		VertexComponent{ VertexComponent::TexCoord1, 0, VertexComponent::Format::Float2 },
		VertexComponent{ VertexComponent::Normal, 0, VertexComponent::Format::Float4 },
	};

	// create vbo
	VertexBufferCreateInfo vboInfo
	{ 
		"terrain_vbo", 
		GpuBufferTypes::AccessRead, 
		GpuBufferTypes::UsageImmutable, 
		GpuBufferTypes::SyncingManual, 
		dims.width * dims.height,  
		comps,
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
			RenderShape shape(RenderShape::Box, RenderShape::Wireframe, rt.sectionBoxes[i].to_mat4(), Math::vec4(0, 1, 0, 1));
			shapeRenderer->AddShape(shape);
		}
	}
}

} // namespace Terrain
