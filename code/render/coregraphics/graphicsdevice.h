#pragma once
//------------------------------------------------------------------------------
/**
	The Graphics Device is the engine which drives the graphics abstraction layer

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "pass.h"
#include "frame/framescript.h"
#include "primitivegroup.h"
#include "vertexbuffer.h"
#include "indexbuffer.h"
#include "imagefileformat.h"
#include "io/stream.h"
#include "fence.h"
#include "debug/debugcounter.h"
#include "coregraphics/rendereventhandler.h"
#include "coregraphics/submissioncontext.h"

namespace CoreGraphics
{

struct GraphicsDeviceCreateInfo
{
	uint globalGraphicsConstantBufferMemorySize[NumConstantBufferTypes];
	uint globalComputeConstantBufferMemorySize[NumConstantBufferTypes];
	byte numBufferedFrames : 3;
	bool enableValidation : 1;		// enables validation layer and writes output to console
};

/// create graphics device
bool CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info);
/// destroy graphics device
void DestroyGraphicsDevice();

/// get number of buffered frames
SizeT GetNumBufferedFrames();
/// get current frame index, which is between 0 and GetNumBufferedFrames
IndexT GetBufferedFrameIndex();


struct GraphicsDeviceState
{
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> renderTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWTextureId> shaderRWTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWBufferId> shaderRWBuffers;

	CoreGraphics::SubmissionContextId resourceSubmissionContext;
	CoreGraphics::CommandBufferId resourceSubmissionCmdBuffer;
	CoreGraphics::SemaphoreId resourceSubmissionSemaphore;
	CoreGraphics::FenceId resourceSubmissionFence;
	Threading::CriticalSection resourceSubmissionCriticalSection;
	bool resourceSubmissionActive;

	CoreGraphics::SubmissionContextId setupSubmissionContext;
	CoreGraphics::CommandBufferId setupSubmissionCmdBuffer;
	CoreGraphics::SemaphoreId setupSubmissionSemaphore;
	bool setupSubmissionActive;

	CoreGraphics::SubmissionContextId gfxSubmission;
	CoreGraphics::CommandBufferId gfxCmdBuffer;
	CoreGraphics::SemaphoreId gfxPrevSemaphore;
	CoreGraphics::SemaphoreId gfxSemaphore;
	CoreGraphics::SemaphoreId gfxWaitSemaphore;
	CoreGraphics::FenceId gfxFence;

	CoreGraphics::SubmissionContextId computeSubmission;
	CoreGraphics::CommandBufferId computeCmdBuffer;
	CoreGraphics::SemaphoreId computePrevSemaphore;
	CoreGraphics::SemaphoreId computeSemaphore;
	CoreGraphics::SemaphoreId computeWaitSemaphore;
	CoreGraphics::FenceId computeFence;

	uint globalGraphicsConstantBufferMaxValue[NumConstantBufferTypes];
	CoreGraphics::ConstantBufferId globalGraphicsConstantBuffer[NumConstantBufferTypes];
	uint globalComputeConstantBufferMaxValue[NumConstantBufferTypes];
	CoreGraphics::ConstantBufferId globalComputeConstantBuffer[NumConstantBufferTypes];

	Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;
	CoreGraphics::PrimitiveTopology::Code primitiveTopology;
	CoreGraphics::PrimitiveGroup primitiveGroup;
	CoreGraphics::PassId pass;
	bool isOpen : 1;
	bool inNotifyEventHandlers : 1;
	bool inBeginFrame : 1;
	bool inBeginPass : 1;
	bool inBeginBatch : 1;
	bool inBeginCompute : 1;
	bool inBeginAsyncCompute : 1;
	bool renderWireframe : 1;
	bool visualizeMipMaps : 1;
	bool usePatches : 1;
	bool enableValidation : 1;
	IndexT currentFrameIndex;

	_declare_counter(RenderDeviceNumComputes);
	_declare_counter(RenderDeviceNumPrimitives);
	_declare_counter(RenderDeviceNumDrawCalls);
};

/// attach a render event handler
void AttachEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);
/// remove a render event handler
void RemoveEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);
/// notify event handlers about an event
bool NotifyEventHandlers(const CoreGraphics::RenderEvent& e);

/// begin complete frame
bool BeginFrame(IndexT frameIndex);
/// start a new submission, with an optional argument for waiting for another queue
void BeginSubmission(CoreGraphicsQueueType queue, CoreGraphicsQueueType waitQueue);
/// begin a rendering pass
void BeginPass(const CoreGraphics::PassId pass);
/// progress to next subpass	
void SetToNextSubpass();
/// begin rendering a batch
void BeginBatch(Frame::FrameBatchType::Code batchType);
/// go to next sub-batch
void SetToNextSubBatch();

/// reset clip settings to pass
void ResetClipSettings();

/// set the current vertex stream source
void SetStreamVertexBuffer(IndexT streamIndex, const CoreGraphics::VertexBufferId& vb, IndexT offsetVertexIndex);
/// set current vertex layout
void SetVertexLayout(const CoreGraphics::VertexLayoutId& vl);
/// set current index buffer
void SetIndexBuffer(const CoreGraphics::IndexBufferId& ib, IndexT offsetIndex);
/// set the type of topology used
void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code topo);
/// get the type of topology used
const CoreGraphics::PrimitiveTopology::Code& GetPrimitiveTopology();
/// set current primitive group
void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg);
/// get current primitive group
const CoreGraphics::PrimitiveGroup& GetPrimitiveGroup();
/// set shader program
void SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
/// set shader program
void SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
/// set resource table
void SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets);
/// set resource table using raw offsets
void SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, uint32 numOffsets, uint32* offsets);
/// set resoure table layout
void SetResourceTablePipeline(const CoreGraphics::ResourcePipelineId layout);

/// push constants
void PushConstants(ShaderPipeline pipeline, uint offset, uint size, byte* data);
/// reserve range of graphics constant buffer memory and return offset
uint AllocateGraphicsConstantBufferMemory(CoreGraphicsGlobalConstantBufferType type, uint size);
/// return id to global graphics constant buffer
CoreGraphics::ConstantBufferId GetGraphicsConstantBuffer(CoreGraphicsGlobalConstantBufferType type);
/// reserve range of compute constant buffer memory and return offset
uint AllocateComputeConstantBufferMemory(CoreGraphicsGlobalConstantBufferType type, uint size);
/// return id to global compute constant buffer
CoreGraphics::ConstantBufferId GetComputeConstantBuffer(CoreGraphicsGlobalConstantBufferType type);
/// return resource submission context
CoreGraphics::SubmissionContextId GetResourceSubmissionContext();
/// return setup submission context
CoreGraphics::SubmissionContextId GetSetupSubmissionContext();
/// return command buffer for current frame graphics submission
CoreGraphics::CommandBufferId GetGfxCommandBuffer();
/// return command buffer for current frame async compute submission
CoreGraphics::CommandBufferId GetComputeCommandBuffer();

/// set pipeline using current layout, shader and pass
void SetGraphicsPipeline();

/// trigger reloading a shader
void ReloadShaderProgram(const CoreGraphics::ShaderProgramId& pro);

/// insert execution barrier
void InsertBarrier(const CoreGraphics::BarrierId barrier, const CoreGraphicsQueueType queue);
/// signals an event
void SignalEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue);
/// signals an event
void WaitEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue);
/// signals an event
void ResetEvent(const CoreGraphics::EventId ev, const CoreGraphicsQueueType queue);
/// insert a fence to be signaled in the selected queue
void SignalFence(const CoreGraphics::FenceId fe, const CoreGraphicsQueueType queue);
/// peek fence, returns true if fence is signaled
bool PeekFence(const CoreGraphics::FenceId fe);
/// reset fence
void ResetFence(const CoreGraphics::FenceId fe);
/// wait for a fence indefinitely (using UINT_MAX) or with a timeout, returns bool if fence was encountered
bool WaitFence(const CoreGraphics::FenceId fe, uint64 wait);

/// draw current primitives
void Draw();
/// draw indexed, instanced primitives
void DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance);
/// perform computation
void Compute(int dimX, int dimY, int dimZ);
/// end current batch
void EndBatch();
/// end current pass
void EndPass();
/// end the current submission, 
void EndSubmission(CoreGraphicsQueueType queue, bool endOfFrame = false);
/// end current frame
void EndFrame(IndexT frameIndex);
/// check if inside BeginFrame
bool IsInBeginFrame();
/// wait for an individual queue to finish
void WaitForQueue(CoreGraphicsQueueType queue);
/// wait for all queues to finish
void WaitForAllQueues();


/// save a screenshot to the provided stream
CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
/// save a region of the screen to the provided stream
CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y);
/// get visualization of mipmaps flag	
bool GetVisualizeMipMaps();
/// set visualization of mipmaps flag	
void SetVisualizeMipMaps(bool val);
/// get the render as wireframe flag
bool GetRenderWireframe();
/// set the render as wireframe flag
void SetRenderWireframe(bool b);

/// insert timestamp, returns handle to timestamp, which can be retreived on the next N'th frame where N is the number of buffered frames
IndexT Timestamp(CoreGraphicsQueueType queue, const CoreGraphics::BarrierStage stage);
/// start query
IndexT BeginQuery(CoreGraphicsQueryType type);
/// end query
void EndQuery(CoreGraphicsQueueType queue, CoreGraphicsQueryType type, IndexT query);

/// copy data between textures
void Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion);
/// copy data between render textures
void Copy(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion);

/// blit between textures
void Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);
/// blit between render textures
void Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);

/// sets whether or not the render device should tessellate
void SetUsePatches(bool state);
/// gets whether or not the render device should tessellate
bool GetUsePatches();

/// adds a viewport
void SetViewport(const Math::rectangle<int>& rect, int index);
/// adds a scissor rect
void SetScissorRect(const Math::rectangle<int>& rect, int index);

/// register render texture
void RegisterRenderTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId id);
/// get render texture
const CoreGraphics::RenderTextureId GetRenderTexture(const Util::StringAtom& name);
/// register render texture
void RegisterShaderRWTexture(const Util::StringAtom& name, const CoreGraphics::ShaderRWTextureId id);
/// get render texture
const CoreGraphics::ShaderRWTextureId GetShaderRWTexture(const Util::StringAtom& name);
/// register render texture
void RegisterShaderRWBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId id);
/// get render texture
const CoreGraphics::ShaderRWBufferId GetShaderRWBuffer(const Util::StringAtom& name);

#if NEBULA_GRAPHICS_DEBUG
/// set debug name for object
template<typename OBJECT_ID_TYPE> void ObjectSetName(const OBJECT_ID_TYPE id, const Util::String& name);
/// begin debug marker region
void QueueBeginMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
/// end debug marker region
void QueueEndMarker(const CoreGraphicsQueueType queue);
/// insert marker
void QueueInsertMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
/// begin debug marker region
void CommandBufferBeginMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
/// end debug marker region
void CommandBufferEndMarker(const CoreGraphicsQueueType queue);
/// insert marker
void CommandBufferInsertMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
#endif



} // namespace CoreGraphics
