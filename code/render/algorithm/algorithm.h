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
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/stringatom.h"
#include "coregraphics/rendertexture.h"
#include "coregraphics/shaderreadwritebuffer.h"
#include "coregraphics/shaderreadwritetexture.h"
#include <functional>
namespace Algorithms
{
class Algorithm : public Core::RefCounted
{
	__DeclareClass(Algorithm);
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
	/// get type of function name
	const FunctionType& GetFunctionType(const Util::StringAtom& str);
	/// get function
	const std::function<void(IndexT)> GetFunction(const Util::StringAtom& str);

	/// add texture
	void AddRenderTexture(const CoreGraphics::RenderTextureId tex);
	/// add buffer
	void AddReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf);
	/// add read-write texture (image)
	void AddReadWriteImage(const Ptr<CoreGraphics::ShaderReadWriteTexture>& img);

protected:
	/// add algorithm
	void AddFunction(const Util::StringAtom& name, const FunctionType type, const std::function<void(IndexT)>& func);

	Util::Array<CoreGraphics::RenderTextureId> renderTextures;
	Util::Array<Ptr<CoreGraphics::ShaderReadWriteBuffer>> readWriteBuffers;
	Util::Array<Ptr<CoreGraphics::ShaderReadWriteTexture>> readWriteTextures;
	Util::Dictionary<Util::StringAtom, std::function<void(IndexT)>> functions;
	Util::Dictionary<Util::StringAtom, FunctionType> nameToType;
};

//------------------------------------------------------------------------------
/**
*/
inline const Algorithms::Algorithm::FunctionType&
Algorithm::GetFunctionType(const Util::StringAtom& str)
{
	return this->nameToType[str];
}

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddRenderTexture(const CoreGraphics::RenderTextureId tex)
{
	this->renderTextures.Append(tex);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddReadWriteBuffer(const Ptr<CoreGraphics::ShaderReadWriteBuffer>& buf)
{
	this->readWriteBuffers.Append(buf);
}

//------------------------------------------------------------------------------
/**
*/
inline void
Algorithm::AddReadWriteImage(const Ptr<CoreGraphics::ShaderReadWriteTexture>& img)
{
	this->readWriteTextures.Append(img);
}
} // namespace Base