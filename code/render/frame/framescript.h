#pragma once
//------------------------------------------------------------------------------
/**
	A FrameScript describes render operations being done to produce a single frame.

	Frame scripts can also execute a subset of commands, which is retrievable through 
	an execution mask.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderreadwritetexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "coregraphics/event.h"
#include "algorithm/algorithm.h"
#include "frame/frameop.h"
#include "frame/framepass.h"
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

	/// set name
	void SetResourceId(const Resources::ResourceId& name);
	/// get name
	const Resources::ResourceId& GetResourceId() const;

	/// add frame operation
	void AddOp(const Ptr<Frame::FrameOp>& op);
	/// add color texture
	void AddColorTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex);
	/// get color texture
	const CoreGraphics::RenderTextureId GetColorTexture(const Util::StringAtom& name);
	/// add depth-stencil texture
	void AddDepthStencilTexture(const Util::StringAtom& name, const CoreGraphics::RenderTextureId tex);
	/// get depth-stencil texture
	const CoreGraphics::RenderTextureId GetDepthStencilTexture(const Util::StringAtom& name);
	/// add read-write texture
	void AddReadWriteTexture(const Util::StringAtom& name, const Ptr<CoreGraphics::ShaderReadWriteTexture>& tex);
	/// get read-write texture
	const Ptr<CoreGraphics::ShaderReadWriteTexture>& GetReadWriteTexture(const Util::StringAtom& name);
	/// add read-write buffer
	void AddReadWriteBuffer(const Util::StringAtom& name, const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);
	/// get read-write buffer
	const Ptr<CoreGraphics::ShaderReadWriteBuffer>& GetReadWriteBuffer(const Util::StringAtom& name);
	/// add event
	void AddEvent(const Util::StringAtom& name, const Ptr<CoreGraphics::Event>& event);
	/// get event
	const Ptr<CoreGraphics::Event>& GetEvent(const Util::StringAtom& name);
	/// add algorithm
	void AddAlgorithm(const Util::StringAtom& name, const Ptr<Algorithms::Algorithm>& alg);
	/// get algorithm
	const Ptr<Algorithms::Algorithm>& GetAlgorithm(const Util::StringAtom& name);
	/// add shader state
	void AddShaderState(const Util::StringAtom& name, const Ptr<CoreGraphics::ShaderState>& state);
	/// get shader state
	const Ptr<CoreGraphics::ShaderState>& GetShaderState(const Util::StringAtom& name);

	/// setup script
	void Setup();
	/// discard script
	void Discard();
	/// run script
	void Run(const IndexT frameIndex);
	/// run segment of script
	void RunSegment(const FrameOp::ExecutionMask mask, const IndexT frameIndex);

	/// create an execution between two subpasses
	FrameOp::ExecutionMask CreateMask(const Util::StringAtom& startOp, const Util::StringAtom& endOp);
	/// create an execution mask running up to a pass, and then into a certain subpass
	FrameOp::ExecutionMask CreateSubpassMask(const Ptr<FramePass>& pass, const Util::StringAtom& passOp, const Util::StringAtom& subpass);
	/// get begin and end ops for mask
	void GetOps(const FrameOp::ExecutionMask mask, Ptr<FrameOp>& startOp, Ptr<FrameOp>& endOp);

private:
	friend class FrameScriptLoader;
	friend class FrameServer;
	friend class Graphics::View;

	/// cleanup script internally
	void Cleanup();
	/// handle resizing
	void OnWindowResized();

	CoreGraphics::WindowId window;

	Resources::ResourceId resId;
	Util::Array<CoreGraphics::RenderTextureId> colorTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> colorTexturesByName;
	Util::Array<CoreGraphics::RenderTextureId> depthStencilTextures;
	Util::Dictionary<Util::StringAtom, CoreGraphics::RenderTextureId> depthStencilTexturesByName;
	Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>> readWriteTextures;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderReadWriteTexture>> readWriteTexturesByName;
	Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>> readWriteBuffers;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderReadWriteBuffer>> readWriteBuffersByName;
	Util::Array<Ptr<CoreGraphics::Event>> events;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::Event>> eventsByName;
	Util::Array<Ptr<Frame::FrameOp>> ops;
	Util::Array<Ptr<Algorithms::Algorithm>> algorithms;
	Util::Dictionary<Util::StringAtom, Ptr<Algorithms::Algorithm>> algorithmsByName;
	Util::Array<Ptr<CoreGraphics::ShaderState>> shaderStates;
	Util::Dictionary<Util::StringAtom, Ptr<CoreGraphics::ShaderState>> shaderStatesByName;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FrameScript::SetResourceId(const Resources::ResourceId& name)
{
	this->resId = name;
}

//------------------------------------------------------------------------------
/**
*/
inline const Resources::ResourceId&
FrameScript::GetResourceId() const
{
	return this->resId;
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::RenderTextureId
FrameScript::GetColorTexture(const Util::StringAtom& name)
{
	return this->colorTexturesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const CoreGraphics::RenderTextureId
FrameScript::GetDepthStencilTexture(const Util::StringAtom& name)
{
	return this->depthStencilTexturesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderReadWriteTexture>&
FrameScript::GetReadWriteTexture(const Util::StringAtom& name)
{
	return this->readWriteTexturesByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderReadWriteBuffer>&
FrameScript::GetReadWriteBuffer(const Util::StringAtom& name)
{
	return this->readWriteBuffersByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::Event>&
FrameScript::GetEvent(const Util::StringAtom& name)
{
	return this->eventsByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Algorithms::Algorithm>&
FrameScript::GetAlgorithm(const Util::StringAtom& name)
{
	return this->algorithmsByName[name];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<CoreGraphics::ShaderState>&
FrameScript::GetShaderState(const Util::StringAtom& name)
{
	return this->shaderStatesByName[name];
}
} // namespace Frame2