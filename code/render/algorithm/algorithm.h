#pragma once
//------------------------------------------------------------------------------
/**
	An algorithm is a frame script triggered piece of code which can run
	in sequences.

	An algorithm can be purely compute, in which case they may not execute within 
	a pass. They can be purely render, which means they must execute within
	a pass. They can also be mixed, in which case the right function has to 
	be executed within the correct scope. 

	An actual algorithm inherits this class and statically binds StringAtom objects
	with functions, and registers them with a certain type, which can be retrieved
	when performing frame script validation. 
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderrwbuffer.h"
#include "coregraphics/shaderrwtexture.h"
#include <functional>
namespace Algorithms
{
class Algorithm
{
public:
	enum FunctionType
	{
		None,			// does neither rendering nor compute
		Graphics,			// algorithm does rendering only
		Compute			// algorithm does only computations
	};

	/// constructor
	Algorithm();
	/// destructor
	virtual ~Algorithm();
	
	/// setup algorithm
	virtual void Setup();
	/// discard operation
	virtual void Discard();
	/// handle window resizing
	void Resize();

	/// add function callback to global dictionary
	static void AddCallback(const Util::StringAtom name, std::function<void(IndexT)> func);
	/// get algorithm function call
	static const std::function<void(IndexT)>& GetAlgorithmCallback(const Util::StringAtom& str);

	/// add texture
	void AddRenderTexture(const CoreGraphics::RenderTextureId& tex);
	/// add buffer
	void AddReadWriteBuffer(const CoreGraphics::ShaderRWBufferId& buf);
	/// add read-write texture (image)
	void AddReadWriteImage(const CoreGraphics::ShaderRWTextureId& img);

protected:

	Util::Array<CoreGraphics::RenderTextureId> renderTextures;
	Util::Array<CoreGraphics::ShaderRWBufferId> readWriteBuffers;
	Util::Array<CoreGraphics::ShaderRWTextureId> readWriteTextures;

	static Util::Dictionary<Util::StringAtom, std::function<void(IndexT)>> nameToFunction;
};

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddRenderTexture(const CoreGraphics::RenderTextureId& tex)
{
	this->renderTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddReadWriteBuffer(const CoreGraphics::ShaderRWBufferId& buf)
{
	this->readWriteBuffers.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddReadWriteImage(const CoreGraphics::ShaderRWTextureId& img)
{
	this->readWriteTextures.Append(img);
}
} // namespace Base