//------------------------------------------------------------------------------
//  ogl4multiplerendertarget.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4multiplerendertarget.h"
#include "ogl4renderdevice.h"
#include "coregraphics/rendertarget.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/shader.h"
#include "coregraphics/shadersemantics.h"
#include "coregraphics/shaderserver.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4MultipleRenderTarget, 'O4MR', Core::RefCounted);

using namespace CoreGraphics;
//------------------------------------------------------------------------------
/**
*/
OGL4MultipleRenderTarget::OGL4MultipleRenderTarget() :
	MultipleRenderTargetBase(),
	ogl4Framebuffer(0)
{
	glGenFramebuffers(1, &this->ogl4Framebuffer);
}

//------------------------------------------------------------------------------
/**
*/
OGL4MultipleRenderTarget::~OGL4MultipleRenderTarget()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4MultipleRenderTarget::AddRenderTarget(const Ptr<CoreGraphics::RenderTarget>& rt)
{
	n_assert(rt.isvalid());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + this->numRenderTargets, GL_TEXTURE_2D, rt->GetTexture(), 0);
	n_assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	MultipleRenderTargetBase::AddRenderTarget(rt);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4MultipleRenderTarget::SetDepthStencilTarget(const Ptr<CoreGraphics::DepthStencilTarget>& dt)
{
	n_assert(dt.isvalid());
	MultipleRenderTargetBase::SetDepthStencilTarget(dt);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, dt->GetDepthStencilRenderbuffer());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4MultipleRenderTarget::BeginPass()
{
	n_assert(this->numRenderTargets > 0);

	// clear buffers
	IndexT i;
	for (i = 0; i < this->numRenderTargets; i++)
	{
		if (0 != (this->clearFlags[i] & RenderTarget::ClearColor))
		{
			glClearBufferfv(GL_COLOR, i, (GLfloat*)&this->clearColor[i]);
		}
	}

	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->SetClearDepth(this->clearDepth);
		this->depthStencilTarget->SetClearStencil(this->clearStencil);
		this->depthStencilTarget->SetClearFlags(this->depthStencilClearFlags);
		this->depthStencilTarget->BeginPass();
	}

	const GLenum glColorAttachments[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
		GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7, GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
		GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15};

	// activate draw buffers
	glDrawBuffers(this->numRenderTargets, glColorAttachments);

    // set display dimensions
	Ptr<Shader> shader = ShaderServer::Instance()->GetSharedShader();
    if (shader.isvalid() && shader->HasVariableByName(NEBULA3_SEMANTIC_RENDERTARGETDIMENSIONS))
    {
        Ptr<CoreGraphics::ShaderVariable> var = shader->GetVariableByName(NEBULA3_SEMANTIC_RENDERTARGETDIMENSIONS);
        uint width = this->renderTarget[0]->GetResolveTextureWidth();
        uint height = this->renderTarget[0]->GetResolveTextureHeight();
        float xRatio = 1 / float(width);
        float yRatio = 1 / float(height);
        var->SetFloat4(Math::float4(xRatio, yRatio, (float)width, (float)height));
    } 
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4MultipleRenderTarget::BeginBatch( CoreGraphics::FrameBatchType::Code batchType )
{
	// empty, do nothing
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4MultipleRenderTarget::EndBatch()
{
	// empty, do nothing
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4MultipleRenderTarget::EndPass()
{
	if (this->depthStencilTarget.isvalid())
	{
		this->depthStencilTarget->EndPass();
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4MultipleRenderTarget::OnDisplayResized(SizeT width, SizeT height)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->ogl4Framebuffer);
	IndexT i;
	for (i = 0; i < this->numRenderTargets; i++)
	{
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + this->numRenderTargets, GL_TEXTURE_2D, this->renderTarget[i]->GetTexture(), 0);
	}
	if (this->depthStencilTarget.isvalid())
	{
		glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->depthStencilTarget->GetDepthStencilRenderbuffer());
	}
	n_assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

} // namespace OpenGL4