#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4RenderTargetCube
    
    Implements an opengl cube map as a render target
    
    (C) 2013 Gustav Sterbrant
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/rendertargetcubebase.h"
namespace OpenGL4
{
class OGL4RenderTargetCube : public Base::RenderTargetCubeBase
{
	__DeclareClass(OGL4RenderTargetCube);
public:
	/// constructor
	OGL4RenderTargetCube();
	/// destructor
	virtual ~OGL4RenderTargetCube();

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

    /// get the OpenGL4 rendertarget cube texture
    const GLuint& GetOGL4Texture() const;
    /// get OpenGL4 depth-stencil cube texture
    const GLuint& GetOGL4DepthStencilTexture() const;
    /// get OpenGL4 framebuffer
    const GLuint& GetOGL4Framebuffer() const;

    /// clears render target by force
    void Clear(uint flags);

protected:
    friend class OGL4RenderDevice;

    GLuint ogl4ResolveTexture;
    GLuint ogl4DepthStencilTexture;
    GLenum ogl4ColorBufferFormat;
    GLenum ogl4ColorBufferComponents;
    GLenum ogl4ColorBufferType;
    GLuint ogl4Framebuffer;

}; 


//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4RenderTargetCube::GetOGL4Texture() const
{
    return this->ogl4ResolveTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4RenderTargetCube::GetOGL4DepthStencilTexture() const
{
    return this->ogl4DepthStencilTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline const GLuint& 
OGL4RenderTargetCube::GetOGL4Framebuffer() const
{
    return this->ogl4Framebuffer;
}

} // namespace OpenGL4
//------------------------------------------------------------------------------