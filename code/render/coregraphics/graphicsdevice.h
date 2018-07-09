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
#include "debug/debugcounter.h"
#include "coregraphics/rendereventhandler.h"

namespace CoreGraphics
{

struct GraphicsDeviceCreateInfo
{

};

/// create graphics device
bool CreateGraphicsDevice(const GraphicsDeviceCreateInfo& info);
/// destroy graphics device
void DestroyGraphicsDevice();

struct GraphicsDeviceState
{
	Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;
	CoreGraphics::PrimitiveTopology::Code primitiveTopology;
	CoreGraphics::PrimitiveGroup primitiveGroup;
	CoreGraphics::PassId pass;
	bool isOpen;
	bool inNotifyEventHandlers;
	bool inBeginFrame;
	bool inBeginPass;
	bool inBeginBatch;
	bool inBeginCompute;
	bool inBeginAsyncCompute;
	bool renderWireframe;
	bool visualizeMipMaps;
	bool usePatches;
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
/// set shader state
void SetShaderState(const CoreGraphics::ShaderStateId& sh);

/// insert execution barrier
void InsertBarrier(const CoreGraphics::BarrierId barrier, const CoreGraphicsQueueType queue);
/// signals an event
void SignalEvent(const CoreGraphics::EventId& ev, const CoreGraphicsQueueType queue);
/// signals an event
void WaitEvent(const CoreGraphics::EventId& ev, const CoreGraphicsQueueType queue);
/// signals an event
void ResetEvent(const CoreGraphics::EventId& ev, const CoreGraphicsQueueType queue);
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

} // namespace CoreGraphics
