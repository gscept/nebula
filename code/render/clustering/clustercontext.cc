//------------------------------------------------------------------------------
//  clustercontext.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "clustercontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/graphicsdevice.h"
#include "frame/plugins/frameplugin.h"
#include "graphics/graphicsserver.h"
#include "graphics/cameracontext.h"

namespace Clustering
{

struct
{
	CoreGraphics::ShaderId clusterShader;
	CoreGraphics::ShaderProgramId clusterGenerateProgram;
	CoreGraphics::ShaderRWBufferId clusterBuffer;

	ClusterGenerate::ClusterUniforms uniforms;
	IndexT uniformsSlot;

	SizeT clusterDimensions[3];
	float zDistribution;
	float zInvScale, zInvBias;
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
ClusterContext::Create(const Graphics::GraphicsEntityId camera, const CoreGraphics::WindowId window)
{
	__bundle.OnBeforeFrame = nullptr;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = ClusterContext::OnBeforeView;
	__bundle.OnAfterView = nullptr;
	__bundle.OnAfterFrame = nullptr;
	__bundle.StageBits = &ClusterContext::__state.currentStage;
#ifndef PUBLIC_BUILD
	__bundle.OnRenderDebug = ClusterContext::OnRenderDebug;
#endif
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);

	using namespace CoreGraphics;
	state.clusterShader = ShaderGet("shd:cluster_generate.fxb");
	state.clusterGenerateProgram = ShaderGetProgram(state.clusterShader, ShaderFeatureFromString("AABBGenerate"));
	state.resourceTable = ShaderCreateResourceTable(state.clusterShader, NEBULA_BATCH_GROUP);
	state.uniformsSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterUniforms");
	IndexT clusterAABBSlot = ShaderGetResourceSlot(state.clusterShader, "ClusterAABBs");

	CoreGraphics::DisplayMode displayMode = CoreGraphics::WindowGetDisplayMode(window);
	const Graphics::CameraSettings settings = Graphics::CameraContext::GetSettings(camera);

	state.clusterDimensions[0] = Math::n_divandroundup(displayMode.GetWidth(), state.ClusterSubdivsX);
	state.clusterDimensions[1] = Math::n_divandroundup(displayMode.GetHeight(), state.ClusterSubdivsY);
	state.clusterDimensions[2] = state.ClusterSubdivsZ;

	state.zDistribution = settings.GetZFar() / settings.GetZNear();
	state.zInvScale = float(state.clusterDimensions[2]) / Math::n_log2(state.zDistribution);
	state.zInvBias = -(float(state.clusterDimensions[2]) * Math::n_log2(settings.GetZNear()) / Math::n_log2(state.zDistribution));
	state.invXResolution = 1.0f / displayMode.GetWidth();
	state.invYResolution = 1.0f / displayMode.GetHeight();

	ShaderRWBufferCreateInfo rwb3Info =
	{
		"ClusterAABBBuffer",
		state.clusterDimensions[0] * state.clusterDimensions[1] * state.clusterDimensions[2] * sizeof(ClusterGenerate::ClusterAABB),
		1,
		false
	};
	state.clusterBuffer = CreateShaderRWBuffer(rwb3Info);

	ResourceTableSetRWBuffer(state.resourceTable, { state.clusterBuffer, clusterAABBSlot, 0, false, false, -1, 0 });

	// called from main script
	Frame::FramePlugin::AddCallback("ClusterContext - Update Clusters", [](IndexT frame) // trigger update
		{
			UpdateClusters();
		});
}

//------------------------------------------------------------------------------
/**
*/
const CoreGraphics::ShaderRWBufferId 
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
void 
ClusterContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
	using namespace CoreGraphics;

	state.uniforms.ZDistribution = state.zDistribution;
	state.uniforms.InvZScale = state.zInvScale;
	state.uniforms.InvZBias = state.zInvBias;
	state.uniforms.InvFramebufferDimensions[0] = state.invXResolution;
	state.uniforms.InvFramebufferDimensions[1] = state.invYResolution;
	state.uniforms.NumCells[0] = state.clusterDimensions[0];
	state.uniforms.NumCells[1] = state.clusterDimensions[1];
	state.uniforms.NumCells[2] = state.clusterDimensions[2];
	state.uniforms.BlockSize[0] = state.ClusterSubdivsX;
	state.uniforms.BlockSize[1] = state.ClusterSubdivsY;

	uint offset = SetComputeConstants(MainThreadConstantBuffer, state.uniforms);
	ResourceTableSetConstantBuffer(state.resourceTable, { GetComputeConstantBuffer(MainThreadConstantBuffer), state.uniformsSlot, 0, false, false, sizeof(ClusterGenerate::ClusterUniforms), (SizeT)offset });
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

	ResourceTableCommitChanges(state.resourceTable);

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
