#pragma once
//------------------------------------------------------------------------------
/**
	A FrameScript describes render operations being done to produce a single frame.

	Frame scripts are loaded once like a template, and then compiled to produce
	an optimized result. When a pass is disabled or re-enabled, the script
	is rebuilt, so refrain from doing this frequently. 

	On DX12 and Vulkan, the compile process serves to insert proper barriers, events
	and semaphore operations such that shader resources are not stomped or read prematurely.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderrwtexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/event.h"
#include "coregraphics/shader.h"
#include "algorithm/algorithm.h"
#include "frame/frameop.h"
#include "frame/framepass.h"
#include "memory/chunkallocator.h"
namespace Graphics
{
	class View;
}
namespace Frame
{
class FrameScript : public Core::RefCounted
{
	__DeclareClass(FrameScript);
public:
	/// constructor
	FrameScript();
	/// destructor
	virtual ~FrameScript();

	/// get allocator
	Memory::ChunkAllocator<0xFFFF>& GetAllocator();

	/// set name
	void SetResourceName(const Resources::ResourceName& name);
	/// get name
	const Resources::ResourceName& GetResourceName() const;

	/// add frame operation
	void AddOp(Frame::FrameOp* op);
	/// add color texture
	void AddColorTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex);
	/// get color texture
	const CoreGraphics::RenderTextureId GetColorTexture(const Util::StringAtom& name);
	/// add depth-stencil texture
	void AddDepthStencilTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex);
	/// get depth-stencil texture
	const CoreGraphics::RenderTextureId GetDepthStencilTexture(const Util::StringAtom& name);
	/// add read-write texture
	void AddReadWriteTexture(const Util::StringAtom& name, const CoreGraphics::ShaderRWTextureId tex);
	/// get read-write texture
	const CoreGraphics::ShaderRWTextureId GetReadWriteTexture(const Util::StringAtom& name);
	/// add read-write buffer
	void AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId buf);
	/// get read-write buffer
	const CoreGraphics::ShaderRWBufferId GetReadWriteBuffer(const Util::StringAtom& name);
	/// add algorithm
	void AddAlgorithm(const Util::StringAtom& name, Algorithms::Algorithm* alg);
	/// get algorithm
	Algorithms::Algorithm* GetAlgorithm(const Util::StringAtom& name);

	/// setup script
	void Setup();
	/// discard script
	void Discard();
	/// run script
	void Run(const IndexT frameIndex);

	/// build framescript, this will delete and replace the old frame used for Run()
	void Build();

private:
	friend class FrameScriptLoader;
	friend class FrameServer;
	friend class Graphics::View;

	/// cleanup script internally
	void Cleanup();
	/// handle resizing
	void OnWindowResized();

	CoreGraphics::WindowId window;
	Memory::ChunkAllocator<0xFFFF> allocator;

	Util::Array<CoreGraphics::EventId> events;
	Util::Array<CoreGraphics::BarrierId> barriers;
	Util::Array<CoreGraphics::SemaphoreId> semaphores;
	Memory::ChunkAllocator<0xFFFF> buildAllocator;

	Resources::ResourceName resId;
	Util::Array<CoreGraphics::RenderTextureId> colorTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> colorTexturesByName;
	Util::Array<CoreGraphics::RenderTextureId> depthStencilTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> depthStencilTexturesByName;
	Util::Array<CoreGraphics::ShaderRWTextureId> readWriteTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWTextureId> readWriteTexturesByName;
	Util::Array<CoreGraphics::ShaderRWBufferId> readWriteBuffers;
	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWBufferId> readWriteBuffersByName;
	Util::Array<Frame::FrameOp*> ops;
	Util::Array<Frame::FrameOp::Compiled*> compiled;
	CoreGraphics::BarrierId endOfFrameBarrier;
	IndexT frameOpCounter;
	Util::Array<Algorithms::Algorithm*> algorithms;
	Util::Dictionary<Util::StringAtom, Algorithms::Algorithm*> algorithmsByName;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameScript::SetResourceName(const Resources::ResourceName& name)
{
	this->resId = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceName&
FrameScript::GetResourceName() const
{
	return this->resId;
}

//------------------------------------------------------------------------------
/**
*/
inline Memory::ChunkAllocator<0xFFFF>&
FrameScript::GetAllocator()
{
	return this->allocator;
}

//------------------------------------------------------------------------------
/**
*/
inline Algorithms::Algorithm*
FrameScript::GetAlgorithm(const Util::StringAtom& name)
{
	return this->algorithmsByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::RenderTextureId
FrameScript::GetColorTexture(const Util::StringAtom& name)
{
	IndexT i = this->colorTexturesByName.FindIndex(name);
	return i == InvalidIndex ? CoreGraphics::RenderTextureId::Invalid() : this->colorTexturesByName.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::RenderTextureId
FrameScript::GetDepthStencilTexture(const Util::StringAtom& name)
{
	IndexT i = this->depthStencilTexturesByName.FindIndex(name);
	return i == InvalidIndex ? CoreGraphics::RenderTextureId::Invalid() : this->colorTexturesByName.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderRWTextureId
FrameScript::GetReadWriteTexture(const Util::StringAtom& name)
{
	IndexT i = this->readWriteTexturesByName.FindIndex(name);
	return i == InvalidIndex ? CoreGraphics::ShaderRWTextureId::Invalid() : this->readWriteTexturesByName.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::ShaderRWBufferId
FrameScript::GetReadWriteBuffer(const Util::StringAtom& name)
{
	IndexT i = this->readWriteBuffersByName.FindIndex(name);
	return i == InvalidIndex ? CoreGraphics::ShaderRWBufferId::Invalid() : this->readWriteBuffersByName.ValueAtIndex(i);
}

} // namespace Frame2