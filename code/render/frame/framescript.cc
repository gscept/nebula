//------------------------------------------------------------------------------
// framescript.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescript.h"
#include "frameserver.h"
#include "coregraphics/displaydevice.h"

namespace Frame
{

__ImplementClass(Frame::FrameScript, 'FRSC', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
FrameScript::FrameScript() :
	window(Ids::InvalidId32),
	endOfFrameBarrier(CoreGraphics::BarrierId::Invalid()),
	frameOpCounter(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FrameScript::~FrameScript()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddColorTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex)
{
	n_assert(!this->colorTexturesByName.Contains(name));
	this->colorTexturesByName.Add(name, tex);
	this->colorTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddDepthStencilTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex)
{
	n_assert(!this->depthStencilTexturesByName.Contains(name));
	this->depthStencilTexturesByName.Add(name, tex);
	this->depthStencilTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddReadWriteTexture(const Util::StringAtom& name, const CoreGraphics::ShaderRWTextureId tex)
{
	n_assert(!this->readWriteTexturesByName.Contains(name));
	this->readWriteTexturesByName.Add(name, tex);
	this->readWriteTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId buf)
{
	n_assert(!this->readWriteBuffersByName.Contains(name));
	this->readWriteBuffersByName.Add(name, buf);
	this->readWriteBuffers.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddAlgorithm(const Util::StringAtom& name, Algorithms::Algorithm* alg)
{
	n_assert(!this->algorithmsByName.Contains(name));
	this->algorithmsByName.Add(name, alg);
	this->algorithms.Append(alg);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::AddOp(Frame::FrameOp* op)
{
	op->index = this->frameOpCounter;
	this->frameOpCounter++;
	this->ops.Append(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Setup()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Discard()
{
	// unload ourselves, this is only for convenience
	FrameServer::Instance()->UnloadFrameScript(this->resId);

	IndexT i;
	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Discard();
		this->ops[i]->~FrameOp();
	}

	this->buildAllocator.Release();
	this->allocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Run(const IndexT frameIndex)
{
	
	IndexT i;
	for (i = 0; i < this->compiled.Size(); i++)
	{
		this->compiled[i]->CrossQueuePreSync();		// wait cross-queue
		this->compiled[i]->QueuePreSync();			// wait within queue
		this->compiled[i]->Run(frameIndex);
		this->compiled[i]->QueuePostSync();			// signal within queue
		this->compiled[i]->CrossQueuePostSync();	// signal cross-queue
	}

	// make sure to transition resources back to their original state in preparation for the next frame
	CoreGraphics::BarrierReset(this->endOfFrameBarrier);
	CoreGraphics::BarrierInsert(this->endOfFrameBarrier, CoreGraphicsQueueType::GraphicsQueueType);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Build()
{
	// go through and discard all compiled (dunno if needed)
	IndexT i;
	for (i = 0; i < this->compiled.Size(); i++)
	{
		this->compiled[i]->Discard();
	}

	for (i = 0; i < this->events.Size(); i++)
		DestroyEvent(this->events[i]);
	for (i = 0; i < this->barriers.Size(); i++)
		DestroyBarrier(this->barriers[i]);
	for (i = 0; i < this->semaphores.Size(); i++)
		DestroySemaphore(this->semaphores[i]);

	// clear old compiled result
	this->buildAllocator.Release();
	 
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>> rwTextures;
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, FrameOp::BufferDependency> rwBuffers;
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>> renderTextures;

	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Build(this->buildAllocator, this->compiled, this->events, this->barriers, this->semaphores, rwTextures, rwBuffers, renderTextures);
	}

	// setup a post-frame barrier to reset the resource state of all resources back to their created original (ShaderRead for RenderTexture, General for RWTexture
	Util::Array<std::tuple<CoreGraphics::RenderTextureId, CoreGraphics::ImageSubresourceInfo, CoreGraphicsImageLayout, CoreGraphicsImageLayout, CoreGraphics::BarrierAccess, CoreGraphics::BarrierAccess>> renderTexturesBarr;
	Util::Array<std::tuple<CoreGraphics::ShaderRWTextureId, CoreGraphics::ImageSubresourceInfo, CoreGraphicsImageLayout, CoreGraphicsImageLayout, CoreGraphics::BarrierAccess, CoreGraphics::BarrierAccess>> shaderRWTexturesBarr;

	for (i = 0; i < rwTextures.Size(); i++)
	{
		const CoreGraphics::ShaderRWTextureId& res = rwTextures.KeyAtIndex(i);
		CoreGraphicsImageLayout layout = CoreGraphics::ShaderRWTextureGetLayout(res);
		const Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>& deps = rwTextures.ValueAtIndex(i);
		for (IndexT j = 0; j < deps.Size(); j++)
		{
			const CoreGraphics::ImageSubresourceInfo& info = std::get<0>(deps[j]);
			const FrameOp::TextureDependency& dep = std::get<1>(deps[j]);

			// rw textures are created with general
			if (dep.layout != layout)
				shaderRWTexturesBarr.Append(std::make_tuple(res, info, dep.layout, layout, CoreGraphics::BarrierAccess::NoAccess, CoreGraphics::BarrierAccess::NoAccess));
		}
	}

	for (i = 0; i < renderTextures.Size(); i++)
	{
		const CoreGraphics::RenderTextureId& res = renderTextures.KeyAtIndex(i);
		CoreGraphicsImageLayout layout = CoreGraphics::RenderTextureGetLayout(res);
		const Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>& deps = renderTextures.ValueAtIndex(i);
		for (IndexT j = 0; j < deps.Size(); j++)
		{
			const CoreGraphics::ImageSubresourceInfo& info = std::get<0>(deps[j]);
			const FrameOp::TextureDependency& dep = std::get<1>(deps[j]);

			// render textures are created as shader read
			if (dep.layout != layout)
				renderTexturesBarr.Append(std::make_tuple(res, info, dep.layout, layout, CoreGraphics::BarrierAccess::NoAccess, CoreGraphics::BarrierAccess::NoAccess));
		}
	}

	// buffers need not be transitioned
	Util::Array<std::tuple<CoreGraphics::ShaderRWBufferId, CoreGraphics::BarrierAccess, CoreGraphics::BarrierAccess>> shaderRWBuffersBarr;
	CoreGraphics::BarrierCreateInfo info =
	{
		CoreGraphics::BarrierDomain::Global,
		CoreGraphics::BarrierStage::Bottom,
		CoreGraphics::BarrierStage::Bottom,
		renderTexturesBarr, shaderRWBuffersBarr, shaderRWTexturesBarr
	};
	if (this->endOfFrameBarrier != CoreGraphics::BarrierId::Invalid())
		CoreGraphics::DestroyBarrier(this->endOfFrameBarrier);
	this->endOfFrameBarrier = CoreGraphics::CreateBarrier(info);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::Cleanup()
{
	IndexT i;
	for (i = 0; i < this->colorTextures.Size(); i++) DestroyRenderTexture(this->colorTextures[i]);
	this->colorTextures.Clear();
	this->colorTexturesByName.Clear();

	for (i = 0; i < this->depthStencilTextures.Size(); i++) DestroyRenderTexture(this->depthStencilTextures[i]);
	this->depthStencilTextures.Clear();
	this->depthStencilTexturesByName.Clear();

	for (i = 0; i < this->readWriteTextures.Size(); i++) DestroyShaderRWTexture(this->readWriteTextures[i]);
	this->readWriteTextures.Clear();
	this->readWriteTexturesByName.Clear();

	for (i = 0; i < this->readWriteBuffers.Size(); i++) DestroyShaderRWBuffer(this->readWriteBuffers[i]);
	this->readWriteBuffers.Clear();
	this->readWriteBuffersByName.Clear();

	for (i = 0; i < this->events.Size(); i++) DestroyEvent(this->events[i]);
	this->events.Clear();

	for (i = 0; i < this->algorithms.Size(); i++) this->algorithms[i]->Discard();
	this->algorithms.Clear();
	this->algorithmsByName.Clear();

	for (i = 0; i < this->ops.Size(); i++) this->ops[i]->Discard();
	this->ops.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScript::OnWindowResized()
{
	// only do this if we actually use the window
	if (this->window != Ids::InvalidId32)
	{
		CoreGraphics::WindowId prev = CoreGraphics::DisplayDevice::Instance()->GetCurrentWindow();

		// make this window current
		WindowMakeCurrent(this->window);

		IndexT i;
		for (i = 0; i < this->colorTextures.Size(); i++)		RenderTextureWindowResized(this->colorTextures[i]);
		for (i = 0; i < this->readWriteTextures.Size(); i++)	ShaderRWTextureWindowResized(this->readWriteTextures[i]);
		for (i = 0; i < this->algorithms.Size(); i++)			this->algorithms[i]->Resize();
		for (i = 0; i < this->ops.Size(); i++)					this->ops[i]->OnWindowResized();

		// reset old window
		WindowMakeCurrent(prev);
	}

}

} // namespace Frame2