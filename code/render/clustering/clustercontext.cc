//------------------------------------------------------------------------------
//  clustercontext.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "clustercontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/graphicsdevice.h"
#include "frame/frameplugin.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"

namespace Clustering
{

struct
{
	CoreGraphics::ShaderId clusterShader;
	CoreGraphics::ShaderProgramId clusterGenerateProgram;
	CoreGraphics::BufferId clusterBuffer;

	ClusterGenerate::ClusterUniforms uniforms;
	CoreGraphics::ConstantBufferId constantBuffer;
	IndexT uniformsSlot;

	SizeT clusterDimensions[3];
	float zDistribution;
	float zInvScale, zInvBias;
	float xResolution, yResolution;
	float invXResolution, invYResolution;

	SizeT numThreads;

	CoreGraphics::ResourceTableId resourceTable;

	static const SizeT ClusterSubdivsX = 64;
	static const SizeT ClusterSubdivsY = 64;
	static const SizeT ClusterSubdivsZ = 24;
} state;

_ImplementPluginContext(ClusterContext);

//------------------------------------------------------------------------------
/**
*/
ClusterContext::ClusterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
ClusterContext::~ClusterContext()
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::Create(float ZNear, float ZFar, const CoreGraphics::WindowId window)
{
	__bundle.OnUpdateResources = ClusterContext::UpdateResources;
	__bundle.StageBits = &ClusterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = ClusterContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	using namespace CoreGraphics;
	state.clusterShader = ShaderGet("shd:cluster_generate.fxb");
	state.clusterGenerateProgram = ShaderGetProgram(state.clusterShader, ShaderFeatureFromString("AABBGenerate"));

	uint numBuffers = CoreGraphics::GetNumBufferedFrames();

	state.uniformsSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterUniforms");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterAABBs");

	CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(window);

	state.clusterDimensions[0] = Math::n_divandroundup(displayMode.GetWidth(), state.ClusterSubdivsX);
	state.clusterDimensions[1] = Math::n_divandroundup(displayMode.GetHeight(), state.ClusterSubdivsY);
	state.clusterDimensions[2] = state.ClusterSubdivsZ;

	state.zDistribution = ZFar / ZNear;
	state.zInvScale = float(state.clusterDimensions[2]) / Math::n_log2(state.zDistribution);
	state.zInvBias = -(float(state.clusterDimensions[2]) * Math::n_log2(ZNear) / Math::n_log2(state.zDistribution));
	state.xResolution = displayMode.GetWidth();
	state.yResolution = displayMode.GetHeight();
	state.invXResolution = 1.0f / displayMode.GetWidth();
	state.invYResolution = 1.0f / displayMode.GetHeight();

	BufferCreateInfo rwb3Info;
	rwb3Info.name = "ClusterAABBBuffer";
	rwb3Info.size = state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
	rwb3Info.elementSize = sizeof(ClusterGenerate::ClusterAABB);
	rwb3Info.mode = BufferAccessMode::DeviceLocal;
	rwb3Info.usageFlags = CoreGraphics::ReadWriteBuffer;
	state.clusterBuffer = CreateBuffer(rwb3Info);
	state.constantBuffer = ShaderCreateConstantBuffer(state.clusterShader, "ClusterUniforms");

	state.resourceTable = ShaderCreateResourceTable(state.clusterShader, NEBULA_BATCH_GROUP);
	ResourceTableSetRWBuffer(state.resourceTable, { state.clusterBuffer, clusterAABBSlot, 0, false, false, -1, 0 });
	ResourceTableSetConstantBuffer(state.resourceTable, { state.constantBuffer, state.uniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), 0 });
	ResourceTableCommitChanges(state.resourceTable);

	// called from main script
	Frame::AddCallback("ClusterContext - Update Clusters", [](const IndexT frame, const IndexT frameBufferIndex) // trigger update
		{
			UpdateClusters();
		});
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::BufferId 
ClusterContext::GetClusterBuffer()
{
	return state.clusterBuffer;
}

//------------------------------------------------------------------------------
/**
*/
const SizeT 
ClusterContext::GetNumClusters()
{
	return state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2];
}

//------------------------------------------------------------------------------
/**
*/
const std::array<SizeT, 3> 
ClusterContext::GetClusterDimensions()
{
	return std::array<SizeT, 3> { state.clusterDimensions[0], state.clusterDimensions[1], state.clusterDimensions[2] };
}

//------------------------------------------------------------------------------
/**
*/
const ClusterGenerate::ClusterUniforms&
ClusterContext::GetUniforms()
{
	return state.uniforms;
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ConstantBufferId 
ClusterContext::GetConstantBuffer()
{
	return state.constantBuffer;
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::UpdateResources(const Graphics::FrameContext& ctx)
{
	using namespace CoreGraphics;

	state.uniforms.ZDistribution = state.zDistribution;
	state.uniforms.InvZScale = state.zInvScale;
	state.uniforms.InvZBias = state.zInvBias;
	state.uniforms.InvFramebufferDimensions[0] = state.invXResolution;
	state.uniforms.InvFramebufferDimensions[1] = state.invYResolution;
	state.uniforms.FramebufferDimensions[0] = state.xResolution;
	state.uniforms.FramebufferDimensions[1] = state.yResolution;
	state.uniforms.NumCells[0] = state.clusterDimensions[0];
	state.uniforms.NumCells[1] = state.clusterDimensions[1];
	state.uniforms.NumCells[2] = state.clusterDimensions[2];
	state.uniforms.BlockSize[0] = state.ClusterSubdivsX;
	state.uniforms.BlockSize[1] = state.ClusterSubdivsY;

	// update constant buffer, probably super unnecessary since these values never change
	ConstantBufferUpdate(state.constantBuffer, state.uniforms, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::OnRenderDebug(uint32_t flags)
{
}

//------------------------------------------------------------------------------
/**
*/
void 
ClusterContext::UpdateClusters()
{
	// update constants
	using namespace CoreGraphics;

	// begin command buffer work
	CommandBufferBeginMarker(ComputeQueueType, NEBULA_MARKER_BLUE, "Cluster AABB Generation");

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				state.clusterBuffer,
				BarrierAccess::ShaderRead,
				BarrierAccess::ShaderWrite,
				0, -1
			}
		}, "AABB begin barrier");

	SetShaderProgram(state.clusterGenerateProgram, ComputeQueueType);

	uint bufferIndex = CoreGraphics::GetBufferedFrameIndex();
	SetResourceTable(state.resourceTable, NEBULA_BATCH_GROUP, CoreGraphics::ComputePipeline, nullptr, ComputeQueueType);

	// run the job as series of 1024 clusters at a time
	Compute(Math::n_ceil((state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2]) / 64.0f), 1, 1, ComputeQueueType);

	// make sure to sync so we don't read from data that is being written...
	BarrierInsert(ComputeQueueType,
		BarrierStage::ComputeShader,
		BarrierStage::ComputeShader,
		BarrierDomain::Global,
		nullptr,
		{
			BufferBarrier
			{
				state.clusterBuffer,
				BarrierAccess::ShaderWrite,
				BarrierAccess::ShaderRead,
				0, -1
			}
		}
		, "AABB finish barrier");

	CommandBufferEndMarker(ComputeQueueType);
}
} // namespace Clustering
