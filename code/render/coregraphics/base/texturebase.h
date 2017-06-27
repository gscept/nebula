#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::TextureBase
  
    The base class for texture objects.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/   
#include "coregraphics/base/resourcebase.h"
#include "coregraphics/pixelformat.h"
#include "math/rectangle.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
    class Texture;
}

namespace Base
{
class TextureBase : public Base::GpuResourceBase
{
    __DeclareClass(TextureBase);
public:
    /// texture types
    enum Type
    {
        InvalidType,

		Texture1D,		//> a 1-dimensional texture
        Texture2D,      //> a 2-dimensional texture
        Texture3D,      //> a 3-dimensional texture
        TextureCube,    //> a cube texture

		Texture1DArray,	//> a 1-dimensional texture array, depth represents array size
		Texture2DArray,	//> a 2-dimensional texture array, depth represents array size
		Texture3DArray,	//> a 3-dimensional texture array, depth represents array size * size of layers in array
		TextureCubeArray //> a cube texture array, depth represents array size * 6
    };

    /// cube map face
    enum CubeFace
    {
        PosX = 0,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ,
    };

    /// access info filled by Map methods
    class MapInfo
    {
    public:
        /// constructor
        MapInfo() : data(0), rowPitch(0), depthPitch(0) {};
        
        void* data;
        SizeT rowPitch;
        SizeT depthPitch;
		SizeT mipWidth;
		SizeT mipHeight;
    };

    /// constructor
    TextureBase();
    /// destructor
    virtual ~TextureBase();

    /// get texture type
    Type GetType() const;
	/// get bits per pixel
	SizeT GetBitsPerPixel() const;
    /// get width of texture 
    SizeT GetWidth() const;
    /// get height of texture (if 2d or 3d texture)
    SizeT GetHeight() const;
    /// get depth of texture (if 3d texture)
    SizeT GetDepth() const;
    /// get number of mip levels
    SizeT GetNumMipLevels() const;
    /// get number of currently skipped mip levels
    SizeT GetSkippedMips() const;
    /// set number of currently skipped mip levels
    void SetSkippedMips(SizeT m);
    /// get pixel format of the texture
    CoreGraphics::PixelFormat::Code GetPixelFormat() const;

	/// returns true if this texture was created as an attachment to a render target
	const bool IsRenderTargetAttachment() const;

    /// map the a texture mip level for CPU access
    bool Map(IndexT mipLevel, Access accessMode, MapInfo& outMapInfo);
    /// unmap texture after CPU access
    void Unmap(IndexT mipLevel);
    /// map a cube map face for CPU access
    bool MapCubeFace(CubeFace face, IndexT mipLevel, Access accessMode, MapInfo& outMapInfo);
    /// unmap cube map face after CPU access
    void UnmapCubeFace(CubeFace face, IndexT mipLevel);

	/// updates unmappable texture
	void Update(void* data, SizeT size, SizeT width, SizeT height, IndexT left, IndexT top, IndexT mip);


protected:
	friend class RenderTextureBase;

    /// set texture type
    void SetType(Type t);
	/// set bits per pixel
	void SetBitsPerPixel(SizeT size);
    /// set texture width
    void SetWidth(SizeT w);
    /// set texture height
    void SetHeight(SizeT h);
    /// set texture depth
    void SetDepth(SizeT d);
    /// set number of mip levels
    void SetNumMipLevels(SizeT n);
    /// set pixel format
    void SetPixelFormat(CoreGraphics::PixelFormat::Code f);

    Type type;
	SizeT bitsPerPixel;
    SizeT width;
    SizeT height;
    SizeT depth;
    SizeT numMipLevels;
    SizeT skippedMips;
	bool isRenderTargetAttachment;
    CoreGraphics::PixelFormat::Code pixelFormat;
};

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureBase::Type
TextureBase::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetBitsPerPixel(SizeT size)
{
	this->bitsPerPixel = size;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetBitsPerPixel() const
{
	return this->bitsPerPixel;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetWidth(SizeT w)
{
    this->width = w;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetHeight(SizeT h)
{
    this->height = h;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetDepth(SizeT d)
{
    this->depth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetDepth() const
{
    return this->depth;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetNumMipLevels(SizeT n)
{
    this->numMipLevels = n;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetNumMipLevels() const
{
    return this->numMipLevels;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetSkippedMips(SizeT m)
{
    this->skippedMips = m;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureBase::GetSkippedMips() const
{
    return this->skippedMips;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureBase::SetPixelFormat(CoreGraphics::PixelFormat::Code f)
{
    this->pixelFormat = f;
}

//------------------------------------------------------------------------------
/**
*/
inline CoreGraphics::PixelFormat::Code
TextureBase::GetPixelFormat() const
{
    return this->pixelFormat;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
TextureBase::IsRenderTargetAttachment() const
{
	return this->isRenderTargetAttachment;
}

} // namespace Base
//------------------------------------------------------------------------------
