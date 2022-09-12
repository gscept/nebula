//------------------------------------------------------------------------------
//  pixelformat.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/pixelformat.h"
#include <IL/il.h>

namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
    Convert a pixel format string into a pixel format code.
*/
PixelFormat::Code
PixelFormat::FromString(const Util::String& str)
{
    if (str == "R8G8B8X8") return R8G8B8X8;
    else if (str == "R8G8B8") return R8G8B8;
    else if (str == "R8G8B8A8") return R8G8B8A8;
    else if (str == "R5G6B5") return R5G6B5;
    else if (str == "SRGBA8") return SRGBA8;
    else if (str == "R5G5B5A1") return R5G5B5A1;
    else if (str == "R4G4B4A4") return R4G4B4A4;
    else if (str == "DXT1") return DXT1;
    else if (str == "DXT1 sRGB") return DXT1sRGB;
    else if (str == "DXT1A") return DXT1A;
    else if (str == "DXT1A sRGB") return DXT1AsRGB;
    else if (str == "DXT3") return DXT3;
    else if (str == "DXT3 sRGB") return DXT3sRGB;
    else if (str == "DXT5") return DXT5;
    else if (str == "DXT5 sRGB") return DXT5sRGB;
    else if (str == "BC4") return BC4;
    else if (str == "BC7") return BC7;
    else if (str == "BC7 sRGB") return BC7sRGB;
    else if (str == "R8") return R8;
    else if (str == "R16F") return R16F;
    else if (str == "R16") return R16;
    else if (str == "R16G16F") return R16G16F;
    else if (str == "R16G16") return R16G16;
    else if (str == "R16G16B16A16F") return R16G16B16A16F;
    else if (str == "R16G16B16A16") return R16G16B16A16;
    else if (str == "R32F") return R32F;
    else if (str == "R32") return R32;
    else if (str == "R32G32F") return R32G32F;
    else if (str == "R32G32") return R32G32;
    else if (str == "R32G32B32A32F") return R32G32B32A32F;
    else if (str == "R32G32B32A32") return R32G32B32A32;
    else if (str == "R32G32B32F") return R32G32B32F;
    else if (str == "R32G32B32") return R32G32B32;
    else if (str == "R11G11B10F") return R11G11B10F;
    else if (str == "R10G10B10X2") return R10G10B10X2;
    else if (str == "R10G10B10A2") return R10G10B10A2;
    else if (str == "D24S8") return D24S8;
    else if (str == "D24X8") return D24X8;
    else if (str == "D32S8") return D32S8;

    
    else
    {
        n_error("Invalid pixel format string '%s'!", str.AsCharPtr());
        return InvalidPixelFormat;
    }
}

//------------------------------------------------------------------------------
/**
    Convert pixel format code into a string.
*/
Util::String
PixelFormat::ToString(PixelFormat::Code code)
{
    switch (code)
    {
        case R8G8B8X8:      return "R8G8B8X8";
        case R8G8B8:        return "R8G8B8";
        case R8G8B8A8:      return "R8G8B8A8";
        case SRGBA8:        return "SRGBA8";
        case R11G11B10F:    return "R11G11B10F";
        case R5G6B5:        return "R5G6B5";
        case R5G5B5A1:      return "R5G5B5A1";
        case R4G4B4A4:      return "R4G4B4A4";
        case DXT1:          return "DXT1";
        case DXT1sRGB:      return "DXT1 sRGB";
        case DXT1A:         return "DXT1A";
        case DXT1AsRGB:     return "DXT1A sRGB";
        case DXT3:          return "DXT3";
        case DXT3sRGB:      return "DXT3 sRGB";
        case DXT5:          return "DXT5";
        case DXT5sRGB:      return "DXT5 sRGB";
        case BC4:           return "BC4";
        case BC7:           return "BC7";
        case BC7sRGB:       return "BC7 sRGB";
        case R8:            return "R8";
        case R16G16F:       return "R16G16F";
        case R16G16:        return "R16G16";
        case R16G16B16A16F: return "R16G16B16A16F";
        case R16G16B16A16:  return "R16G16B16A16";
        case R32F:          return "R32F";
        case R32:           return "R32";
        case R32G32F:       return "R32G32F";
        case R32G32:        return "R32G32";
        case R32G32B32A32F: return "R32G32B32A32F";
        case R32G32B32A32:  return "R32G32B32A32";
        case R32G32B32F:    return "R32G32B32F";
        case R32G32B32:     return "R32G32B32";
        case R10G10B10X2:   return "R10G10B10X2";
        case R10G10B10A2:   return "R10G10B10A2";
        case D16S8:         return "D16S8";
        case D24X8:         return "D24X8";
        case D24S8:         return "D24S8";
        case D32S8:         return "D32S8";

        default:
            n_error("Invalid pixel format code");
            return "";
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
PixelFormat::ToSize(Code code)
{
    switch (code)
    {
    case R8:
        return 1;
    case R16F:
    case R16:
    case R5G6B5:
    case R5G5B5A1:
    case R4G4B4A4:
        return 2;
    case R8G8B8:
        return 3;
    case R8G8B8X8:
    case R8G8B8A8:
    case B8G8R8A8:
    case SRGBA8:
    case R16G16F:
    case R16G16:
    case R32F:
    case R32:
    case R10G10B10X2:
    case R10G10B10A2:
    case D24X8:
    case D24S8:
        return 4;
    case D32S8:
        return 5;
    case DXT1:
    case DXT1sRGB:
    case DXT1A:
    case DXT1AsRGB:     
    case BC4:
    case R32G32F:
    case R32G32:
    case R16G16B16A16F:
    case R16G16B16A16:
        return 8;
    case DXT3:
    case DXT5:
    case DXT3sRGB:
    case DXT5sRGB:
    case BC5:
    case BC7:
    case BC7sRGB:
    case R32G32B32A32F:
    case R32G32B32A32:
        return 16;
    case R32G32B32F:
    case R32G32B32:
        return 12;

    default:
        n_error("Invalid pixel format code");
        return 4;
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
PixelFormat::ToChannels(Code code)
{
    switch (code)
    {
    case R8G8B8X8:      return 4;
    case R8G8B8A8:      return 4;
    case SRGBA8:        return 4;
    case R11G11B10F:    return 3;
    case R8G8B8:        return 3;
    case R5G6B5:        return 3;
    case R5G5B5A1:      return 4;
    case R4G4B4A4:      return 4;
    case DXT1:          return 3;
    case DXT1sRGB:      return 3;
    case DXT1A:         return 4;
    case DXT1AsRGB:     return 4;
    case DXT3:          return 4;
    case DXT3sRGB:      return 4;
    case DXT5:          return 4;
    case DXT5sRGB:      return 4;
    case BC4:           return 1;
    case BC7:           return 4;
    case BC7sRGB:       return 4;
    case R8:            return 1;
    case R16F:          return 1;
    case R16:           return 1;
    case R16G16F:       return 2;
    case R16G16:        return 2;
    case R16G16B16A16F: return 4;
    case R16G16B16A16:  return 4;
    case R32F:          return 1;
    case R32:           return 1;
    case R32G32F:       return 2;
    case R32G32:        return 2;
    case R32G32B32A32F: return 4;
    case R32G32B32A32:  return 4;
    case R32G32B32F:    return 3;
    case R32G32B32:     return 3;
    case R10G10B10X2:   return 4;
    case R10G10B10A2:   return 4;
    case D24X8:         return 2;
    case D24S8:         return 2;
    case D32S8:         return 2;

    default:
        n_error("Invalid pixel format code");
        return 4;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
PixelFormat::ToCompressed(Code code)
{
    switch (code)
    {
    case DXT1:
    case DXT1sRGB:
    case DXT1A:
    case DXT1AsRGB:
    case DXT3:
    case DXT3sRGB:
    case DXT5:
    case DXT5sRGB:
    case BC4:
    case BC5:
    case BC7:
    case BC7sRGB:       
        return true;
    default:            
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
SizeT
PixelFormat::ToTexelSize(Code code)
{
    switch (code)
    {
        case PixelFormat::R8G8B8X8:         return 4;
        case PixelFormat::R8G8B8A8:         return 4;
        case PixelFormat::R8G8B8:           return 4;
        case PixelFormat::R5G6B5:           return 4;
        case PixelFormat::SRGBA8:           return 4;
        case PixelFormat::R5G5B5A1:         return 4;
        case PixelFormat::R4G4B4A4:         return 4;
        case PixelFormat::DXT1:             return 16;
        case PixelFormat::DXT1A:            return 16;
        case PixelFormat::DXT3:             return 16;
        case PixelFormat::DXT5:             return 16;
        case PixelFormat::DXT1sRGB:         return 16;
        case PixelFormat::DXT1AsRGB:        return 16;
        case PixelFormat::DXT3sRGB:         return 16;
        case PixelFormat::DXT5sRGB:         return 16;
        case PixelFormat::BC4:              return 16;
        case PixelFormat::BC5:              return 16;
        case PixelFormat::BC7:              return 16;
        case PixelFormat::BC7sRGB:          return 16;
        case PixelFormat::R8:               return 4;
        case PixelFormat::R16F:             return 8;
        case PixelFormat::R16:              return 8;
        case PixelFormat::R16G16F:          return 8;
        case PixelFormat::R16G16:           return 8;
        case PixelFormat::R16G16B16A16F:    return 8;
        case PixelFormat::R16G16B16A16:     return 8;
        case PixelFormat::R11G11B10F:       return 4;
        case PixelFormat::R32F:             return 16;
        case PixelFormat::R32:              return 16;
        case PixelFormat::R32G32F:          return 16;
        case PixelFormat::R32G32:           return 16;
        case PixelFormat::R32G32B32A32F:    return 16;
        case PixelFormat::R32G32B32A32:     return 16;
        case PixelFormat::R32G32B32F:       return 16;
        case PixelFormat::R32G32B32:        return 16;
        case PixelFormat::R10G10B10A2:      return 4;
        case PixelFormat::D32S8:            return 4;
        case PixelFormat::D24X8:            return 4;
        case PixelFormat::D24S8:            return 4;
        default:
        {
            n_error("PixelFormat::ToBlockDimensions(): invalid pixel format '%d'", code);
            return 4;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
PixelFormat::ToILComponents(Code code)
{
    switch (code)
    {
    case PixelFormat::R8G8B8X8:         return IL_RGBA;
    case PixelFormat::R8G8B8A8:         return IL_RGBA;                         
    case PixelFormat::R5G6B5:           return IL_RGB;
    case PixelFormat::SRGBA8:           return IL_RGBA;
    case PixelFormat::R5G5B5A1:         return IL_RGBA;                     
    case PixelFormat::R4G4B4A4:         return IL_RGBA;
    case PixelFormat::DXT1:             return IL_RGB;
    case PixelFormat::DXT1sRGB:         return IL_RGB;
    case PixelFormat::DXT1A:            return IL_RGBA;
    case PixelFormat::DXT1AsRGB:        return IL_RGBA;
    case PixelFormat::DXT3:             return IL_RGBA;
    case PixelFormat::DXT3sRGB:         return IL_RGBA;
    case PixelFormat::DXT5:             return IL_RGBA;
    case PixelFormat::DXT5sRGB:         return IL_RGBA;
    case PixelFormat::BC4:              return IL_RED;
    case PixelFormat::BC7:              return IL_RGBA;
    case PixelFormat::BC7sRGB:          return IL_RGBA;
    case PixelFormat::R8:               return IL_RED;
    case PixelFormat::R16F:             return IL_RED;
    case PixelFormat::R16:              return IL_RED;
    case PixelFormat::R16G16:           return IL_RG;
    case PixelFormat::R16G16F:          return IL_RG;
    case PixelFormat::R16G16B16A16F:    return IL_RGBA;
    case PixelFormat::R16G16B16A16:     return IL_RGBA;
    case PixelFormat::R11G11B10F:       return IL_RGB;
    case PixelFormat::R32F:             return IL_RED;
    case PixelFormat::R32:              return IL_RED;
    case PixelFormat::R32G32F:          return IL_RG;
    case PixelFormat::R32G32:           return IL_RG;
    case PixelFormat::R32G32B32A32F:    return IL_RGBA;
    case PixelFormat::R32G32B32A32:     return IL_RGBA;
    case PixelFormat::R32G32B32F:       return IL_RGB;
    case PixelFormat::R32G32B32:        return IL_RGB;
    case PixelFormat::R10G10B10A2:      return IL_RGBA;
    case PixelFormat::D24X8:            
    case PixelFormat::D24S8:            return IL_RG;
    case PixelFormat::D32S8:            return IL_RG;
    case PixelFormat::R8G8B8:           return IL_RGB;
    default:                            
        {
            n_error("PixelFormat::ToILType(): invalid pixel components '%d'!", code);
            return IL_RGBA;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
uint
PixelFormat::ToILType(Code code)
{
    switch (code)
    {
    case PixelFormat::R8G8B8X8:         return IL_UNSIGNED_BYTE;
    case PixelFormat::R8G8B8A8:         return IL_UNSIGNED_BYTE;                            
    case PixelFormat::R5G6B5:           return IL_UNSIGNED_BYTE;
    case PixelFormat::SRGBA8:           return IL_UNSIGNED_BYTE;
    case PixelFormat::R5G5B5A1:         return IL_UNSIGNED_BYTE;                        
    case PixelFormat::R4G4B4A4:         return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT1:             return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT1sRGB:         return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT1A:            return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT1AsRGB:        return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT3:             return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT3sRGB:         return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT5:             return IL_UNSIGNED_BYTE;
    case PixelFormat::DXT5sRGB:         return IL_UNSIGNED_BYTE;
    case PixelFormat::BC4:              return IL_FLOAT;
    case PixelFormat::BC7:              return IL_UNSIGNED_BYTE;
    case PixelFormat::BC7sRGB:          return IL_UNSIGNED_BYTE;
    case PixelFormat::R8:               return IL_UNSIGNED_BYTE;
    case PixelFormat::R16F:             return IL_HALF;
    case PixelFormat::R16:              return IL_SHORT;
    case PixelFormat::R16G16F:          return IL_HALF;
    case PixelFormat::R16G16:           return IL_SHORT;
    case PixelFormat::R16G16B16A16F:    return IL_HALF;
    case PixelFormat::R16G16B16A16:     return IL_SHORT;
    case PixelFormat::R11G11B10F:       return IL_FLOAT;
    case PixelFormat::R32F:             return IL_FLOAT;
    case PixelFormat::R32:              return IL_UNSIGNED_INT;
    case PixelFormat::R32G32F:          return IL_FLOAT;
    case PixelFormat::R32G32:           return IL_UNSIGNED_INT;
    case PixelFormat::R32G32B32A32F:    return IL_FLOAT;
    case PixelFormat::R32G32B32A32:     return IL_UNSIGNED_INT;
    case PixelFormat::R32G32B32F:       return IL_FLOAT;
    case PixelFormat::R32G32B32:        return IL_UNSIGNED_INT;
    case PixelFormat::R10G10B10A2:      return IL_UNSIGNED_BYTE;
    case PixelFormat::D24X8:            
    case PixelFormat::D24S8:            return IL_FLOAT;
    case PixelFormat::D32S8:            return IL_FLOAT;
    case PixelFormat::R8G8B8:           return IL_UNSIGNED_BYTE;
    default:                            
        {
            n_error("PixelFormat::ToILType(): invalid pixel components '%d'!", code);
            return IL_UNSIGNED_BYTE;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
PixelFormat::IsDepthFormat(Code code)
{
    switch (code)
    {
    case PixelFormat::D24X8:
    case PixelFormat::D24S8:
    case PixelFormat::D32S8:            return true;
    }
    return false;
}
} // namespace CoreGraphics
