//------------------------------------------------------------------------------
// ogl4shaderimage.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "ogl4shaderimage.h"
#include "ogl4types.h"
#include "coregraphics/texture.h"
#include "coregraphics/displaydevice.h"

namespace OpenGL4
{

__ImplementClass(OpenGL4::OGL4ShaderImage, 'O4SI', Base::ShaderReadWriteTextureBase);
//------------------------------------------------------------------------------
/**
*/
OGL4ShaderImage::OGL4ShaderImage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4ShaderImage::~OGL4ShaderImage()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderImage::Setup(const SizeT width, const SizeT height, const CoreGraphics::PixelFormat::Code& format, const Resources::ResourceId& id)
{
	Base::ShaderReadWriteTextureBase::Setup(width, height, format, id);

	// setup our pixel format and multisample parameters (order important!)
	GLenum ogl4ColorBufferComponents = OGL4Types::AsOGL4PixelComponents(format);
	GLenum ogl4ColorBufferFormat = OGL4Types::AsOGL4PixelFormat(format);
	GLenum ogl4ColorBufferType = OGL4Types::AsOGL4PixelType(format);

	// create texture
	glGenTextures(1, &this->ogl4Texture);
	glBindTexture(GL_TEXTURE_2D, this->ogl4Texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		ogl4ColorBufferFormat,
		width,
		height,
		0,
		ogl4ColorBufferComponents,
		ogl4ColorBufferType,
		NULL);
	n_assert(GLSUCCESS);
	glBindTexture(GL_TEXTURE_2D, 0);

	// setup texture with 0 mips
	this->texture->SetupFromOGL4Texture(this->ogl4Texture, format);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderImage::Resize(SizeT width, SizeT height)
{
	CoreGraphics::PixelFormat::Code format = this->texture->GetPixelFormat();

	// setup our pixel format and multisample parameters (order important!)
	GLenum ogl4ColorBufferComponents = OGL4Types::AsOGL4PixelComponents(format);
	GLenum ogl4ColorBufferFormat = OGL4Types::AsOGL4PixelFormat(format);
	GLenum ogl4ColorBufferType = OGL4Types::AsOGL4PixelType(format);

	// setup texture with new dimensions
	//GLuint newTex;

	// if we have a relative size, then assume we have to resize this too
	if (this->useRelativeSize)
	{
		width = SizeT(width * this->relWidth);
		height = SizeT(height * this->relHeight);
	}	

	// ugh, I need to do this...
	glDeleteTextures(1, &this->ogl4Texture);
	glGenTextures(1, &this->ogl4Texture);
	
	//this->ogl4Texture = newTex;
	glBindTexture(GL_TEXTURE_2D, this->ogl4Texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		ogl4ColorBufferFormat,
		width,
		height,
		0,
		ogl4ColorBufferComponents,
		ogl4ColorBufferType,
		NULL);
	n_assert(GLSUCCESS);
	glBindTexture(GL_TEXTURE_2D, 0);

	// setup texture with 0 mips
	this->texture->SetupFromOGL4Texture(this->ogl4Texture, format);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4ShaderImage::Clear(const Math::float4& clearColor)
{
	// setup clear flags
	CoreGraphics::PixelFormat::Code format = this->texture->GetPixelFormat();
	//GLenum ogl4ColorBufferComponents = OGL4Types::AsOGL4PixelComponents(format);
	//GLenum ogl4ColorBufferType = OGL4Types::AsOGL4PixelType(format);

	// perform clear, this requires GL 4.4, other versions should use the texture update method and convert floats to underlying type..
	glClearTexImage(this->ogl4Texture, 0, GL_RGBA, GL_FLOAT, (GLfloat*)&clearColor);
}

} // namespace OpenGL4