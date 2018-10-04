#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4RenderDevice
  
    OGL4 implementation of RenderTarget.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/base/rendertargetbase.h"
#include "coregraphics/shadervariable.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4RenderTarget : public Base::RenderTargetBase
{
    __DeclareClass(OGL4RenderTarget);
public:

	struct Viewport
	{
		GLint x;
		GLint y;
		GLsizei width;
		GLsizei height;
	};

    /// constructor
    OGL4RenderTarget();
    /// destructor
    virtual ~OGL4RenderTarget();
    
    /// setup the render target object
    void Setup();
    /// discard the render target object
    void Discard();
    /// begin a render pass
    void BeginPass();
    /// end current render pass
    void EndPass();
    /// generate mipmap levels
    void GenerateMipLevels();

    /// reload render target
    void Reload();

	/// called after we change the display size
	void OnWindowResized(SizeT width, SizeT height);

	/// return viewport
	const Viewport& GetViewport() const;
	
	/// get the OpenGL4 rendertarget texture
	const GLuint& GetTexture() const;
	/// get OpenGL4 framebuffer
	const GLuint& GetFramebuffer() const;

    /// copy from this opengl render target to an opengl texture
	void Copy(const Ptr<CoreGraphics::RenderTarget>& rt);

	/// clears render target by force
	void Clear(uint flags);

protected:
    friend class OGL4RenderDevice;

    /// setup compatible multisample type
    void SetupMultiSampleType();
                                      
	Ptr<CoreGraphics::ShaderVariable> textureRatio;
    Ptr<CoreGraphics::ShaderVariable> sharedPixelSize; 
    Ptr<CoreGraphics::ShaderVariable> sharedHalfPixelSize;

	GLuint ogl4ResolveTexture;
	GLenum ogl4ColorBufferFormat;
	GLenum ogl4ColorBufferComponents;
	GLenum ogl4ColorBufferType;

	GLuint ogl4Framebuffer;

	GLuint msCount;
	GLuint msQuality;
    bool needsResolve;
	bool clear;
};

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4RenderTarget::GetTexture() const
{
	return this->ogl4ResolveTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4RenderTarget::GetFramebuffer() const
{
	return this->ogl4Framebuffer;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------
