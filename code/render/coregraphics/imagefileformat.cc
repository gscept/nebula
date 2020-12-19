//------------------------------------------------------------------------------
//  imagefileformat.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/imagefileformat.h"

namespace CoreGraphics
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
ImageFileFormat::Code
ImageFileFormat::FromString(const String& str)
{
    if (str == "bmp") return BMP;
    else if (str == "jpg") return JPG;
    else if (str == "png") return PNG;
    else if (str == "dds") return DDS;
    else if (str == "tga") return TGA;
    else
    {
        return InvalidImageFileFormat;
    }
}

//------------------------------------------------------------------------------
/**
*/
Util::String
ImageFileFormat::ToString(ImageFileFormat::Code c)
{
    switch (c)
    {
        case BMP:   return "bmp";
        case JPG:   return "jpg";
        case PNG:   return "png";
        case DDS:   return "dds";
        case TGA:   return "tga";
        default:
            n_error("ImageFileFormat::ToString(): invalid image file format code!");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
ImageFileFormat::Code
ImageFileFormat::FromMediaType(const IO::MediaType& mediaType)
{
    n_assert(mediaType.IsValid());
    if (mediaType.GetType() != "image")
    {
        return InvalidImageFileFormat;
    }
    else
    {
        const String& subType = mediaType.GetSubType();
        if (subType == "bmp")       return BMP;
        else if (subType == "jpeg") return JPG;
        else if (subType == "png")  return PNG;
        else if (subType == "dds")  return DDS; // hmm... non-standard
        else if (subType == "tga")  return TGA;
        else
        {
            return InvalidImageFileFormat;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
IO::MediaType
ImageFileFormat::ToMediaType(Code c)
{
    switch (c)
    {
        case BMP:   return IO::MediaType("image", "bmp");
        case JPG:   return IO::MediaType("image", "jpeg");
        case PNG:   return IO::MediaType("image", "png");
        case DDS:   return IO::MediaType("image", "dds");   // hmm... non-standard
        case TGA:   return IO::MediaType("image", "tga");
        default:
            n_error("ImageFileFormat::ToMediaType(): invalid image file format code!");
            return IO::MediaType();
    }
}

} // namespace CoreGraphics
