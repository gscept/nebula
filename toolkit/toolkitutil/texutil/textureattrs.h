#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::TextureAttrs
    
    Defines texture batch conversion attributes.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "io/filetime.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class TextureAttrs
{
public:
    /// pixel formats
    enum PixelFormat
    {
        DXT1C,
        DXT1A,
        DXT3,
        DXT5,
        DXT5NM,
        U1555,
        U4444,
        U565,
        U8888,
        U888,
        // single channel
        BC4,
        // dual channel
        BC5,
        // rgb/floating point/hdr
        BC6H,
        // rgb(a)
        BC7,
        R8,
        R16,
        R16G16,
        // raw dxgi string
        DXGI,
    };

    /// filter types
    enum Filter
    {
        Point,
        Box,
        Triangle,
        Quadrat,
        Cubic,
        Kaiser,
    };

    /// quality types
    enum Quality
    {
        Low,
        Normal,
        High,
    };

    /// color spaces
    enum ColorSpace
    {
        Linear,
        sRGB
    };
  
    /// constructor
    TextureAttrs();
    /// set max width
    void SetMaxWidth(SizeT w);
    /// get max width
    SizeT GetMaxWidth() const;
    /// set max height
    void SetMaxHeight(SizeT h);
    /// get max height
    SizeT GetMaxHeight() const;
    /// set mipmap generation flag
    void SetGenMipMaps(bool b);
    /// get mipmap generation flag
    bool GetGenMipMaps() const;
    /// set RGB pixel format
    void SetPixelFormat(PixelFormat p);
    /// get RGB pixel form
    PixelFormat GetPixelFormat() const;
    /// set mipmap filter
    void SetMipMapFilter(Filter f);
    /// get mipmap filter
    Filter GetMipMapFilter() const;
    /// set scale filter
    void SetScaleFilter(Filter f);
    /// get scale filter
    Filter GetScaleFilter() const;
    /// set conversion quality
    void SetQuality(Quality q);
    /// get conversion quality
    Quality GetQuality() const;
    /// set conversion color space
    void SetColorSpace(ColorSpace c);
    /// get conversion color space
    ColorSpace GetColorSpace() const;
    /// set file date of attributes file to check for changes
    void SetTime(const IO::FileTime & ft);
    /// get attribute file date
    const IO::FileTime & GetTime() const;
    /// check if the normal maps y axis should be flipped
    const bool GetFlipNormalY() const;
    /// set if the normal maps y axis should be flipped
    void SetFlipNormalY(bool const val);
    /// set raw dxgi string for describing output format
    void SetDxgi(const Util::String& str);
    /// get raw dxgi string
    Util::String GetDxgi() const;

    /// convert pixel format to string
    static Util::String PixelFormatToString(PixelFormat p);
    /// convert string to pixel format
    static PixelFormat StringToPixelFormat(const Util::String& str);
    /// convert filter type to string
    static Util::String FilterToString(Filter f);
    /// convert string to filter type
    static Filter StringToFilter(const Util::String& str);
    /// convert quality to string
    static Util::String QualityToString(Quality q);
    /// convert string to quality
    static Quality StringToQuality(const Util::String& str);   
    /// convert color space to string
    static Util::String ColorSpaceToString(ColorSpace c);
    /// convert string to color space
    static ColorSpace StringToColorSpace(const Util::String& str);

private:
    SizeT maxWidth;
    SizeT maxHeight;
    bool genMipMaps;
    PixelFormat pixelFormat;
    Filter mipMapFilter;
    Filter scaleFilter;
    Quality quality;
    ColorSpace colorSpace;
    IO::FileTime attrTime;
    bool flipNormalY;

    Util::String dxgiRaw;
};

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetMaxWidth(SizeT w)
{
    this->maxWidth = w;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureAttrs::GetMaxWidth() const
{
    return this->maxWidth;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetMaxHeight(SizeT h)
{
    this->maxHeight = h;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
TextureAttrs::GetMaxHeight() const
{
    return this->maxHeight;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetGenMipMaps(bool b)
{
    this->genMipMaps = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
TextureAttrs::GetGenMipMaps() const
{
    return this->genMipMaps;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetPixelFormat(PixelFormat p)
{
    this->pixelFormat = p;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureAttrs::PixelFormat
TextureAttrs::GetPixelFormat() const
{
    return this->pixelFormat;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetMipMapFilter(Filter f)
{
    this->mipMapFilter = f;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureAttrs::Filter
TextureAttrs::GetMipMapFilter() const
{
    return this->mipMapFilter;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetScaleFilter(Filter f)
{
    this->scaleFilter = f;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureAttrs::Filter
TextureAttrs::GetScaleFilter() const
{
    return this->scaleFilter;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetQuality(Quality q)
{
    this->quality = q;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureAttrs::Quality
TextureAttrs::GetQuality() const
{
    return this->quality;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetColorSpace(ColorSpace c)
{
    this->colorSpace = c;
}

//------------------------------------------------------------------------------
/**
*/
inline TextureAttrs::ColorSpace
TextureAttrs::GetColorSpace() const
{
    return this->colorSpace;
}

//------------------------------------------------------------------------------
/**
*/
inline const IO::FileTime &
TextureAttrs::GetTime() const
{
    return this->attrTime;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
TextureAttrs::GetFlipNormalY() const
{
    return this->flipNormalY;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetFlipNormalY(bool const val)
{
    this->flipNormalY = val;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetTime(const IO::FileTime & t)
{
    this->attrTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline void
TextureAttrs::SetDxgi(const Util::String& str)
{
    this->dxgiRaw = str;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::String
TextureAttrs::GetDxgi() const
{
    return this->dxgiRaw;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    