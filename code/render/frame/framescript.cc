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
	frameOpCounter(0),
	subScript(false)
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
		this->compiled[i]->QueuePreSync();			// wait within queue
		this->compiled[i]->Run(frameIndex);
		this->compiled[i]->QueuePostSync();			// signal within queue
	}
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

	for (i = 0; i < this->resourceResetBarriers.Size(); i++)
		DestroyBarrier(this->resourceResetBarriers[i]);
	this->resourceResetBarriers.Clear();

	for (i = 0; i < this->events.Size(); i++)
		DestroyEvent(this->events[i]);

	for (i = 0; i < this->barriers.Size(); i++)
		DestroyBarrier(this->barriers[i]);

	// clear old compiled result
	this->buildAllocator.Release();
	 
	Util::Dictionary<CoreGraphics::ShaderRWTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>> rwTextures;
	Util::Dictionary<CoreGraphics::ShaderRWBufferId, FrameOp::BufferDependency> rwBuffers;
	Util::Dictionary<CoreGraphics::RenderTextureId, Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>> renderTextures;

	// setup initial layout for resources
	for (i = 0; i < this->colorTextures.Size(); i++)
	{
		auto tex = this->colorTextures[i];
		CoreGraphicsImageLayout layout = CoreGraphics::RenderTextureGetLayout(tex);
		CoreGraphics::ImageSubresourceInfo subres;
		subres.aspect = CoreGraphicsImageAspect::ColorBits;
		subres.layer = 0;
		subres.layerCount = 1;
		subres.mip = 0;
		subres.mipCount = CoreGraphics::RenderTextureGetNumMips(tex);
		auto& arr = renderTextures.AddUnique(tex);
		arr.Append(std::make_tuple(subres, FrameOp::TextureDependency{ nullptr, CoreGraphicsQueueType::GraphicsQueueType, layout, CoreGraphics::BarrierStage::AllGraphicsShaders | CoreGraphics::BarrierStage::ComputeShader, CoreGraphics::BarrierAccess::ShaderRead, DependencyIntent::Read, 0 }));
	}

	for (i = 0; i < this->depthStencilTextures.Size(); i++)
	{
		auto tex = this->depthStencilTextures[i];
		CoreGraphicsImageLayout layout = CoreGraphics::RenderTextureGetLayout(tex);
		CoreGraphics::ImageSubresourceInfo subres;
		subres.aspect = CoreGraphicsImageAspect::DepthBits | CoreGraphicsImageAspect::StencilBits;
		subres.layer = 0;
		subres.layerCount = 1;
		subres.mip = 0;
		subres.mipCount = CoreGraphics::RenderTextureGetNumMips(tex);
		auto& arr = renderTextures.AddUnique(tex);
		arr.Append(std::make_tuple(subres, FrameOp::TextureDependency{ nullptr, CoreGraphicsQueueType::GraphicsQueueType, layout, CoreGraphics::BarrierStage::AllGraphicsShaders, CoreGraphics::BarrierAccess::DepthAttachmentRead, DependencyIntent::Read, 0 }));
	}

	for (i = 0; i < this->readWriteTextures.Size(); i++)
	{
		auto tex = this->readWriteTextures[i];
		CoreGraphicsImageLayout layout = CoreGraphics::ShaderRWTextureGetLayout(tex);
		CoreGraphics::TextureDimensions dims = CoreGraphics::ShaderRWTextureGetDimensions(tex);
		CoreGraphics::ImageSubresourceInfo subres;
		subres.aspect = CoreGraphicsImageAspect::ColorBits;
		subres.layer = 0;
		subres.layerCount = dims.depth;
		subres.mip = 0;
		subres.mipCount = CoreGraphics::ShaderRWTextureGetNumMips(tex);
		auto& arr = rwTextures.AddUnique(tex);
		arr.Append(std::make_tuple(subres, FrameOp::TextureDependency{ nullptr, CoreGraphicsQueueType::GraphicsQueueType, layout, CoreGraphics::BarrierStage::ComputeShader, CoreGraphics::BarrierAccess::ShaderRead, DependencyIntent::Read, 0 }));
	}

	for (i = 0; i < this->ops.Size(); i++)
	{
		this->ops[i]->Build(this->buildAllocator, this->compiled, this->events, this->barriers, rwTextures, rwBuffers, renderTextures);
	}

	// setup a post-frame barrier to reset the resource state of all resources back to their created original (ShaderRead for RenderTexture, General for RWTexture
	Util::Array<CoreGraphics::RWTextureBarrier> shaderRWTexturesBarr;
	Util::Array<CoreGraphics::RenderTextureBarrier> renderTexturesBarr;

	for (i = 0; i < rwTextures.Size(); i++)
	{
		const CoreGraphics::ShaderRWTextureId& res = rwTextures.KeyAtIndex(i);
		CoreGraphicsImageLayout layout = CoreGraphics::ShaderRWTextureGetLayout(res);
		const Util::Array<std::tuple<CoreGraphics::ImageSubresourceInfo, FrameOp::TextureDependency>>& deps = rwTextures.ValueAtIndex(i);
		for (IndexT j = 0; j < deps.Size(); j++)
		{
			const CoreGraphics::ImageSubresourceInfo& info = std::get<0>(deps[j]);
			const FrameOp::TextureDependency& dep = std::get<1>(deps[j]);
			CoreGraphics::BarrierAccess outAccess = layout == CoreGraphicsImageLayout::Present ? CoreGraphics::BarrierAccess::TransferRead : CoreGraphics::BarrierAccess::ShaderRead;
			CoreGraphics::BarrierStage outStage = outAccess == CoreGraphics::BarrierAccess::TransferRead ? CoreGraphics::BarrierStage::Transfer : CoreGraphics::BarrierStage::AllGraphicsShaders;

			// rw textures are created with general
			if (dep.layout != layout)
			{
				CoreGraphics::BarrierCreateInfo inf =
				{
					Util::String::Sprintf("End of Frame Resource RWTexture Transition %d", res.id24),
					CoreGraphics::BarrierDomain::Global,
					dep.stage,
					outStage,
					nullptr, nullptr, { CoreGraphics::RWTextureBarrier{ res, info, dep.layout, layout, dep.access, outAccess } }
				};
				this->resourceResetBarriers.Append(CoreGraphics::CreateBarrier(inf));
			}
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
			CoreGraphics::BarrierAccess outAccess = layout == CoreGraphicsImageLayout::Present ? CoreGraphics::BarrierAccess::TransferRead : CoreGraphics::BarrierAccess::ShaderRead;
			CoreGraphics::BarrierStage outStage = outAccess == CoreGraphics::BarrierAccess::TransferRead ? CoreGraphics::BarrierStage::Transfer : CoreGraphics::BarrierStage::AllGraphicsShaders;

			// render textures are created as shader read
			if (dep.layout != layout)
			{
				CoreGraphics::BarrierCreateInfo inf =
				{
					Util::String::Sprintf("End of Frame RenderTexture Reset Transition %d", res.id24),
					CoreGraphics::BarrierDomain::Global,
					dep.stage,
					outStage,
					{ CoreGraphics::RenderTextureBarrier{ res, info, dep.layout, layout, dep.access, outAccess } }, nullptr, nullptr
				};
				this->resourceResetBarriers.Append(CoreGraphics::CreateBarrier(inf));
			}
		}
	}
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