//------------------------------------------------------------------------------
//  renderdevicebase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/base/renderdevicebase.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics/pass.h"
#include "coregraphics/displaydevice.h"
#include "math/scalar.h"

namespace Base
{
__ImplementClass(Base::RenderDeviceBase, 'RNDB', Core::RefCounted);
__ImplementSingleton(Base::RenderDeviceBase);

using namespace Util;
using namespace CoreGraphics;
using namespace Graphics;

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
RenderDeviceBase::SetPrimitiveTopology(const PrimitiveTopology::Code topo)
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
RenderDeviceBase::BeginPass(const CoreGraphics::PassId pass)
{
	n_assert(this->inBeginFrame);
	n_assert(!this->inBeginPass);
	n_assert(!this->inBeginBatch);
	n_assert(this->pass == PassId::Invalid());
	this->inBeginPass = true;

	this->pass = pass;
	PassBegin(pass);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::SetToNextSubpass()
{
	n_assert(this->inBeginFrame);
	n_assert(this->inBeginPass);
	n_assert(this->pass != PassId::Invalid());

	PassNextSubpass(this->pass);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::BeginBatch(Frame::FrameBatchType::Code batchType)
{
    n_assert(this->inBeginPass);
    n_assert(!this->inBeginBatch);
	n_assert(this->pass != PassId::Invalid());

	this->inBeginBatch = true;
	PassBeginBatch(this->pass, batchType);
	//this->pass->BeginBatch(batchType);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndBatch()
{
    n_assert(this->inBeginBatch);
	n_assert(this->pass != PassId::Invalid());

    this->inBeginBatch = false;
	PassEndBatch(this->pass);
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::EndPass()
{
    n_assert(this->inBeginPass);
	n_assert(this->pass != PassId::Invalid());

	PassEnd(this->pass);
	this->pass = PassId::Invalid();
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
RenderDeviceBase::Compute(int dimX, int dimY, int dimZ)
{
    n_assert(!this->inBeginPass);
    // override in subclass!
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
RenderDeviceBase::Copy(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion)
{
	n_assert(from == Ids::InvalidId32 && to == Ids::InvalidId32);
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Blit(const CoreGraphics::RenderTextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::RenderTextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from == Ids::InvalidId32 && to == Ids::InvalidId32);
	
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
RenderDeviceBase::Blit(const CoreGraphics::TextureId from, Math::rectangle<SizeT> fromRegion, IndexT fromMip, const CoreGraphics::TextureId to, Math::rectangle<SizeT> toRegion, IndexT toMip)
{
	n_assert(from == Ids::InvalidId32 && to == Ids::InvalidId32);

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
