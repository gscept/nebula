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

namespace CoreGraphics
{

struct GraphicsDeviceCreateInfo
{
	bool enableValidation : 1;		// enables validation layer and writes output to console
};

/// create graphics device
bool CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info);
/// destroy graphics device
void DestroyGraphicsDevice();

struct GraphicsDeviceState
{
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> renderTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWTextureId> shaderRWTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWBufferId> shaderRWBuffers;
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
/// bake the current state of the render device (only used on DX12 and Vulkan renderers where pipeline creation is required)
void BuildRenderPipeline();
/// set shader program
void SetShaderProgram(const CoreGraphics::ShaderProgramId& pro);
/// set shader program
void SetShaderProgram(const CoreGraphics::ShaderId shaderId, const CoreGraphics::ShaderFeature::Mask mask);
/// set resource table
void SetResourceTable(const CoreGraphics::ResourceTableId table, const IndexT slot, ShaderPipeline pipeline, const Util::FixedArray<uint>& offsets);
/// set resoure table layout
void SetResourceTablePipeline(const CoreGraphics::ResourcePipelineId layout);
/// push constants
void PushConstants(ShaderPipeline pipeline, uint offset, uint size, byte* data);
/// set pipeline using current layout, shader and pass
void SetGraphicsPipeline();

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
/// end current frame
void EndFrame(IndexT frameIndex);
/// check if inside BeginFrame
bool IsInBeginFrame();
/// present the rendered scene
void Present();
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
void CmdBufBeginMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
/// end debug marker region
void CmdBufEndMarker(const CoreGraphicsQueueType queue);
/// insert marker
void CmdBufInsertMarker(const CoreGraphicsQueueType queue, const Math::float4& color, const Util::String& name);
#endif



} // namespace CoreGraphics
