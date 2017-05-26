#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4DepthStencilTarget
    
    Implements an OpenGL4 depth-stencil target.
    
    (C) 2013 gscept
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/depthstenciltargetbase.h"
namespace OpenGL4
{
class OGL4DepthStencilTarget : public Base::DepthStencilTargetBase
{
	__DeclareClass(OGL4DepthStencilTarget);
public:
	/// constructor
	OGL4DepthStencilTarget();
	/// destructor
	virtual ~OGL4DepthStencilTarget();

	/// setup depth-stencil target
	void Setup();
	/// discard depth-stencil target
	void Discard();

	/// called after we change the display size
	void OnWindowResized(SizeT width, SizeT height);

	/// begins pass
	void BeginPass();
	/// ends pass
	void EndPass();

	/// clear depth-stencil target
	void Clear(uint flags);

	/// get depth-stencil texture
	const GLuint& GetDepthStencilRenderbuffer() const;

private:
	GLuint ogl4DepthStencilRenderbuffer;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4DepthStencilTarget::GetDepthStencilRenderbuffer() const
{
	return this->ogl4DepthStencilRenderbuffer;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------