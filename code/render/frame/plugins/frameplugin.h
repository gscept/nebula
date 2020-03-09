#pragma once
//------------------------------------------------------------------------------
/**
	A plugin is a frame script triggered piece of code which can run
	in sequences.

	A plugin can be purely compute, in which case they may not execute within 
	a pass. They can be purely render, which means they must execute within
	a pass. They can also be mixed, in which case the right function has to 
	be executed within the correct scope. 

	An actual plugin inherits this class and statically binds StringAtom objects
	with functions, and registers them with a certain type, which can be retrieved
	when performing frame script validation. 
	
	(C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/texture.h"
#include "coregraphics/shaderrwbuffer.h"
#include <functional>
namespace Graphics
{
	class View;
}
namespace Frame
{
class FramePlugin
{
public:
	enum FunctionType
	{
		None,			// does neither rendering nor compute
		Graphics,		// algorithm does rendering only
		Compute			// algorithm does only computations
	};

	/// constructor
	FramePlugin();
	/// destructor
	virtual ~FramePlugin();
	
	/// setup algorithm
	virtual void Setup();
	/// discard operation
	virtual void Discard();

	/// update resources
	virtual void UpdateResources(const IndexT frameIndex);
	/// update view dependent resources
	virtual void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const IndexT frameIndex);
	/// handle window resizing
	virtual void Resize();

	/// add function callback to global dictionary
	static void AddCallback(const Util::StringAtom name, std::function<void(IndexT)> func);
	/// get algorithm function call
	static const std::function<void(IndexT)>& GetCallback(const Util::StringAtom& str);

	/// add buffer
	void AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId& buf);
	/// add texture
	void AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId& tex);

protected:

	Util::Dictionary<Util::StringAtom, CoreGraphics::ShaderRWBufferId> readWriteBuffers;
	Util::Dictionary<Util::StringAtom, CoreGraphics::TextureId> textures;

	static Util::Dictionary<Util::StringAtom, std::function<void(IndexT)>> nameToFunction;
};

//------------------------------------------------------------------------------
/**
*/
inline void
FramePlugin::AddReadWriteBuffer(const Util::StringAtom& name, const CoreGraphics::ShaderRWBufferId& buf)
{
	this->readWriteBuffers.Add(name, buf);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
FramePlugin::AddTexture(const Util::StringAtom& name, const CoreGraphics::TextureId& tex)
{
	this->textures.Add(name, tex);
}

} // namespace Base