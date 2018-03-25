//------------------------------------------------------------------------------
//  textureattrs.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "textureattrs.h"

namespace ToolkitUtil
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
TextureAttrs::TextureAttrs() :
    maxWidth(0),
    maxHeight(0),
    genMipMaps(false),
    rgbPixelFormat(DXT1C),
    rgbaPixelFormat(DXT3),
    mipMapFilter(Point),
    scaleFilter(Point),
	quality(Low),
	colorSpace(sRGB)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
String
TextureAttrs::PixelFormatToString(PixelFormat f)
{
    switch (f)
    {
        case DXT1C:     return "dxt1c";
        case DXT1A:     return "dxt1a";
        case DXT3:      return "dxt3";
        case DXT5:      return "dxt5";
        case DXT5NM:    return "dxt5nm";
        case U1555:     return "u1555";
        case U4444:     return "u4444";
        case U565:      return "u565";
        case U8888:     return "u8888";
        case U888:      return "u888";
        case V8U8:      return "v8u8";
        default:        return "dxt1c";
    }
}

//------------------------------------------------------------------------------
/**
*/
TextureAttrs::PixelFormat
TextureAttrs::StringToPixelFormat(const String& str)
{
    if (str == "dxt1c")       return DXT1C;
    else if (str == "dxt1a")  return DXT1A;
    else if (str == "dxt3")   return DXT3;
    else if (str == "dxt5")   return DXT5;
    else if (str == "dxt5nm") return DXT5NM;
    else if (str == "u1555")  return U1555;
    else if (str == "u4444")  return U4444;
    else if (str == "u565")   return U565;
    else if (str == "u8888")  return U8888;
    else if (str == "u888")   return U888;
    else if (str == "v8u8")   return V8U8;
    else
    {
        return DXT1C;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
TextureAttrs::FilterToString(Filter f)
{
    switch (f)
    {
        case Point:     return "Point";
        case Box:       return "Box";
        case Triangle:  return "Triangle";
        case Quadrat:   return "Quadrat";
        case Cubic:     return "Cubic";
        case Kaiser:    return "Kaiser";
        default:        return "Box";
    }
}

//------------------------------------------------------------------------------
/**
*/
TextureAttrs::Filter
TextureAttrs::StringToFilter(const String& str)
{
    if (str == "Point") return Point;
    else if (str == "Box") return Box;
    else if (str == "Triangle") return Triangle;
    else if (str == "Quadrat") return Quadrat;
    else if (str == "Cubic") return Cubic;
    else if (str == "Kaiser") return Kaiser;
    else return Box;
}

//------------------------------------------------------------------------------
/**
*/
String
TextureAttrs::QualityToString(Quality q)
{
    switch (q)
    {
        case Low:       return "Low";
        case Normal:    return "Normal";
        case High:      return "High";
        default:        return "Low";
    }
}

//------------------------------------------------------------------------------
/**
*/
TextureAttrs::Quality
TextureAttrs::StringToQuality(const String& str)
{
    if (str == "Low") return Low;
    else if (str == "Normal") return Normal;
    else if (str == "High") return High;
    else return Low;
}

//------------------------------------------------------------------------------
/**
*/
String
TextureAttrs::ColorSpaceToString(ColorSpace q)
{
	switch (q)
	{
	case Linear:       return "RGB";
	case sRGB:		return "sRGB";
	default:        return "RGB";
	}
}

//------------------------------------------------------------------------------
/**
*/
TextureAttrs::ColorSpace
TextureAttrs::StringToColorSpace(const String& str)
{
	if (str == "RGB") return Linear;
	else if (str == "sRGB") return sRGB;
	else return Linear;
}

} // namespace ToolkitUtil