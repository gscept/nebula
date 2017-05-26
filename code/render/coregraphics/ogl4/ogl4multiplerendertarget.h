#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4MultipleRenderTarget
    
    Implements an OpenGL4 MRT.
    
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/multiplerendertargetbase.h"
namespace OpenGL4
{
class OGL4MultipleRenderTarget : public Base::MultipleRenderTargetBase
{
	__DeclareClass(OGL4MultipleRenderTarget);
public:
	/// constructor
	OGL4MultipleRenderTarget();
	/// destructor
	virtual ~OGL4MultipleRenderTarget();

	/// add render target
	void AddRenderTarget(const Ptr<CoreGraphics::RenderTarget>& rt);
	/// set depth-stencil target
	void SetDepthStencilTarget(const Ptr<CoreGraphics::DepthStencilTarget>& dt);
	/// begin pass
	void BeginPass();
	/// begin a batch
	void BeginBatch(CoreGraphics::FrameBatchType::Code batchType);
	/// end current batch
	void EndBatch();
	/// end current render pass
	void EndPass(); 

	/// called after we change the display size
	void OnDisplayResized(SizeT width, SizeT height);

	/// get OpenGL4 framebuffer handle
	const GLuint& GetFramebuffer() const;
private:

	GLuint ogl4Framebuffer;
}; 


//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4MultipleRenderTarget::GetFramebuffer() const
{
	return this->ogl4Framebuffer;
}


} // namespace OpenGL4
//------------------------------------------------------------------------------