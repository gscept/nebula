//------------------------------------------------------------------------------
//  ogl4rendertargetcube.cc
//  (C) 2013 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4rendertargetcube.h"
#include "ogl4types.h"
#include "resources/resourcemanager.h"
#include "coregraphics/texture.h"
#include "coregraphics/renderdevice.h"

using namespace CoreGraphics;
using namespace Resources;
namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4RenderTargetCube, 'O4RC', Base::RenderTargetCubeBase);

//------------------------------------------------------------------------------
/**
*/
OGL4RenderTargetCube::OGL4RenderTargetCube() :
    ogl4ResolveTexture(0),
    ogl4DepthStencilTexture(0),
    ogl4ColorBufferFormat(GL_RGBA8),
    ogl4Framebuffer(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4RenderTargetCube::~OGL4RenderTargetCube()
{
    n_assert(!this->isValid);
    n_assert(0 == this->ogl4ResolveTexture);
	n_assert(0 == this->ogl4Framebuffer);
	n_assert(0 == this->ogl4DepthStencilTexture);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::Setup()
{
    n_assert(0 == this->ogl4ResolveTexture);

    // call parent class
    RenderTargetCubeBase::Setup();

    // setup our pixel format and multisample parameters (order important!)
    this->ogl4ColorBufferComponents = OGL4Types::AsOGL4PixelComponents(this->colorBufferFormat);
    this->ogl4ColorBufferFormat = OGL4Types::AsOGL4PixelFormat(this->colorBufferFormat);
    this->ogl4ColorBufferType = OGL4Types::AsOGL4PixelType(this->colorBufferFormat);

    // generate render target texture
    glGenTextures(1, &this->ogl4ResolveTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->ogl4ResolveTexture);
    IndexT i;
    for (i = 0; i < 6; i++)
    {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X  + i, 
            0, 
            this->ogl4ColorBufferFormat, 
            this->width, 
            this->height, 
            0, 
            this->ogl4ColorBufferComponents,
            this->ogl4ColorBufferType,
            NULL);
        n_assert(GLSUCCESS);        
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    this->resolveRect.left = 0;
    this->resolveRect.top = 0;
    this->resolveRect.right = this->width;
    this->resolveRect.bottom = this->height;

    // if a resolve texture exists, create a shared texture resource, so that
    // the texture is publicly visible
    if (this->resolveTextureResId.IsValid())
    {
        this->resolveTexture = ResourceManager::Instance()->CreateUnmanagedResource(this->resolveTextureResId, Texture::RTTI).downcast<Texture>();

        // setup texture
        this->resolveTexture->SetupFromOGL4CubeTexture(this->ogl4ResolveTexture, this->colorBufferFormat, 0, true, true);
    }

    glGenFramebuffers(1, &this->ogl4Framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
	if (this->layered)
	{
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->ogl4ResolveTexture, 0);
	}
	else
	{
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, this->ogl4ResolveTexture, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, this->ogl4ResolveTexture, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, this->ogl4ResolveTexture, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, this->ogl4ResolveTexture, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, this->ogl4ResolveTexture, 0);
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, this->ogl4ResolveTexture, 0);
	}
	
    if (this->useDepthStencilCube)
    {
		if (this->layered)
		{
			glGenTextures(1, &this->ogl4DepthStencilTexture);
			glBindTexture(GL_TEXTURE_CUBE_MAP, this->ogl4DepthStencilTexture);
			IndexT i;
			for (i = 0; i < 6; i++)
			{
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0,
					GL_DEPTH_COMPONENT24,
					this->width,
					this->height,
					0,
					GL_DEPTH_COMPONENT,
					GL_FLOAT,
					NULL);
				n_assert(GLSUCCESS);
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, this->ogl4ResolveTexture, 0);
		}
		else
		{
			glGenRenderbuffers(1, &this->ogl4DepthStencilTexture);
			glBindRenderbuffer(GL_RENDERBUFFER, this->ogl4DepthStencilTexture);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->width, this->height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->ogl4DepthStencilTexture);
		}
    }
    n_assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	n_assert(GLSUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::Discard()
{
    RenderTargetCubeBase::Discard();
	n_assert(GLSUCCESS);
    if (this->ogl4ResolveTexture)
    {
        glDeleteTextures(1, &this->ogl4ResolveTexture);
    }	
    this->ogl4ResolveTexture = 0;

    if (this->ogl4DepthStencilTexture)
    {
		glDeleteRenderbuffers(1, &this->ogl4DepthStencilTexture);
    }	
    this->ogl4DepthStencilTexture = 0;

    if (this->ogl4Framebuffer)
    {
        glDeleteFramebuffers(1, &this->ogl4Framebuffer);
    }	
    this->ogl4Framebuffer = 0;	
	n_assert(GLSUCCESS);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::BeginPass()
{
    // call base function
    RenderTargetCubeBase::BeginPass();

    // clear rendertarget
    this->Clear(this->clearFlags);

    // if we draw to -1, we use the zeroth layered cube map
	if (this->layered)
	{
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}
	else
	{
		// otherwise we use the per-cube map selection method
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + this->currentDrawFace);
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::EndPass()
{
    RenderTargetCubeBase::EndPass();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::GenerateMipLevels()
{
    n_assert(0 != this->ogl4ResolveTexture);
    n_assert(this->mipMapsEnabled);

    this->resolveTexture->GenerateMipmaps();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTargetCube::Clear( uint flags )
{
    // bind framebuffer if not currently bound
    if (!this->inBeginPass)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
    }

    if (0 != (flags & ClearColor))
    {
        n_assert(0 != this->ogl4ResolveTexture);
        glClearBufferfv(GL_COLOR, 0, (GLfloat*)&this->clearColor);	
    }
}
} // namespace OpenGL4