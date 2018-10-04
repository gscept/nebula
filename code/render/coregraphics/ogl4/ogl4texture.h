#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4Texture
    
    OGL4 implementation of Texture class
    
    (C) 2013 Gustav Sterbrant
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/texturebase.h"
#include "afxapi.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
    class Texture;
}

namespace OpenGL4
{
class OGL4Texture : public Base::TextureBase
{
    __DeclareClass(OGL4Texture);
public:
    /// constructor
    OGL4Texture();
    /// destructor
    virtual ~OGL4Texture();

    /// unload the resource, or cancel the pending load
    virtual void Unload();
    /// map a texture mip level for CPU access
    bool Map(IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
    /// unmap texture after CPU access
    void Unmap(IndexT mipLevel);
    /// map a cube map face for CPU access
    bool MapCubeFace(CubeFace face, IndexT mipLevel, MapType mapType, MapInfo& outMapInfo);
    /// unmap cube map face after CPU access
    void UnmapCubeFace(CubeFace face, IndexT mipLevel);
	/// generates mipmaps
	void GenerateMipmaps();

	/// updates texture
	void Update(void* data, SizeT size, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip);

	/// get ogl4 texture handle
    const GLuint& GetOGL4Texture() const;
	/// get ogl4 texture target
	const GLenum& GetOGL4TextureTarget() const;
	/// get ogl4 texture variable
	const AnyFX::OpenGLTextureBinding* GetOGL4Variable() const;

    /// setup from an opengl 2D texture
	void SetupFromOGL4Texture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	/// setup from an opengl 2d multisample texture
	void SetupFromOGL4MultisampleTexture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
    /// setup from an opengl texture cube
	void SetupFromOGL4CubeTexture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
    /// setup from an opengl volume texture
	void SetupFromOGL4VolumeTexture(const GLuint& texture, CoreGraphics::PixelFormat::Code format, GLint numMips = 0, const bool setLoaded = true, const bool isAttachment = false);
	
protected:

    AnyFX::OpenGLTextureBinding* ogl4TextureBinding;

	MapType mapType;
	GLenum target;
	GLuint ogl4Texture;
    GLuint64 ogl4TextureHandle;
	void* mappedData;
    int mapCount;
};

//------------------------------------------------------------------------------
/**
*/
inline const GLuint&
OGL4Texture::GetOGL4Texture() const
{
	return this->ogl4Texture;
}

//------------------------------------------------------------------------------
/**
*/
inline const GLenum& 
OGL4Texture::GetOGL4TextureTarget() const
{
	return this->target;
}

//------------------------------------------------------------------------------
/**
*/
inline const AnyFX::OpenGLTextureBinding*
OGL4Texture::GetOGL4Variable() const
{
	return this->ogl4TextureBinding;
}

}// namespace OpenGL4
//------------------------------------------------------------------------------
    