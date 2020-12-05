//------------------------------------------------------------------------------
//  vegetationcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "graphics/graphicsserver.h"
#include "vegetationcontext.h"
#include "resources/resourceserver.h"
#include "math/plane.h"
#include "graphics/view.h"
#include "graphics/cameracontext.h"
#include "frame/frameplugin.h"
#include "memory/rangeallocator.h"

#include "vegetation.h"
namespace Vegetation
{

VegetationContext::VegetationAllocator VegetationContext::vegetationAllocator;
_ImplementContext(VegetationContext, VegetationContext::vegetationAllocator);


struct MeshBufferCopy
{
	CoreGraphics::BufferId fromBuf;
	CoreGraphics::BufferId toBuf;
	CoreGraphics::BufferCopy to, from;
	SizeT numBytes;
};

struct
{
	CoreGraphics::TextureId heightMap;
	float minHeight, maxHeight;
	Math::uint2 worldSize;
	uint numGrassBlades;

	CoreGraphics::BufferId grassVbo;
	CoreGraphics::BufferId grassIbo;
	CoreGraphics::VertexLayoutId grassLayout;

	CoreGraphics::BufferId combinedMeshVbo;
	CoreGraphics::BufferId combinedMeshIbo;
	CoreGraphics::VertexLayoutId combinedMeshLayout;
	Memory::RangeAllocator vboAllocator;
	Memory::RangeAllocator iboAllocator;

	Vegetation::MeshInfo meshInfos[Vegetation::MAX_MESH_INFOS];
	Ids::IdPool meshInfoPool;
	CoreGraphics::BufferId meshInfoBuffer;

	Util::Array<MeshBufferCopy> meshBufferCopiesThisFrame;

	CoreGraphics::ResourceTableId systemResourceTable;
	CoreGraphics::BufferId systemUniforms;

	CoreGraphics::ShaderId vegetationBaseShader;
	CoreGraphics::ShaderProgramId vegetationGenerateDrawsShader;
	CoreGraphics::ShaderProgramId vegetationRenderShader;

	CoreGraphics::BufferId indirectGrassDrawBuffer;
	CoreGraphics::BufferId indirectGrassUniformBuffer;
	CoreGraphics::BufferId indirectMeshDrawBuffer;
	CoreGraphics::BufferId indirectMeshUniformBuffer;
	static const uint MaxNumIndirectDraws = 8192;

	CoreGraphics::BufferId indirectCountBuffer;
	Util::FixedArray<CoreGraphics::BufferId> indirectCountReadbackBuffer;

	IndexT grassMaskCount = 0;
	IndexT meshMaskCount = 0;
	Ids::IdPool texturePool;

	SizeT grassDrawsThisFrame = 0;
	SizeT meshDrawsThisFrame = 0;

} vegetationState;

static const uint VegetationDistributionRadius = 64;

struct GrassVertex
{
	Math::float3 position;
	Math::float3 normal;
	Math::float2 uv;
};

/*
* 	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	[slot=3] in vec3 tangent,
	[slot=4] in vec3 binormal,
*/
struct CombinedMeshVertex
{
	Math::float3 position;
	Math::float3 normal;
	Math::float2 uv;
	Math::float3 tangent;
	Math::float3 binormal;
};

//------------------------------------------------------------------------------
/**
*/
VegetationContext::VegetationContext()
{
}

//------------------------------------------------------------------------------
/**
*/
VegetationContext::~VegetationContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Create(const VegetationSetupSettings& settings)
{
	_CreateContext();
	using namespace CoreGraphics;

	__bundle.OnUpdateViewResources = VegetationContext::UpdateViewResources;

	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	vegetationState.heightMap = Resources::CreateResource(settings.heightMap, "Vegetation", nullptr, nullptr, true);
	vegetationState.minHeight = settings.minHeight;
	vegetationState.maxHeight = settings.maxHeight;
	vegetationState.worldSize = settings.worldSize;
	vegetationState.numGrassBlades = settings.numGrassPlanes;

	Util::FixedArray<GrassVertex> vertices(settings.numGrassPlanes * 4);
	Util::FixedArray<ushort> indices(settings.numGrassPlanes * 6);

	// calculate center points for grass patches by angle
	float angle = 0.0f;
	float angleIncrement = 360.0f / settings.numGrassPlanes;
	int index = 0;
	int vertex = 0;
	for (SizeT i = 0; i < settings.numGrassPlanes; i++, index += 6, vertex += 4)
	{
		float x = Math::n_cos(Math::n_deg2rad(angle));
		float z = Math::n_sin(Math::n_deg2rad(angle));

		// calculate a plane at the point facing the center
		Math::plane plane(Math::point(0, 0, 0), Math::point(0, 0, 0) - Math::point(x, 0, z));

		// get the plane components
		Math::point center = get_point(plane);
		Math::vector normal = get_normal(plane);

		// create perpendicular 2d vector
		Math::vector perp = Math::vector(normal.z, 0, -normal.x);

		// extrude vertex positions using perp
		(center + -perp * settings.grassPatchRadius).storeu(vertices[vertex].position.v);
		vertices[vertex].position.y = 0.0f;
		(center + -perp * settings.grassPatchRadius).storeu(vertices[vertex + 1].position.v);
		vertices[vertex + 1].position.y = settings.grassPatchRadius;
		(center + perp * settings.grassPatchRadius).storeu(vertices[vertex + 2].position.v);
		vertices[vertex + 2].position.y = 0.0f;
		(center + perp * settings.grassPatchRadius).storeu(vertices[vertex + 3].position.v);
		vertices[vertex + 3].position.y = settings.grassPatchRadius;

		// store normals
		normal.storeu(vertices[vertex].normal.v);
		normal.storeu(vertices[vertex + 1].normal.v);
		normal.storeu(vertices[vertex + 2].normal.v);
		normal.storeu(vertices[vertex + 3].normal.v);

		// store uvs
		vertices[vertex].uv = Math::float2{ 0.0f, 0.0f };
		vertices[vertex + 1].uv = Math::float2{ 0.0f, 1.0f };
		vertices[vertex + 2].uv = Math::float2{ 1.0f, 0.0f };
		vertices[vertex + 3].uv = Math::float2{ 1.0f, 1.0f };

		// tri 0
		indices[index] = vertex;
		indices[index + 1] = vertex + 1;
		indices[index + 2] = vertex + 2;

		// tri 1
		indices[index + 3] = vertex + 1;
		indices[index + 4] = vertex + 3;
		indices[index + 5] = vertex + 2;

		angle += angleIncrement;
	}

	CoreGraphics::VertexLayoutCreateInfo vloInfo;
	vloInfo.comps =
	{
		// per vertex geometry data
		VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3),
		VertexComponent(VertexComponent::Normal, 0, VertexComponent::Float3),
		VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2),

		// per instance data
		/*
		VertexComponent((VertexComponent::SemanticName)3, 1, VertexComponent::Float3, 1, VertexComponent::PerInstance),	// position
		VertexComponent((VertexComponent::SemanticName)4, 1, VertexComponent::UInt, 1, VertexComponent::PerInstance),	// texture
		VertexComponent((VertexComponent::SemanticName)5, 1, VertexComponent::Float3, 1, VertexComponent::PerInstance),	// normal
		VertexComponent((VertexComponent::SemanticName)6, 1, VertexComponent::Float, 1, VertexComponent::PerInstance),	// random value
		VertexComponent((VertexComponent::SemanticName)7, 1, VertexComponent::Float2, 1, VertexComponent::PerInstance),	// sin cos
		*/
	};
	vegetationState.grassLayout = CoreGraphics::CreateVertexLayout(vloInfo);

	CoreGraphics::BufferCreateInfo vboInfo;
	vboInfo.name = "Grass VBO";
	vboInfo.usageFlags = CoreGraphics::VertexBuffer;
	vboInfo.elementSize = sizeof(GrassVertex);
	vboInfo.size = settings.numGrassPlanes * 4;
	vboInfo.mode = CoreGraphics::DeviceLocal;
	vboInfo.data = vertices.Begin();
	vboInfo.dataSize = vertices.ByteSize();
	vegetationState.grassVbo = CoreGraphics::CreateBuffer(vboInfo);

	CoreGraphics::BufferCreateInfo iboInfo;
	iboInfo.name = "Grass IBO";
	iboInfo.usageFlags = CoreGraphics::IndexBuffer;
	iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index16);
	iboInfo.size = settings.numGrassPlanes * 6;
	iboInfo.mode = CoreGraphics::DeviceLocal;
	iboInfo.data = indices.Begin();
	iboInfo.dataSize = indices.ByteSize();
	vegetationState.grassIbo = CoreGraphics::CreateBuffer(iboInfo);

	vloInfo.comps =
	{
		// per vertex geometry data
		VertexComponent(VertexComponent::Position, 0, VertexComponent::Float3),
		VertexComponent(VertexComponent::Normal, 0, VertexComponent::Float3),
		VertexComponent(VertexComponent::TexCoord1, 0, VertexComponent::Float2),
		VertexComponent(VertexComponent::Binormal, 0, VertexComponent::Float3),
		VertexComponent(VertexComponent::Tangent, 0, VertexComponent::Float3),

		// per instance data
		/*
		VertexComponent((VertexComponent::SemanticName)5, 1, VertexComponent::Float3, 1, VertexComponent::PerInstance),	// position
		VertexComponent((VertexComponent::SemanticName)6, 1, VertexComponent::UInt, 1, VertexComponent::PerInstance),	// texture
		VertexComponent((VertexComponent::SemanticName)7, 1, VertexComponent::Float3, 1, VertexComponent::PerInstance),	// normal
		VertexComponent((VertexComponent::SemanticName)8, 1, VertexComponent::Float, 1, VertexComponent::PerInstance),	// random value
		VertexComponent((VertexComponent::SemanticName)9, 1, VertexComponent::Float2, 1, VertexComponent::PerInstance),	// sin cos
		*/
	};
	vegetationState.combinedMeshLayout = CoreGraphics::CreateVertexLayout(vloInfo);

	vboInfo.name = "Combined Mesh VBO";
	vboInfo.elementSize = sizeof(CombinedMeshVertex);
	vboInfo.size = 65535;	// start with 64k vertices
	vboInfo.usageFlags = CoreGraphics::VertexBuffer | CoreGraphics::TransferBufferDestination;
	vegetationState.combinedMeshVbo = CoreGraphics::CreateBuffer(vboInfo);
	vegetationState.vboAllocator.Resize(BufferGetByteSize(vegetationState.combinedMeshVbo));

	iboInfo.name = "Combined Mesh IBO";
	iboInfo.elementSize = CoreGraphics::IndexType::SizeOf(CoreGraphics::IndexType::Index32);
	iboInfo.size = 65535;	// start with 64k indices
	iboInfo.usageFlags = CoreGraphics::IndexBuffer | CoreGraphics::TransferBufferDestination;
	vegetationState.combinedMeshIbo = CoreGraphics::CreateBuffer(iboInfo);
	vegetationState.iboAllocator.Resize(BufferGetByteSize(vegetationState.combinedMeshIbo));

	CoreGraphics::BufferCreateInfo meshInfo;
	meshInfo.elementSize = sizeof(Vegetation::MeshInfo);
	meshInfo.size = Vegetation::MAX_MESH_INFOS;
	meshInfo.mode = CoreGraphics::HostToDevice;
	meshInfo.usageFlags = CoreGraphics::ConstantBuffer | CoreGraphics::TransferBufferDestination;
	vegetationState.meshInfoBuffer = CoreGraphics::CreateBuffer(meshInfo);

	CoreGraphics::BufferCreateInfo cboInfo;
	cboInfo.name = "System Uniforms";
	cboInfo.usageFlags = CoreGraphics::ConstantBuffer | CoreGraphics::TransferBufferDestination;
	cboInfo.elementSize = sizeof(VegetationGenerateUniforms);
	cboInfo.size = 1;
	cboInfo.mode = CoreGraphics::HostToDevice;
	vegetationState.systemUniforms = CoreGraphics::CreateBuffer(cboInfo);

	CoreGraphics::BufferCreateInfo idBufInfo;
	idBufInfo.name = "Indirect Grass Draw Buffer";
	idBufInfo.usageFlags = CoreGraphics::IndirectBuffer | CoreGraphics::ReadWriteBuffer;
	idBufInfo.mode = CoreGraphics::DeviceLocal;
	idBufInfo.elementSize = sizeof(DrawIndexedCommand);
	idBufInfo.size = 1;			// will use instancing instead of multi-draw
	vegetationState.indirectGrassDrawBuffer = CoreGraphics::CreateBuffer(idBufInfo);

	// per-instance uniforms (fed through vertex)
	idBufInfo.name = "Indirect Grass Uniform Buffer";
	idBufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
	idBufInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
	idBufInfo.mode = CoreGraphics::DeviceLocal;
	idBufInfo.size = vegetationState.MaxNumIndirectDraws;
	vegetationState.indirectGrassUniformBuffer = CoreGraphics::CreateBuffer(idBufInfo);

	idBufInfo.name = "Indirect Mesh Draw Buffer";
	idBufInfo.usageFlags = CoreGraphics::IndirectBuffer | CoreGraphics::ReadWriteBuffer;
	idBufInfo.elementSize = sizeof(DrawIndexedCommand);
	idBufInfo.mode = CoreGraphics::DeviceLocal;
	idBufInfo.size = vegetationState.MaxNumIndirectDraws;
	vegetationState.indirectMeshDrawBuffer = CoreGraphics::CreateBuffer(idBufInfo);

	// per-instance uniforms (fed through vertex)
	idBufInfo.name = "Indirect Mesh Uniform Buffer";
	idBufInfo.usageFlags = CoreGraphics::ReadWriteBuffer;
	idBufInfo.elementSize = sizeof(Vegetation::InstanceUniforms);
	idBufInfo.mode = CoreGraphics::DeviceLocal;
	idBufInfo.size = vegetationState.MaxNumIndirectDraws;
	vegetationState.indirectMeshUniformBuffer = CoreGraphics::CreateBuffer(idBufInfo);

	idBufInfo.name = "Indirect Count Buffer";
	idBufInfo.usageFlags = CoreGraphics::ReadWriteBuffer | CoreGraphics::TransferBufferSource;
	idBufInfo.elementSize = sizeof(uint);
	idBufInfo.mode = CoreGraphics::DeviceLocal;
	idBufInfo.size = 2;			// will use instancing instead of multi-draw
	vegetationState.indirectCountBuffer = CoreGraphics::CreateBuffer(idBufInfo);

	idBufInfo.usageFlags = CoreGraphics::TransferBufferDestination;
	idBufInfo.mode = CoreGraphics::DeviceToHost;
	vegetationState.indirectCountReadbackBuffer.Resize(CoreGraphics::GetNumBufferedFrames());
	for (IndexT i = 0; i < vegetationState.indirectCountReadbackBuffer.Size(); i++)
	{
		vegetationState.indirectCountReadbackBuffer[i] = CoreGraphics::CreateBuffer(idBufInfo);
	}

	vegetationState.vegetationBaseShader = CoreGraphics::ShaderGet("shd:vegetation.fxb");
	vegetationState.vegetationGenerateDrawsShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationGenerateDraws"));
	vegetationState.vegetationRenderShader = CoreGraphics::ShaderGetProgram(vegetationState.vegetationBaseShader, ShaderFeatureFromString("VegetationGrassDraw"));
	vegetationState.systemResourceTable = ShaderCreateResourceTable(vegetationState.vegetationBaseShader, NEBULA_SYSTEM_GROUP);

	ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.systemUniforms,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "VegetationGenerateUniforms"),
			0,
			false, false,
			sizeof(Vegetation::VegetationGenerateUniforms),
			0
		});

	ResourceTableSetConstantBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.meshInfoBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "MeshInfoUniforms"),
			0,
			false, false,
			sizeof(Vegetation::MeshInfo) * 8,
			0
		});

	ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.indirectGrassDrawBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "IndirectGrassDrawBuffer"),
			0,
			false, false,
			BufferGetByteSize(vegetationState.indirectGrassDrawBuffer),
			0
		});

	ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.indirectGrassUniformBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "InstanceGrassUniformBuffer"),
			0,
			false, false,
			BufferGetByteSize(vegetationState.indirectGrassUniformBuffer),
			0
		});

	ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.indirectMeshDrawBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "IndirectMeshDrawBuffer"),
			0,
			false, false,
			BufferGetByteSize(vegetationState.indirectMeshDrawBuffer),
			0
		});

	ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.indirectMeshUniformBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "InstanceMeshUniformBuffer"),
			0,
			false, false,
			BufferGetByteSize(vegetationState.indirectMeshUniformBuffer),
			0
		});

	ResourceTableSetRWBuffer(vegetationState.systemResourceTable,
		{
			vegetationState.indirectCountBuffer,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "DrawCount"),
			0,
			false, false,
			BufferGetByteSize(vegetationState.indirectCountBuffer),
			0
		});

	ResourceTableCommitChanges(vegetationState.systemResourceTable);

	// add callback for updating the meshes
	Frame::AddCallback("VegetationContext - Update Meshes", [](const IndexT frame, const IndexT bufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_TRANSFER, "Vegetation Generation");

			BarrierPush(GraphicsQueueType,
				BarrierStage::VertexInput,
				BarrierStage::Transfer,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						vegetationState.combinedMeshVbo,
						BarrierAccess::VertexRead,
						BarrierAccess::TransferWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
					BufferBarrier
					{
						vegetationState.combinedMeshIbo,
						BarrierAccess::VertexRead,
						BarrierAccess::TransferWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				});

			// perform copies
			for (IndexT i = 0; i < vegetationState.meshBufferCopiesThisFrame.Size(); i++)
			{
				const MeshBufferCopy& meshCopy = vegetationState.meshBufferCopiesThisFrame[i];
				Copy(GraphicsQueueType, meshCopy.fromBuf, { meshCopy.from }, meshCopy.toBuf, { meshCopy.to }, meshCopy.numBytes);
			}
			vegetationState.meshBufferCopiesThisFrame.Clear();

			BarrierPop(GraphicsQueueType);

			CommandBufferEndMarker(GraphicsQueueType);
		});

	// add callback for vegetation generation pass
	Frame::AddCallback("VegetationContext - Generate Draws", [](const IndexT frame, const IndexT bufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_COMPUTE, "Vegetation Generation");

			BarrierPush(GraphicsQueueType,
				BarrierStage::Indirect,
				BarrierStage::ComputeShader,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						vegetationState.indirectGrassDrawBuffer,
						BarrierAccess::IndirectRead,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
					BufferBarrier
					{
						vegetationState.indirectMeshDrawBuffer,
						BarrierAccess::IndirectRead,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				});

			BarrierPush(GraphicsQueueType,
				BarrierStage::AllGraphicsShaders,
				BarrierStage::ComputeShader,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						vegetationState.indirectGrassUniformBuffer,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
					BufferBarrier
					{
						vegetationState.indirectMeshUniformBuffer,
						BarrierAccess::ShaderRead,
						BarrierAccess::ShaderWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				});

			SetShaderProgram(vegetationState.vegetationGenerateDrawsShader);
			SetResourceTable(vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, ComputePipeline, nullptr);

			Compute(VegetationDistributionRadius / 64, VegetationDistributionRadius, 1);

			// pop barriers 
			BarrierPop(GraphicsQueueType);
			BarrierPop(GraphicsQueueType);

			CommandBufferEndMarker(GraphicsQueueType);
		});

	// read back counts
	Frame::AddCallback("VegetationContext - Readback", [](const IndexT frame, const IndexT bufferIndex)
		{

			BarrierPush(GraphicsQueueType,
				BarrierStage::ComputeShader,
				BarrierStage::Transfer,
				BarrierDomain::Global,
				nullptr,
				{
					BufferBarrier
					{
						vegetationState.indirectCountBuffer,
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
						vegetationState.indirectCountReadbackBuffer[bufferIndex],
						BarrierAccess::HostRead,
						BarrierAccess::TransferWrite,
						0, NEBULA_WHOLE_BUFFER_SIZE
					},
				});

			BufferCopy from, to;
			from.offset = 0;
			to.offset = 0;
			Copy(GraphicsQueueType, vegetationState.indirectCountBuffer, { from }, vegetationState.indirectCountReadbackBuffer[bufferIndex], { to }, BufferGetByteSize(vegetationState.indirectCountBuffer));

			BarrierPop(GraphicsQueueType);
			BarrierPop(GraphicsQueueType);
		});

	// add callback for rendering
	Frame::AddCallback("VegetationContext - Render", [](const IndexT frame, const IndexT bufferIndex)
		{
			CommandBufferBeginMarker(GraphicsQueueType, NEBULA_MARKER_GRAPHICS, "Vegetation Grass");

			if (vegetationState.grassDrawsThisFrame > 0)
			{
				SetShaderProgram(vegetationState.vegetationRenderShader);

				// setup mesh
				SetVertexLayout(vegetationState.grassLayout);
				SetStreamVertexBuffer(0, vegetationState.grassVbo, 0);
				//SetStreamVertexBuffer(1, vegetationState.indirectGrassUniformBuffer, 0);
				SetIndexBuffer(vegetationState.grassIbo, 0);

				// set graphics pipeline
				SetGraphicsPipeline();

				// bind resource table
				SetResourceTable(vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

				// draw
				DrawIndirectIndexed(vegetationState.indirectGrassDrawBuffer, 0, 1, sizeof(Vegetation::DrawIndexedCommand));
			}

			if (vegetationState.meshDrawsThisFrame > 0)
			{
				SetShaderProgram(vegetationState.vegetationRenderShader);

				// setup mesh
				SetVertexLayout(vegetationState.combinedMeshLayout);
				SetStreamVertexBuffer(0, vegetationState.combinedMeshVbo, 0);
				//SetStreamVertexBuffer(1, vegetationState.indirectMeshUniformBuffer, 0);
				SetIndexBuffer(vegetationState.combinedMeshIbo, 0);

				// set graphics pipeline
				SetGraphicsPipeline();

				// bind resources
				SetResourceTable(vegetationState.systemResourceTable, NEBULA_SYSTEM_GROUP, GraphicsPipeline, nullptr);

				// draw
				DrawIndirectIndexed(vegetationState.indirectMeshDrawBuffer, 0, vegetationState.meshDrawsThisFrame, sizeof(Vegetation::DrawIndexedCommand));
			}

			CommandBufferEndMarker(GraphicsQueueType);
		});
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Discard()
{
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetupGrass(const Graphics::GraphicsEntityId id, const VegetationGrassSetup& setup)
{
	Graphics::ContextEntityId cid = GetContextId(id.id);
	CoreGraphics::TextureId mask = Resources::CreateResource(setup.mask, "Vegetation", nullptr, nullptr, true);
	vegetationAllocator.Set<Vegetation_Mask>(cid.id, mask);

	CoreGraphics::TextureId& albedo = vegetationAllocator.Get<Vegetation_Albedos>(cid.id);
	CoreGraphics::TextureId& normal = vegetationAllocator.Get<Vegetation_Normals>(cid.id);
	CoreGraphics::TextureId& material = vegetationAllocator.Get<Vegetation_Materials>(cid.id);
	CoreGraphics::MeshId& mesh = vegetationAllocator.Get<Vegetation_Meshes>(cid.id);
	float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
	float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);
	IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(cid.id);
	VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(cid.id);
	
	albedo = Resources::CreateResource(setup.albedo, "Vegetation", nullptr, nullptr, true);
	normal = Resources::CreateResource(setup.normals, "Vegetation", nullptr, nullptr, true);
	material = Resources::CreateResource(setup.material, "Vegetation", nullptr, nullptr, true);
	mesh = CoreGraphics::MeshId::Invalid();
	slopeThreshold = setup.slopeThreshold;
	heightThreshold = setup.heightThreshold;
	textureIndex = vegetationState.texturePool.Alloc();
	type = VegetationType::GrassType;

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			mask,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "MaskTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			albedo,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "AlbedoTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			normal,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "NormalTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			material,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "MaterialTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::SetupMesh(const Graphics::GraphicsEntityId id, const VegetationMeshSetup& setup)
{
	Graphics::ContextEntityId cid = GetContextId(id.id);
	CoreGraphics::TextureId mask = Resources::CreateResource(setup.mask, "Vegetation", nullptr, nullptr, true);
	vegetationAllocator.Set<Vegetation_Mask>(cid.id, mask);

	CoreGraphics::TextureId& albedo = vegetationAllocator.Get<Vegetation_Albedos>(cid.id);
	CoreGraphics::TextureId& normal = vegetationAllocator.Get<Vegetation_Normals>(cid.id);
	CoreGraphics::TextureId& material = vegetationAllocator.Get<Vegetation_Materials>(cid.id);
	CoreGraphics::MeshId& mesh = vegetationAllocator.Get<Vegetation_Meshes>(cid.id);
	uint& fragment = vegetationAllocator.Get<Vegetation_MeshInfo>(cid.id);
	float& slopeThreshold = vegetationAllocator.Get<Vegetation_SlopeThreshold>(cid.id);
	float& heightThreshold = vegetationAllocator.Get<Vegetation_HeightThreshold>(cid.id);
	IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(cid.id);
	VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(cid.id);

	albedo = Resources::CreateResource(setup.albedo, "Vegetation", nullptr, nullptr, true);
	normal = Resources::CreateResource(setup.normals, "Vegetation", nullptr, nullptr, true);
	material = Resources::CreateResource(setup.material, "Vegetation", nullptr, nullptr, true);
	mesh = Resources::CreateResource(setup.mesh, "Vegetation", nullptr, nullptr, true);

	fragment = vegetationState.meshInfoPool.Alloc();
	n_assert(fragment < Vegetation::MAX_MESH_INFOS);

	// get primitive groups so we can update the mesh info
	const Util::Array<CoreGraphics::PrimitiveGroup>& groups = CoreGraphics::MeshGetPrimitiveGroups(mesh);
	MeshInfo& info = vegetationState.meshInfos[fragment];
	info.indexCount = 0;
	info.vertexCount = 0;
	info.numLods = groups.Size();
	for (IndexT i = 0; i < groups.Size(); i++)
	{
		uint lod = Math::n_log2(i + 1);
		info.lodDistances[i] = 50 >> lod;
		info.lodIndexOffsets[i] = groups[i].GetBaseIndex();
		info.lodVertexOffsets[i] = groups[i].GetBaseVertex();
		info.indexCount += groups[i].GetNumIndices();
		info.vertexOffset += groups[i].GetNumVertices();
	}
	slopeThreshold = setup.slopeThreshold;
	heightThreshold = setup.heightThreshold;
	textureIndex = vegetationState.texturePool.Alloc();
	type = VegetationType::MeshType;

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			mask,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "MaskTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});
	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			albedo,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "AlbedoTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			normal,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "NormalTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	ResourceTableSetTexture(vegetationState.systemResourceTable,
		{
			material,
			ShaderGetResourceSlot(vegetationState.vegetationBaseShader, "MaterialTextureArray"),
			textureIndex,
			CoreGraphics::SamplerId::Invalid(),
			false, false
		});

	// merge mesh into global mesh
	CoreGraphics::BufferId vbo = CoreGraphics::MeshGetVertexBuffer(mesh, 0);
	CoreGraphics::BufferId ibo = CoreGraphics::MeshGetIndexBuffer(mesh);

	SizeT vboSize = CoreGraphics::BufferGetByteSize(vbo);
	SizeT iboSize = CoreGraphics::BufferGetByteSize(ibo);

	IndexT vboIndex;
	bool res = vegetationState.vboAllocator.Alloc(vboSize, 0, vboIndex);
	n_assert2(res, "Not enough space for vertices");

	IndexT iboIndex;
	res = vegetationState.iboAllocator.Alloc(iboSize, 0, iboIndex);
	n_assert2(res, "Not enough space for indices");

	// create copy commands
	MeshBufferCopy copy;
	copy.fromBuf = vbo;
	copy.toBuf = vegetationState.combinedMeshVbo;
	copy.from.offset = 0;
	copy.to.offset = vboIndex;
	copy.numBytes = vboSize;
	vegetationState.meshBufferCopiesThisFrame.Append(copy);

	copy.fromBuf = ibo;
	copy.toBuf = vegetationState.combinedMeshIbo;
	copy.from.offset = 0;
	copy.to.offset = iboIndex;
	copy.numBytes = iboSize;
	vegetationState.meshBufferCopiesThisFrame.Append(copy);
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::UpdateViewResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx)
{
	Math::mat4 cameraTransform = Math::inverse(Graphics::CameraContext::GetTransform(view->GetCamera()));
	VegetationGenerateUniforms uniforms;
	cameraTransform.position.store(uniforms.CameraPosition);
	uniforms.HeightMap = CoreGraphics::TextureGetBindlessHandle(vegetationState.heightMap);
	uniforms.WorldSize[0] = vegetationState.worldSize.x;
	uniforms.WorldSize[1] = vegetationState.worldSize.y;
	uniforms.InvWorldSize[0] = 1.0f / vegetationState.worldSize.x;
	uniforms.InvWorldSize[1] = 1.0f / vegetationState.worldSize.y;
	uniforms.GenerateQuadSize[0] = VegetationDistributionRadius;
	uniforms.GenerateQuadSize[1] = VegetationDistributionRadius;
	uniforms.MinHeight = vegetationState.minHeight;
	uniforms.MaxHeight = vegetationState.maxHeight;
	uniforms.NumGrassBlades = vegetationState.numGrassBlades;
	uniforms.HeightMapSize[0] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).width;
	uniforms.HeightMapSize[1] = CoreGraphics::TextureGetDimensions(vegetationState.heightMap).height;

	const Util::Array<VegetationType>& types = vegetationAllocator.GetArray<Vegetation_Type>();
	const Util::Array<float>& slopeThreshold = vegetationAllocator.GetArray<Vegetation_SlopeThreshold>();
	const Util::Array<float>& heightThreshold = vegetationAllocator.GetArray<Vegetation_HeightThreshold>();

	uniforms.NumGrassTypes = 0;
	uniforms.NumMeshTypes = 0;
	for (IndexT i = 0; i < types.Size(); i++)
	{
		switch (types[i])
		{
		case VegetationType::GrassType:
		{
			uniforms.GrassParameters[uniforms.NumGrassTypes][0] = 0.0f;
			uniforms.GrassParameters[uniforms.NumGrassTypes][1] = FLT_MAX;
			uniforms.GrassParameters[uniforms.NumGrassTypes][2] = slopeThreshold[i];
			uniforms.GrassParameters[uniforms.NumGrassTypes][3] = heightThreshold[i];
			uniforms.NumGrassTypes++;
			break;
		}
		case VegetationType::MeshType:
		{
			uint& meshInfo = vegetationAllocator.Get<Vegetation_MeshInfo>(i);
			uniforms.MeshParameters[uniforms.NumMeshTypes][0] = 0.0f;
			uniforms.MeshParameters[uniforms.NumMeshTypes][1] = meshInfo;
			uniforms.MeshParameters[uniforms.NumMeshTypes][2] = slopeThreshold[i];
			uniforms.MeshParameters[uniforms.NumMeshTypes][3] = heightThreshold[i];
			uniforms.NumMeshTypes++;
			break;
		}
		}
	}

	// update uniform buffer
	CoreGraphics::BufferUpdate(vegetationState.systemUniforms, uniforms);

	// update mesh info buffer
	CoreGraphics::BufferUpdateArray(vegetationState.meshInfoBuffer, vegetationState.meshInfos, Vegetation::MAX_MESH_INFOS);

	// readback draw counts
	struct DrawCounts
	{
		uint meshDraws;
		uint grassDraws;
	};

	CoreGraphics::BufferInvalidate(vegetationState.indirectCountReadbackBuffer[ctx.bufferIndex]);
	DrawCounts* counts = (DrawCounts*)CoreGraphics::BufferMap(vegetationState.indirectCountReadbackBuffer[ctx.bufferIndex]);

	// update draw counts
	vegetationState.grassDrawsThisFrame = counts->grassDraws;
	vegetationState.meshDrawsThisFrame = counts->meshDraws;

	CoreGraphics::BufferUnmap(vegetationState.indirectCountReadbackBuffer[ctx.bufferIndex]);
}

//------------------------------------------------------------------------------
/**
*/
Graphics::ContextEntityId
VegetationContext::Alloc()
{
	return vegetationAllocator.Alloc();
}

//------------------------------------------------------------------------------
/**
*/
void
VegetationContext::Dealloc(Graphics::ContextEntityId id)
{
	const VegetationType& type = vegetationAllocator.Get<Vegetation_Type>(id.id);

	// if mesh type, dealloc mesh info struct
	if (type == VegetationType::MeshType)
	{
		uint& meshInfo = vegetationAllocator.Get<Vegetation_MeshInfo>(id.id);
		vegetationState.meshInfoPool.Dealloc(meshInfo);
	}

	// dealloc texture
	IndexT& textureIndex = vegetationAllocator.Get<Vegetation_TextureIndex>(id.id);
	vegetationState.texturePool.Dealloc(textureIndex);

	vegetationAllocator.Dealloc(id.id);
}

} // namespace Vegetation
