//------------------------------------------------------------------------------
//  renderdevicebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/renderdevicebase.h"
#include "coregraphics/rendertarget.h"
#include "coregraphics/multiplerendertarget.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/feedbackbuffer.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/bufferlock.h"
#include "coregraphics/pass.h"
#include "coregraphics/displaydevice.h"
#include "math/scalar.h"
#include "graphics/graphicsserver.h"

namespace Base
{
__ImplementClass(Base::RenderDeviceBase, 'RNDB', Core::RefCounted);
__ImplementSingleton(Base::RenderDeviceBase);

using namespace Util;
using namespace CoreGraphics;
using namespace Graphics;

Util::Queue<RenderDeviceBase::__BufferLockData> RenderDeviceBase::bufferLockQueue;
//------------------------------------------------------------------------------
/**
*/
RenderDeviceBase::RenderDeviceBase() :
    isOpen(false),
    inNotifyEventHandlers(false),
    inBeginFrame(false),
    inBeginPass(false),
	inBeginFeedback(false),
    inBeginBatch(false),
	renderWireframe(false),
    visualizeMipMaps(false),
	usePatches(false),
	primitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList),
	currentFrameIndex(InvalidIndex)
{
    __ConstructSingleton;
    _setup_grouped_counter(RenderDeviceNumPrimitives, "Render");
	_setup_grouped_counter(RenderDeviceNumDrawCalls, "Render");
	_setup_grouped_counter(RenderDeviceNumComputes, "Render");
    Memory::Clear(this->streamVertexOffsets, sizeof(this->streamVertexOffsets));
}

//------------------------------------------------------------------------------
/**
*/
RenderDeviceBase::~RenderDeviceBase()
{
    n_assert(!this->IsOpen());
    n_assert(this->eventHandlers.IsEmpty());

    _discard_counter(RenderDeviceNumPrimitives);
    _discard_counter(RenderDeviceNumDrawCalls);
    _discard_counter(RenderDeviceNumComputes);
    
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    This static method can be used to check whether a RenderDevice 
    object can be created on this machine before actually instantiating
    the device object (for instance by checking whether the right Direct3D
    version is installed). Use this method at application startup
    to check if the application should run at all.
*/
bool
RenderDeviceBase::CanCreate()
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
RenderDeviceBase::Open()
{
    n_assert(!this->IsOpen());
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginPass);
    n_assert(!this->inBeginBatch);
    this->isOpen = true;

    // notify event handlers
    RenderEvent openEvent(RenderEvent::DeviceOpen);
    this->NotifyEventHandlers(openEvent);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Close()
{
    n_assert(this->IsOpen());
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginPass);
    n_assert(!this->inBeginBatch);

	// clear buffer locks
	this->bufferLockQueue.Clear();

    // notify event handlers
    RenderEvent closeEvent(RenderEvent::DeviceClose);
    this->NotifyEventHandlers(closeEvent);

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
RenderDeviceBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::SetStreamVertexBuffer(IndexT streamIndex, VertexBuffer* vb, IndexT offsetVertexIndex)
{
    n_assert((streamIndex >= 0) && (streamIndex < VertexLayoutBase::MaxNumVertexStreams));
    this->streamVertexBuffers[streamIndex] = vb;
    this->streamVertexOffsets[streamIndex] = offsetVertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
VertexBuffer*
RenderDeviceBase::GetStreamVertexBuffer(IndexT streamIndex) const
{
	n_assert((streamIndex >= 0) && (streamIndex < VertexLayoutBase::MaxNumVertexStreams));
    return this->streamVertexBuffers[streamIndex];
}

//------------------------------------------------------------------------------
/**
*/
IndexT
RenderDeviceBase::GetStreamVertexOffset(IndexT streamIndex) const
{
	n_assert((streamIndex >= 0) && (streamIndex < VertexLayoutBase::MaxNumVertexStreams));
    return this->streamVertexOffsets[streamIndex];
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::SetVertexLayout(const Ptr<VertexLayout>& vl)
{
    this->vertexLayout = vl;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<VertexLayout>&
RenderDeviceBase::GetVertexLayout() const
{
    return this->vertexLayout;
}


//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::SetPrimitiveTopology(const PrimitiveTopology::Code& topo)
{
	this->primitiveTopology = topo;
}

//------------------------------------------------------------------------------
/**
*/
const PrimitiveTopology::Code&
RenderDeviceBase::GetPrimitiveTopology() const
{
	return this->primitiveTopology;
}

//------------------------------------------------------------------------------
/**
    Attach an event handler to the render device.
*/
void
RenderDeviceBase::AttachEventHandler(const Ptr<RenderEventHandler>& handler)
{
    n_assert(handler.isvalid());
    n_assert(InvalidIndex == this->eventHandlers.FindIndex(handler));
    n_assert(!this->inNotifyEventHandlers);
    this->eventHandlers.Append(handler);
    handler->OnAttach();
}

//------------------------------------------------------------------------------
/**
    Remove an event handler from the display device.
*/
void
RenderDeviceBase::RemoveEventHandler(const Ptr<RenderEventHandler>& handler)
{
    n_assert(handler.isvalid());
    n_assert(!this->inNotifyEventHandlers);
    IndexT index = this->eventHandlers.FindIndex(handler);
    n_assert(InvalidIndex != index);
    this->eventHandlers.EraseIndex(index);
    handler->OnRemove();
}

//------------------------------------------------------------------------------
/**
    Notify all event handlers about an event.
*/
bool
RenderDeviceBase::NotifyEventHandlers(const RenderEvent& event)
{
    n_assert(!this->inNotifyEventHandlers);
    bool handled = false;
    this->inNotifyEventHandlers = true;
    IndexT i;
    for (i = 0; i < this->eventHandlers.Size(); i++)
    {
        handled |= this->eventHandlers[i]->PutEvent(event);
    }
    this->inNotifyEventHandlers = false;
    return handled;
}

//------------------------------------------------------------------------------
/**
*/
bool
RenderDeviceBase::BeginFrame(IndexT frameIndex)
{
    n_assert(!this->inBeginFrame);
    n_assert(!this->inBeginPass);
    n_assert(!this->inBeginBatch);
    n_assert(!this->indexBuffer.isvalid());
    n_assert(!this->vertexLayout.isvalid());
    IndexT i;
	for (i = 0; i < VertexLayoutBase::MaxNumVertexStreams; i++)
    {
        n_assert(!this->streamVertexBuffers[i].isvalid());
    }

	if (frameIndex != this->currentFrameIndex)
	{
		_begin_counter(RenderDeviceNumComputes);
		_begin_counter(RenderDeviceNumPrimitives);
		_begin_counter(RenderDeviceNumDrawCalls);
	}

    this->inBeginFrame = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::BeginPass(const Ptr<CoreGraphics::Pass>& pass)
{
	n_assert(this->inBeginFrame);
	n_assert(!this->inBeginPass);
	n_assert(!this->inBeginBatch);
	this->inBeginPass = true;

	this->pass = pass;
	this->pass->Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::SetToNextSubpass()
{
	n_assert(this->inBeginFrame);
	n_assert(this->inBeginPass);
	n_assert(this->pass.isvalid());

	this->pass->NextSubpass();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::BeginBatch(FrameBatchType::Code batchType)
{
    n_assert(this->inBeginPass);
    n_assert(!this->inBeginBatch);
    n_assert(this->pass.isvalid());

	this->inBeginBatch = true;
	this->pass->BeginBatch(batchType);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndBatch()
{
    n_assert(this->inBeginBatch);
	n_assert(this->pass.isvalid());

    this->inBeginBatch = false;
	this->pass->EndBatch();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndPass()
{
    n_assert(this->inBeginPass);
    n_assert(this->pass.isvalid());

	this->pass->End();
	this->pass = 0;
    this->inBeginPass = false;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndFrame(IndexT frameIndex)
{
    n_assert(this->inBeginFrame);

	if (this->currentFrameIndex != frameIndex)
	{
		_end_counter(RenderDeviceNumComputes);
		_end_counter(RenderDeviceNumPrimitives);
		_end_counter(RenderDeviceNumDrawCalls);
		this->currentFrameIndex = frameIndex;
	}	
    
    this->inBeginFrame = false;
    IndexT i;
	for (i = 0; i < VertexLayoutBase::MaxNumVertexStreams; i++)
    {
        this->streamVertexBuffers[i] = 0;
    }
    this->vertexLayout = 0;
    this->indexBuffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Present()
{
    n_assert(!this->inBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::BuildRenderPipeline()
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::InsertBarrier(const Ptr<CoreGraphics::Barrier>& barrier)
{
	// empty, override in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Draw()
{
    n_assert(this->inBeginPass);
    // override in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance)
{
    n_assert(this->inBeginPass);
    // override in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::BeginCompute(bool asyncAllowed, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>>& textureDependencies, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>>& bufferDependencies)
{
	n_assert(!this->inBeginCompute);
	this->inBeginCompute = true;
	this->inBeginAsyncCompute = asyncAllowed;
	// override and implement memory barriers for buffers and textures
}

//------------------------------------------------------------------------------
/**
*/
void 
RenderDeviceBase::Compute(int dimX, int dimY, int dimZ)
{
    n_assert(!this->inBeginPass);
    // override in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndCompute(const Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>>& textureDependencies, const Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>>& bufferDependencies)
{
	n_assert(this->inBeginCompute);
	this->inBeginCompute = false;
	this->inBeginAsyncCompute = false;
	// override and implement memory barriers needed for the textures and buffers
}

//------------------------------------------------------------------------------
/**
*/
ImageFileFormat::Code
RenderDeviceBase::SaveScreenshot(ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
    // override in subclass!
    return fmt;    
}

//------------------------------------------------------------------------------
/**
*/
ImageFileFormat::Code
RenderDeviceBase::SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y)
{
	// override in subclass!
	return fmt;
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EnqueueBufferLockIndex(const Ptr<CoreGraphics::BufferLock>& lock, IndexT buffer)
{
	__BufferLockData data;
	data.mode = __BufferLockMode::BufferRing;
	data.i = buffer;
	data.lock = lock;
	RenderDeviceBase::bufferLockQueue.Enqueue(data);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EnqueueBufferLockRange(const Ptr<CoreGraphics::BufferLock>& lock, IndexT start, SizeT range)
{
	__BufferLockData data;
	data.mode = __BufferLockMode::BufferRing;
	data.start = start;
	data.range = range;
	data.lock = lock;
	RenderDeviceBase::bufferLockQueue.Enqueue(data);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::DequeueBufferLocks()
{
	IndexT i;
	for (i = 0; i < RenderDeviceBase::bufferLockQueue.Size(); i++)
	{
		const __BufferLockData& data = RenderDeviceBase::bufferLockQueue[i];
		switch (data.mode)
		{
		case __BufferLockMode::BufferRing:
			data.lock->LockBuffer(data.i);
			break;
		case __BufferLockMode::BufferRange:
			data.lock->LockRange(data.start, data.range);
			break;
		}
	}
	RenderDeviceBase::bufferLockQueue.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion)
{
	n_assert(from.isvalid() && to.isvalid());
	n_assert(from->GetWidth() == to->GetWidth());
	n_assert(from->GetHeight() == to->GetHeight());
	n_assert(from->GetDepth() == to->GetDepth());
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from.isvalid() && to.isvalid());
	
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from.isvalid() && to.isvalid());

	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44
RenderDeviceBase::GetProjectionCorrectionMatrix()
{
	return Math::matrix44::identity();
}


} // namespace Base
