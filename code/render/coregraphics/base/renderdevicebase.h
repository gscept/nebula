#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::RenderDeviceBase
  
    The central rendering object of the Nebula3 core graphics system. This
    is basically an encapsulation of the Direct3D device. The render device
    will presents its backbuffer to the display managed by the
    CoreGraphics::DisplayDevice singleton.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/rendereventhandler.h"
#include "coregraphics/primitivegroup.h"
#include "models/framebatchtype.h"
#include "coregraphics/imagefileformat.h"
#include "io/stream.h"
#include "debug/debugcounter.h"
#include "math/float4.h"
#include "math/rectangle.h"
#include "graphics/view.h"
#include "graphics/graphicsserver.h"
#include "coregraphics/shader.h"
#include "util/queue.h"
#include "vertexlayoutbase.h"
#include "coregraphics/barrier.h"
#include "coregraphics/texture.h"
#include "coregraphics/rendertexture.h"

namespace CoreGraphics
{
class Pass;
};

//------------------------------------------------------------------------------
namespace Base
{
class RenderDeviceBase : public Core::RefCounted
{
    __DeclareClass(RenderDeviceBase);
    __DeclareSingleton(RenderDeviceBase);
public:

    /// constructor
    RenderDeviceBase();
    /// destructor
    virtual ~RenderDeviceBase();
    /// test if a compatible render device can be created on this machine
    static bool CanCreate();

    /// open the device
    bool Open();
    /// close the device
    void Close();
    /// return true if currently open
    bool IsOpen() const;
    /// attach a render event handler
    void AttachEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);
    /// remove a render event handler
    void RemoveEventHandler(const Ptr<CoreGraphics::RenderEventHandler>& h);

    /// begin complete frame
    bool BeginFrame(IndexT frameIndex);
	/// begin a rendering pass
	void BeginPass(const CoreGraphics::PassId pass);
	/// progress to next subpass
	void SetToNextSubpass();
    /// begin rendering a batch
    void BeginBatch(Models::FrameBatchType::Code batchType);
	/// set the type of topology used
	void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code topo);
	/// get the type of topology used
	const CoreGraphics::PrimitiveTopology::Code& GetPrimitiveTopology() const;
    /// set current primitive group
    void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg);
    /// get current primitive group
    const CoreGraphics::PrimitiveGroup& GetPrimitiveGroup() const;
	/// bake the current state of the render device (only used on DX12 and Vulkan renderers where pipeline creation is required)
	void BuildRenderPipeline();
	/// insert execution barrier
	void InsertBarrier(const CoreGraphics::BarrierId barrier);
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
    bool IsInBeginFrame() const;
    /// present the rendered scene
    void Present();
    /// save a screenshot to the provided stream
    CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
	/// save a region of the screen to the provided stream
	CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y);
    /// get visualization of mipmaps flag	
    bool GetVisualizeMipMaps() const;
    /// set visualization of mipmaps flag	
    void SetVisualizeMipMaps(bool val);
	/// set the render as wireframe flag
	void SetRenderWireframe(bool b);
	/// get the render as wireframe flag
	bool GetRenderWireframe() const;

	/// copy data between textures
	void Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion);
	/// blit between render textures
	void Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);
	/// blit between textures
	void Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip);

	/// sets whether or not the render device should tessellate
	void SetUsePatches(bool state);
	/// gets whether or not the render device should tessellate
	bool GetUsePatches();

	/// adds a viewport
	void SetViewport(const Math::rectangle<int>& rect, int index);
	/// adds a scissor rect
	void SetScissorRect(const Math::rectangle<int>& rect, int index);

	/// get implementation specific correction matrix
	static const Math::matrix44 GetProjectionCorrectionMatrix();

protected:
    /// notify event handlers about an event
    bool NotifyEventHandlers(const CoreGraphics::RenderEvent& e);

    
    Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;
	/*
    CoreGraphics::VertexBuffer* streamVertexBuffers[VertexLayoutBase::MaxNumVertexStreams];
	IndexT streamVertexOffsets[VertexLayoutBase::MaxNumVertexStreams];
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
    CoreGraphics::IndexBuffer* indexBuffer;
	*/
	CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    CoreGraphics::PrimitiveGroup primitiveGroup;
	CoreGraphics::PassId pass;
    bool isOpen;
    bool inNotifyEventHandlers;
    bool inBeginFrame;
    bool inBeginPass;
	bool inBeginFeedback;
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

//------------------------------------------------------------------------------
/**
*/
inline void
RenderDeviceBase::SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg)
{
    this->primitiveGroup = pg;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::PrimitiveGroup&
RenderDeviceBase::GetPrimitiveGroup() const
{
    return this->primitiveGroup;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
RenderDeviceBase::IsInBeginFrame() const
{
    return this->inBeginFrame;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
RenderDeviceBase::GetVisualizeMipMaps() const
{
    return this->visualizeMipMaps;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
RenderDeviceBase::SetVisualizeMipMaps(bool val)
{
    this->visualizeMipMaps = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
RenderDeviceBase::SetRenderWireframe(bool b)
{
	this->renderWireframe = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
RenderDeviceBase::GetRenderWireframe() const
{
	return this->renderWireframe;
}

//------------------------------------------------------------------------------
/**
*/
inline void
RenderDeviceBase::SetUsePatches(bool state)
{
	this->usePatches = state;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
RenderDeviceBase::GetUsePatches()
{
	return this->usePatches;
}

//------------------------------------------------------------------------------
/**
*/
inline void
RenderDeviceBase::SetViewport(const Math::rectangle<int>& rect, int index)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
inline void
RenderDeviceBase::SetScissorRect(const Math::rectangle<int>& rect, int index)
{
	// implement in subclass
}

} // namespace Base
//------------------------------------------------------------------------------

