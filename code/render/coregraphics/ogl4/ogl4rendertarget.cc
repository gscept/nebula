//------------------------------------------------------------------------------
//  OGL4RenderDevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/rendertarget.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "coregraphics/displaydevice.h"
#include "resources/resourcemanager.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/renderdevice.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4RenderTarget, 'D1RT', Base::RenderTargetBase);

using namespace CoreGraphics;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
OGL4RenderTarget::OGL4RenderTarget() :
    ogl4ResolveTexture(0),
    ogl4ColorBufferFormat(GL_RGBA8),
	ogl4Framebuffer(0),
    needsResolve(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4RenderTarget::~OGL4RenderTarget()
{
    n_assert(!this->isValid);
    n_assert(0 == this->ogl4ResolveTexture);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderTarget::Setup()
{
    n_assert(0 == this->ogl4ResolveTexture);
    
    // call parent class
    RenderTargetBase::Setup();

    // if we're the default render target, query display device
    // for setup parameters
    if (this->isDefaultRenderTarget)
    {
        // NOTE: the default render target will never be anti-aliased!
        // this assumes a render pipeline where the actual rendering goes
        // into an offscreen render target and is then resolved to the back buffer
        DisplayDevice* displayDevice = DisplayDevice::Instance();
		const CoreGraphics::DisplayMode& mode = displayDevice->GetCurrentWindow()->GetDisplayMode();
		this->SetWidth(mode.GetWidth());
		this->SetHeight(mode.GetHeight());
        this->SetAntiAliasQuality(AntiAliasQuality::None);
		this->SetColorBufferFormat(mode.GetPixelFormat());

		this->resolveRect.left = 0;
		this->resolveRect.top = 0;
		this->resolveRect.right = mode.GetWidth();
		this->resolveRect.bottom = mode.GetHeight();
		
		this->ogl4ResolveTexture = 0;
    }
	else if (this->relativeSizeValid)
	{
		DisplayDevice* displayDevice = DisplayDevice::Instance();
		const CoreGraphics::DisplayMode& mode = displayDevice->GetCurrentWindow()->GetDisplayMode();
		this->SetWidth(Math::n_max(1, SizeT(mode.GetWidth() * this->relWidth)));
		this->SetHeight(Math::n_max(1, SizeT(mode.GetHeight() * this->relHeight)));
	}

    // setup our pixel format and multisample parameters (order important!)
	this->ogl4ColorBufferComponents = OGL4Types::AsOGL4PixelComponents(this->colorBufferFormat);
	this->ogl4ColorBufferFormat = OGL4Types::AsOGL4PixelFormat(this->colorBufferFormat);
	this->ogl4ColorBufferType = OGL4Types::AsOGL4PixelType(this->colorBufferFormat);
    this->SetupMultiSampleType();

    // check if a resolve texture must be allocated
    if (this->mipMapsEnabled ||
        (0 != this->msCount) ||
        (this->resolveTextureDimensionsValid &&
         ((this->resolveTextureWidth != this->width) ||
          (this->resolveTextureHeight != this->height))))
    {
        this->needsResolve = true;
    }
    else
    {
        this->needsResolve = false;
    }

    // create the render target either as a texture, or as
    // a surface, or don't create it if rendering goes
    // into backbuffer
    if (!this->isDefaultRenderTarget)
    {
        SizeT resolveWidth = this->resolveTextureDimensionsValid ? this->resolveTextureWidth : this->width;
        SizeT resolveHeight = this->resolveTextureDimensionsValid ? this->resolveTextureHeight : this->height;

        glGenTextures(1, &this->ogl4ResolveTexture);
        n_assert(GLSUCCESS);

        if (this->msCount > 1)
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->ogl4ResolveTexture);
            glTexImage2DMultisample(
                GL_TEXTURE_2D_MULTISAMPLE, 
                this->msCount, 
                this->ogl4ColorBufferFormat, 
                resolveWidth, 
                resolveHeight, 
                GL_FALSE);
            n_assert(GLSUCCESS);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, this->ogl4ResolveTexture);
            glTexImage2D(
                GL_TEXTURE_2D, 
                0, 
                this->ogl4ColorBufferFormat, 
                resolveWidth, 
                resolveHeight, 
                0, 
                this->ogl4ColorBufferComponents,
                this->ogl4ColorBufferType,
                NULL);
            n_assert(GLSUCCESS);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        this->resolveRect.left = 0;
        this->resolveRect.top = 0;
        this->resolveRect.right = resolveWidth;
        this->resolveRect.bottom = resolveHeight;

        // setup frame buffer
        glGenFramebuffers(1, &this->ogl4Framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->ogl4ResolveTexture, 0);
        if (this->depthStencilTarget.isvalid())
        {
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->depthStencilTarget->GetDepthStencilRenderbuffer());
        }
		
		// ensure framebuffer is valid, then unbind it
		n_assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        // setup texture resource
        if (this->resolveTextureResId.IsValid())
        {
            this->resolveTexture = ResourceManager::Instance()->CreateUnmanagedResource(this->resolveTextureResId, Texture::RTTI).downcast<Texture>();
        }
        else
        {
            // just create a texture natively without managing it
            this->resolveTexture = CoreGraphics::Texture::Create();
        }

        // setup actual texture
        if (this->msCount > 1)
        {
            this->resolveTexture->SetupFromOGL4MultisampleTexture(this->ogl4ResolveTexture, this->colorBufferFormat, 0, true, true);
        }
        else
        {
			this->resolveTexture->SetupFromOGL4Texture(this->ogl4ResolveTexture, this->colorBufferFormat, 0, true, true);
        }
    }
    else
    {
        // NOTE: if we are the default render target and not antialiased, 
        // rendering will go directly to the backbuffer, so there's no
        // need to allocate a render target
    }
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderTarget::Discard()
{
    RenderTargetBase::Discard();
	if (this->ogl4ResolveTexture)
	{
		glDeleteTextures(1, &this->ogl4ResolveTexture);
	}	
	this->ogl4ResolveTexture = 0;

	if (this->ogl4Framebuffer)
	{
		glDeleteFramebuffers(1, &this->ogl4Framebuffer);
	}	
	this->ogl4Framebuffer = 0;	

    this->sharedPixelSize = 0;
    this->sharedHalfPixelSize = 0;
}

//------------------------------------------------------------------------------
/**
    Select the antialias parameters that most closely resembly 
    the preferred settings in the DisplayDevice object.
*/
void
OGL4RenderTarget::SetupMultiSampleType()
{
    n_assert(0 != this->ogl4ColorBufferFormat);
    OGL4RenderDevice* renderDevice = OGL4RenderDevice::Instance();

    #if NEBULA_DIRECT3D_DEBUG
        this->msCount = 0;
        this->msQuality = 0;
    #else
        // convert Nebula antialias quality into D3D type
        this->msCount = OGL4Types::AsOGL4MultiSampleType(this->antiAliasQuality);

		if ( this->msCount > 0)
		{
			// check if the multisample type is compatible with the selected display mode
			GLuint availableQualityLevels = 0;
			GLuint depthBufferQualityLevels = 0;

			// clamp multisample quality to the available quality levels
			if (availableQualityLevels > 0)
			{
				this->msQuality = availableQualityLevels;
			}
			else
			{
				this->msQuality = 0;
			}
		}
    #endif
}  

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderTarget::BeginPass()
{
	OGL4RenderDevice* renderDevice = OGL4RenderDevice::Instance();
	
	// call base function
    RenderTargetBase::BeginPass();

	// clear rendertarget
	this->Clear(this->clearFlags);

	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->SetClearDepth(this->clearDepth);
		this->depthStencilTarget->SetClearStencil(this->clearStencil);
		this->depthStencilTarget->SetClearFlags(this->clearFlags);
		this->depthStencilTarget->BeginPass();
	}

	// bind draw buffers
	glDrawBuffer(this->isDefaultRenderTarget ? GL_BACK : GL_COLOR_ATTACHMENT0);

    // set display dimensions
	Ptr<Shader> shader = RenderDevice::Instance()->GetPassShader();
    if (shader.isvalid() && shader->HasVariableByName(NEBULA_SEMANTIC_RENDERTARGETDIMENSIONS))
	{
        Ptr<ShaderVariable> var = shader->GetVariableByName(NEBULA_SEMANTIC_RENDERTARGETDIMENSIONS);
        uint width = this->width;
        uint height = this->height; 
		float xRatio = 1 / float(width);
		float yRatio = 1 / float(height);
		var->SetFloat4(Math::float4(xRatio, yRatio, (float)width, (float)height));
	}    
}

//------------------------------------------------------------------------------
/**
	Note: assumes a framebuffer object with this render target is currently bound
*/
void 
OGL4RenderTarget::Clear(uint flags)
{
	// if we're not using this render target, we must bind it first and then clear
	if (!this->inBeginPass)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
	}

	if (0 != (flags & ClearColor))
	{
		if (this->isDefaultRenderTarget)
		{
			const float* rgba = (const float*)&this->clearColor;
			glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else
		{
			n_assert(0 != this->ogl4ResolveTexture);
			glClearBufferfv(GL_COLOR, 0, (GLfloat*)&this->clearColor);
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderTarget::EndPass()
{
    RenderTargetBase::EndPass();
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->EndPass();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderTarget::GenerateMipLevels()
{
    n_assert(0 != this->ogl4ResolveTexture);
    n_assert(this->mipMapsEnabled);

	this->resolveTexture->GenerateMipmaps();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTarget::OnWindowResized(SizeT w, SizeT h)
{
	DisplayDevice* displayDevice = DisplayDevice::Instance();

	// save depth stencil
	if (this->ogl4ResolveTexture && this->relativeSizeValid)
	{
		this->width = SizeT(Math::n_floor(w * this->relWidth));
		this->height = SizeT(Math::n_floor(h * this->relHeight));

		//glDeleteTextures(1, &this->ogl4ResolveTexture);
		//glGenTextures(1, &this->ogl4ResolveTexture);
		if (this->msCount > 1)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->ogl4ResolveTexture);
			glTexImage2DMultisample(
				GL_TEXTURE_2D_MULTISAMPLE, 
				this->msCount, 
				this->ogl4ColorBufferFormat, 
				this->width, 
				this->height, 
				GL_FALSE);
			n_assert(GLSUCCESS);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, this->ogl4ResolveTexture);            
			glTexImage2D(
				GL_TEXTURE_2D, 
				0, 
				this->ogl4ColorBufferFormat, 
				this->width, 
				this->height, 
				0, 
				this->ogl4ColorBufferComponents,
				this->ogl4ColorBufferType,
				NULL);
			n_assert(GLSUCCESS);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

        // do not set left or top since they may be set prior to this
		this->resolveRect.right = this->resolveRect.left + this->width;
		this->resolveRect.bottom = this->resolveRect.top + this->height;

		if (this->msCount > 1)
		{
			this->resolveTexture->SetupFromOGL4MultisampleTexture(this->ogl4ResolveTexture, this->colorBufferFormat, 0, true, true);
		}
		else
		{
			this->resolveTexture->SetupFromOGL4Texture(this->ogl4ResolveTexture, this->colorBufferFormat, 0, true, true);
		}
	}
	else if (this->isDefaultRenderTarget)
	{
		this->resolveRect.top = 0;
		this->resolveRect.left = 0;
		this->resolveRect.right = w;
		this->resolveRect.bottom = h;
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderTarget::Reload()
{
    this->Discard();
    this->Setup();
}

//------------------------------------------------------------------------------
/**
    Can only be done with 2D textures.
*/
void
OGL4RenderTarget::Copy(const Ptr<CoreGraphics::RenderTarget>& rt)
{
    n_assert(rt.isvalid());

    // get other texture target and texture
    GLenum rtFbo = rt->ogl4Framebuffer;
    GLenum pixelFormat = rt->ogl4ColorBufferFormat;

    // assert both pixel formats are equal
    n_assert(this->ogl4ColorBufferFormat == pixelFormat);
    n_assert(this->width == rt->width);
    n_assert(this->height == rt->height);

	// bind other fbo as draw, this will unbind this render targets fbo if its currently bound
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rtFbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

    // bind our fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->ogl4Framebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    // blit
    glBlitFramebuffer(0, 0, this->width, this->height, 0, 0, rt->width, rt->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // undbind fbos
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

} // namespace OpenGL4

