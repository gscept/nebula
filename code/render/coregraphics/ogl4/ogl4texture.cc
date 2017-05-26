//------------------------------------------------------------------------------
//  OGL4Texture.cc
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4texture.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "coregraphics/renderdevice.h"

namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4Texture, 'O4TX', Base::TextureBase);

using namespace CoreGraphics;
using namespace OpenGL4;

//------------------------------------------------------------------------------
/**
*/
OGL4Texture::OGL4Texture() :
    ogl4Texture(0),
	ogl4TextureBinding(0),
    mapCount(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
OGL4Texture::~OGL4Texture()
{
    n_assert(0 == this->mapCount);
    n_assert(0 == this->ogl4Texture);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4Texture::Unload()
{
    n_assert(0 == this->mapCount);
    if (0 != this->ogl4Texture)
    {
        glDeleteTextures(1, &this->ogl4Texture);
		this->ogl4Texture = 0;
    }

	if (this->ogl4TextureBinding)
	{
		n_delete(this->ogl4TextureBinding);
		this->ogl4TextureBinding = 0;
	}	

    TextureBase::Unload();
}

//------------------------------------------------------------------------------
/**
*/
bool
OGL4Texture::Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
    n_assert((this->type == Texture2D) || (this->type == Texture3D));
    n_assert(MapWriteNoOverwrite != mapType);
	this->mapType = mapType;
    bool retval = false;
	GLenum flags;
	switch (mapType)
	{
	case MapRead:
		n_assert(AccessRead & this->access);
		flags = GL_STREAM_READ;
		break;
	case MapWrite:
		n_assert((UsageDynamic == this->usage) && (AccessWrite & this->access));
		flags = GL_STREAM_DRAW;
		break;
	case MapReadWrite:
		n_assert((UsageDynamic == this->usage) && (AccessReadWrite == this->access));
		flags = GL_STREAM_COPY;
		break;
	case MapWriteDiscard:
		n_assert((UsageDynamic == this->usage) && (AccessWrite & this->access));
		flags = GL_STREAM_COPY;
		break;
	}

	// bind texture
	glBindTexture(this->target, this->ogl4Texture);

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(this->pixelFormat);
	glPixelStorei(GL_PACK_ALIGNMENT, pixelAlignment);

	if (Texture2D == this->type)
	{
		GLenum pixelType = OGL4Types::AsOGL4PixelFormat(this->pixelFormat);
		GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
		GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);

		GLint mipWidth;
		GLint mipHeight;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_WIDTH, &mipWidth);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_HEIGHT, &mipHeight);

		GLint size;
		size = PixelFormat::ToSize(this->pixelFormat);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.mipWidth = mipWidth;
		outMapInfo.mipHeight = mipHeight;
		outMapInfo.rowPitch = size * mipWidth;
		outMapInfo.depthPitch = 0;
		
		this->mappedData = Memory::Alloc(Memory::ObjectArrayHeap, mipWidth * mipHeight * size);
		glGetTexImage(this->target, mipLevel, components, type, this->mappedData);

		outMapInfo.data = this->mappedData;
		retval = GLSUCCESS;
    }
    else if (Texture3D == this->type)
    {
		GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
		GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);

		GLint mipWidth;
		GLint mipHeight;
		GLint mipDepth;
		glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_WIDTH, &mipWidth);
		glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_HEIGHT, &mipHeight);
		glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_DEPTH, &mipDepth);

		GLint size;
		size = PixelFormat::ToSize(this->pixelFormat);

		// the row pitch must be the size of one pixel times the number of pixels in width
		outMapInfo.rowPitch = size * mipWidth;
		outMapInfo.depthPitch = 0;		

		this->mappedData = Memory::Alloc(Memory::ObjectArrayHeap, mipWidth * mipHeight * mipDepth * size);
		glGetTexImage(this->target, mipLevel, components, type, this->mappedData);

		outMapInfo.data = this->mappedData;
		retval = GLSUCCESS;
    }

	// reset texture bind and pixel alignment
	glBindTexture(this->target, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

    if (retval)
    {
        this->mapCount++;
    }
    return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4Texture::Unmap(IndexT mipLevel)
{
    n_assert(this->mapCount > 0);
    n_assert((Texture2D == this->type) || (Texture3D == this->type));

	// unmap buffer
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, this->ogl4PixelBuffer);
	//glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	//glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindTexture(this->target, this->ogl4Texture);
	
	GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
	GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);
	GLenum format = OGL4Types::AsOGL4PixelFormat(this->pixelFormat);

	GLint mipWidth;
	GLint mipHeight;
	GLint mipDepth;
	glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_WIDTH, &mipWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_HEIGHT, &mipHeight);
	glGetTexLevelParameteriv(GL_TEXTURE_3D, mipLevel, GL_TEXTURE_DEPTH, &mipDepth);

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(this->pixelFormat);
	glPixelStorei(GL_UNPACK_ALIGNMENT, pixelAlignment);

	// calculate size of pixel
	GLint size;
	size = PixelFormat::ToSize(this->pixelFormat);

	// only write back if we bound this texture using any write mapping
	switch (mapType)
	{
	case MapWriteDiscard:
	case MapReadWrite:
	case MapWrite:
		if (Texture2D == this->type)
		{
			glTexImage2D(this->target, mipLevel, components, mipWidth, mipHeight, 0, format, type, this->mappedData);
		}
		else if (Texture3D == this->type)
		{
			glTexImage3D(this->target, mipLevel, components, mipWidth, mipHeight, mipDepth, 0, format, type, this->mappedData);
		}
		break;
	}

	// reset texture bind and pixel alignment
	glBindTexture(this->target, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	Memory::Free(Memory::ObjectArrayHeap, this->mappedData);

	this->mappedData = 0;
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
*/
bool
OGL4Texture::MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo)
{
    n_assert(TextureCube == this->type);
    n_assert(MapWriteNoOverwrite != mapType);
	this->mapType = mapType;
	bool retval = false;
	GLenum flags;
    switch (mapType)
    {
	case MapRead:
		n_assert(AccessRead & this->access);
		flags = GL_STREAM_READ;
		break;
	case MapWrite:
		n_assert((UsageDynamic == this->usage) && (AccessWrite & this->access));
		flags = GL_STREAM_DRAW;
		break;
	case MapReadWrite:
		n_assert((UsageDynamic == this->usage) && (AccessReadWrite == this->access));
		flags = GL_STREAM_COPY;
		break;
	case MapWriteDiscard:
		n_assert((UsageDynamic == this->usage) && (AccessWrite & this->access));
		flags = GL_STREAM_COPY;
		break;
    }
	
	glBindTexture(this->target, this->ogl4Texture);
	GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
	GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);

	GLint mipWidth;
	GLint mipHeight;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipLevel, GL_TEXTURE_WIDTH, &mipWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, mipLevel, GL_TEXTURE_HEIGHT, &mipHeight);

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(this->pixelFormat);
	glPixelStorei(GL_PACK_ALIGNMENT, pixelAlignment);

	// calculate size of pixel
	GLint size;
	size = PixelFormat::ToSize(this->pixelFormat);

	// the row pitch must be the size of one pixel times the number of pixels in width
	outMapInfo.mipWidth = mipWidth;
	outMapInfo.mipHeight = mipHeight;
	outMapInfo.rowPitch = size * mipWidth;
	outMapInfo.depthPitch = 0;

	this->mappedData = n_new_array(byte, mipWidth * mipHeight * size);
	glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X  + face, mipLevel, components, type, this->mappedData);

	outMapInfo.data = this->mappedData;
	retval = GLSUCCESS;

	// reset texture bind and pixel alignment
	glBindTexture(this->target, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	if (retval)
	{
		this->mapCount++;
	}

	return retval;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4Texture::UnmapCubeFace(CubeFace face, IndexT mipLevel)
{
    n_assert(TextureCube == this->type);
    n_assert(this->mapCount > 0);

	glBindTexture(this->target, this->ogl4Texture);
	GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
	GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);
	GLenum format = OGL4Types::AsOGL4PixelFormat(this->pixelFormat);

	GLint mipWidth;
	GLint mipHeight;
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mipLevel, GL_TEXTURE_WIDTH, &mipWidth);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mipLevel, GL_TEXTURE_HEIGHT, &mipHeight);

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(this->pixelFormat);
	glPixelStorei(GL_UNPACK_ALIGNMENT, pixelAlignment);

	// calculate size of pixel
	GLint size;
	size = PixelFormat::ToSize(this->pixelFormat);

	// only write back if we bound this texture using any write mapping
	switch (mapType)
	{
	case MapWriteDiscard:
	case MapReadWrite:
	case MapWrite:
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mipLevel, components, mipWidth, mipHeight, 0, format, type, this->mappedData);
		break;
	}
	glBindTexture(this->target, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	
	Memory::Free(Memory::ObjectArrayHeap, this->mappedData);
	this->mappedData = 0;
    this->mapCount--;
}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a OGL4 2D texture.
*/
void
OGL4Texture::SetupFromOGL4Texture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips, const bool setLoaded, const bool isAttachment)
{
    n_assert(0 != texture);    

    this->ogl4Texture = texture;
	this->target = GL_TEXTURE_2D;
	GLint width;
	GLint height;
	glBindTexture(GL_TEXTURE_2D, this->ogl4Texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, numMips);
	glBindTexture(GL_TEXTURE_2D, 0);

    // create bindless texture if it's not an attachment
    if (0 == this->ogl4TextureBinding) this->ogl4TextureBinding = n_new(AnyFX::OpenGLTextureBinding);

#if OGL4_USE_BINDLESS_TEXTURE
	// setup bindless state
	this->ogl4TextureHandle = glGetTextureHandleARB(this->ogl4Texture);
    glMakeTextureHandleResidentARB(this->ogl4TextureHandle);
    this->ogl4TextureBinding->notbound.handle = this->ogl4TextureHandle;
#endif

    this->ogl4TextureBinding->bindless = false;
    this->ogl4TextureBinding->bound.handle = this->ogl4Texture;
    this->ogl4TextureBinding->bound.textureType = GL_TEXTURE_2D;

	this->SetType(OGL4Texture::Texture2D);
    this->SetWidth(width);
    this->SetHeight(height);
    this->SetDepth(1);
	this->SetNumMipLevels(Math::n_max(1, numMips));
    this->SetPixelFormat(format);
	this->access = ResourceBase::AccessReadWrite;
	this->isRenderTargetAttachment = isAttachment;
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4Texture::SetupFromOGL4MultisampleTexture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips, const bool setLoaded, const bool isAttachment)
{
	n_assert(0 != texture);    

	this->ogl4Texture = texture;
	this->target = GL_TEXTURE_2D_MULTISAMPLE;
	GLint width;
	GLint height;
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->ogl4Texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_HEIGHT, &height);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, numMips);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

    // create bindless texture if it's not an attachment
    if (0 == this->ogl4TextureBinding) this->ogl4TextureBinding = n_new(AnyFX::OpenGLTextureBinding);

#if OGL4_USE_BINDLESS_TEXTURE
	// setup bindless state
	this->ogl4TextureHandle = glGetTextureHandleARB(this->ogl4Texture);
    glMakeTextureHandleResidentARB(this->ogl4TextureHandle);
    this->ogl4TextureBinding->notbound.handle = this->ogl4TextureHandle;
#endif

    this->ogl4TextureBinding->bindless = false;
    this->ogl4TextureBinding->bound.handle = this->ogl4Texture;
    this->ogl4TextureBinding->bound.textureType = GL_TEXTURE_2D_MULTISAMPLE;
    
	this->SetType(OGL4Texture::Texture2D);
	this->SetWidth(width);
	this->SetHeight(height);
	this->SetDepth(1);
	this->SetNumMipLevels(Math::n_max(1, numMips));
	this->SetPixelFormat(format);
	this->access = ResourceBase::AccessReadWrite;
	this->isRenderTargetAttachment = isAttachment;
	if (setLoaded)
	{
		this->SetState(Resource::Loaded);
	}
}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a OGL4 volume texture.
*/
void
OGL4Texture::SetupFromOGL4VolumeTexture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips, const bool setLoaded, const bool isAttachment)
{
    n_assert(0 != texture);

	this->ogl4Texture = texture;
	this->target = GL_TEXTURE_3D;
	GLint width;
	GLint height;
	GLint depth;
	glBindTexture(GL_TEXTURE_3D, this->ogl4Texture);
	glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &height);
	glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &depth);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, numMips);
	glBindTexture(GL_TEXTURE_3D, 0);

    // create bindless texture if it's not an attachment
    if (0 == this->ogl4TextureBinding) this->ogl4TextureBinding = n_new(AnyFX::OpenGLTextureBinding);

#if OGL4_USE_BINDLESS_TEXTURE
	// setup bindless state
	this->ogl4TextureHandle = glGetTextureHandleARB(this->ogl4Texture);
    glMakeTextureHandleResidentARB(this->ogl4TextureHandle);
    this->ogl4TextureBinding->notbound.handle = this->ogl4TextureHandle;
#endif

    this->ogl4TextureBinding->bindless = false;
    this->ogl4TextureBinding->bound.handle = this->ogl4Texture;
    this->ogl4TextureBinding->bound.textureType = GL_TEXTURE_3D;

	this->SetType(OGL4Texture::Texture3D);
    this->SetWidth(width);
    this->SetHeight(height);
    this->SetDepth(depth);
	this->SetNumMipLevels(Math::n_max(1, numMips));
    this->SetPixelFormat(format);
	this->access = ResourceBase::AccessReadWrite;
	this->isRenderTargetAttachment = isAttachment;
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }
}

//------------------------------------------------------------------------------
/**
    Helper method to setup the texture object from a OGL4 cube texture.
*/
void
OGL4Texture::SetupFromOGL4CubeTexture(const GLuint& texCube, CoreGraphics::PixelFormat::Code format, GLint numMips, const bool setLoaded, const bool isAttachment)
{
    n_assert(0 != texCube);

    this->ogl4Texture = texCube;
	this->target = GL_TEXTURE_CUBE_MAP;
	GLint width;
	GLint height;

	glBindTexture(GL_TEXTURE_CUBE_MAP, this->ogl4Texture);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_HEIGHT, &height);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, numMips);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // create bindless texture if it's not an attachment
    if (0 == this->ogl4TextureBinding) this->ogl4TextureBinding = n_new(AnyFX::OpenGLTextureBinding);

#if OGL4_USE_BINDLESS_TEXTURE
	// setup bindless state
	this->ogl4TextureHandle = glGetTextureHandleARB(this->ogl4Texture);
    glMakeTextureHandleResidentARB(this->ogl4TextureHandle);
    this->ogl4TextureBinding->notbound.handle = this->ogl4TextureHandle;
#endif

    // setup bound state
    this->ogl4TextureBinding->bindless = false;
    this->ogl4TextureBinding->bound.handle = this->ogl4Texture;
    this->ogl4TextureBinding->bound.textureType = GL_TEXTURE_CUBE_MAP;

	this->SetType(OGL4Texture::TextureCube);
    this->SetWidth(width);
    this->SetHeight(height);
    this->SetDepth(6);
	this->SetNumMipLevels(Math::n_max(1, numMips));
    this->SetPixelFormat(format);
	this->access = ResourceBase::AccessReadWrite;
	this->isRenderTargetAttachment = isAttachment;
    if (setLoaded)
    {
        this->SetState(Resource::Loaded);
    }
}

//------------------------------------------------------------------------------
/**
*/
void OGL4Texture::GenerateMipmaps()
{
	n_assert(this->type == OGL4Texture::Texture2D || this->type == OGL4Texture::TextureCube)
	if (this->ogl4Texture)
	{
		// calculate how many mips we will have	
		GLint mipLevels = (GLint)(ceil(Math::n_log2(Math::n_max((float)width,(float)height)))+1);

		// then set the amount
		this->SetNumMipLevels(mipLevels);

		glBindTexture(this->target, this->ogl4Texture);
		glTexParameteri(this->target, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(this->target, GL_TEXTURE_MAX_LEVEL, mipLevels);
		glGenerateMipmap(this->target);
		glBindTexture(this->target, 0);
	}
}

//------------------------------------------------------------------------------
/**
    Hmm, this is only viable for 2D textures
*/
void 
OGL4Texture::Update(void* data, SizeT size, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip)
{
	n_assert(0 != this->ogl4Texture);
	GLenum components = OGL4Types::AsOGL4PixelComponents(this->pixelFormat);
	GLenum type = OGL4Types::AsOGL4PixelType(this->pixelFormat);

	// update image, in my honest opinion, it should be possible to select what kind of function 
    glInvalidateTexSubImage(this->ogl4Texture, mip, left, top, 0, width, height, 1);
	glBindTexture(this->target, this->ogl4Texture);
	glTexSubImage2D(this->target, mip, left, top, width, height, components, type, data);
	glBindTexture(this->target, 0);
}
} // namespace OpenGL4

