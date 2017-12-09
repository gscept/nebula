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
#include "coregraphics/framebatchtype.h"
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

namespace CoreGraphics
{
class Texture;
class RenderTexture;
class VertexBuffer;
class IndexBuffer;
class FeedbackBuffer;
class VertexLayout;
class ShaderState;
class RenderTarget;
class MultipleRenderTarget;
class BufferLock;
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

	enum MemoryBarrierBits
	{
		NoBarrierBit = 0,
		ImageAccessBarrierBits			= 1 << 0,	// subsequent images will see data done written before barrier
		BufferAccessBarrierBits			= 1 << 1,	// subsequent buffers will see data done written before barrier
		SamplerAccessBarrierBits		= 1 << 2,	// subsequent texture samplers will see data done written before barrier
		RenderTargetAccessBarrierBits	= 1 << 3	// subsequent samples from render targets will be visible
	};

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
	void BeginPass(const Ptr<CoreGraphics::Pass>& pass);
	/// progress to next subpass
	void SetToNextSubpass();
    /// begin rendering a batch
    void BeginBatch(CoreGraphics::FrameBatchType::Code batchType);
    /// set the current vertex stream source
    void SetStreamVertexBuffer(IndexT streamIndex, CoreGraphics::VertexBuffer* vb, IndexT offsetVertexIndex);
    /// get currently set vertex buffer
    CoreGraphics::VertexBuffer* GetStreamVertexBuffer(IndexT streamIndex) const;
    /// get currently set vertex stream offset
    IndexT GetStreamVertexOffset(IndexT streamIndex) const;
    /// set current vertex layout
    void SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& vl);
    /// get current vertex layout
    const Ptr<CoreGraphics::VertexLayout>& GetVertexLayout() const;
    /// set current index buffer
    void SetIndexBuffer(CoreGraphics::IndexBuffer* ib, IndexT offsetIndex);
    /// get current index buffer
    CoreGraphics::IndexBuffer* GetIndexBuffer() const;
	/// set the type of topology to be used
	void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code& topo);
	/// get the type of topology used
	const CoreGraphics::PrimitiveTopology::Code& GetPrimitiveTopology() const;
    /// set current primitive group
    void SetPrimitiveGroup(const CoreGraphics::PrimitiveGroup& pg);
    /// get current primitive group
    const CoreGraphics::PrimitiveGroup& GetPrimitiveGroup() const;
	/// bake the current state of the render device (only used on DX12 and Vulkan renderers where pipeline creation is required)
	void BuildRenderPipeline();
	/// insert execution barrier
	void InsertBarrier(const Ptr<CoreGraphics::Barrier>& barrier);
    /// draw current primitives
    void Draw();
    /// draw indexed, instanced primitives
    void DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance);
	/// begin computation, if asyncAllowed is true, then implementation may perform a compute in parallel with graphics
	void BeginCompute(bool asyncAllowed, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>>& textureDependencies, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>>& bufferDependencies);
    /// perform computation
	void Compute(int dimX, int dimY, int dimZ);
	/// end computation
	void EndCompute(const Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>>& textureDependencies, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>>& bufferDependencies);
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

	/// enqueue a buffer lock which will cause the render device to lock a buffer index whenever the next draw command gets executed
	static void EnqueueBufferLockIndex(const Ptr<CoreGraphics::BufferLock>& lock, IndexT buffer);
	/// enqueue a buffer lock which will cause the render device to lock a buffer range whenever the next draw command gets executed
	static void EnqueueBufferLockRange(const Ptr<CoreGraphics::BufferLock>& lock, IndexT start, SizeT range);
	/// empty queues and lock buffers
	static void DequeueBufferLocks();

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

	enum __BufferLockMode
	{
		BufferRing = 0,
		BufferRange = 1
	};
	struct __BufferLockData
	{
		Ptr<CoreGraphics::BufferLock> lock;
		__BufferLockMode mode;
		IndexT start;
		SizeT range;
		IndexT i;
	};
    
	static Util::Queue<__BufferLockData> bufferLockQueue;
    Util::Array<Ptr<CoreGraphics::RenderEventHandler> > eventHandlers;
    CoreGraphics::VertexBuffer* streamVertexBuffers[VertexLayoutBase::MaxNumVertexStreams];
	IndexT streamVertexOffsets[VertexLayoutBase::MaxNumVertexStreams];
    Ptr<CoreGraphics::VertexLayout> vertexLayout;
    CoreGraphics::IndexBuffer* indexBuffer;
	CoreGraphics::PrimitiveTopology::Code primitiveTopology;
    CoreGraphics::PrimitiveGroup primitiveGroup;
	Ptr<CoreGraphics::Pass> pass;
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
RenderDeviceBase::SetIndexBuffer(IndexBuffer* ib, IndexT offsetIndex)
{
	this->indexBuffer = ib;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexBuffer*
RenderDeviceBase::GetIndexBuffer() const
{
	return this->indexBuffer;
}

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

