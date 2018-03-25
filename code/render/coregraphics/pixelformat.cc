//------------------------------------------------------------------------------
//  pixelformat.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
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
	else if (str == "BC7") return BC7;
	else if (str == "BC7 sRGB") return BC7sRGB;
	else if (str == "R16F") return R16F;
	else if (str == "R16G16F") return R16G16F;
	else if (str == "R16G16B16A16F") return R16G16B16A16F;
	else if (str == "R16G16B16A16") return R16G16B16A16;
	else if (str == "R32F") return R32F;         
	else if (str == "R32G32F") return R32G32F;      
	else if (str == "R32G32B32A32F") return R32G32B32A32F;
	else if (str == "R32G32B32F") return R32G32B32F;
	else if (str == "R11G11B10F") return R11G11B10F;
	else if (str == "A8") return A8;
	else if (str == "R10G10B10X2") return R10G10B10X2;
	else if (str == "R10G10B10A2") return R10G10B10A2;
	else if (str == "R16G16") return R16G16;
	else if (str == "D24S8") return D24S8;
	else if (str == "D24X8") return D24X8;
	else if (str == "D32S8") return D32S8;

	// Xbox360 pixel-formats
	else if (str == "DXN") return DXN;
	else if (str == "LINDXN") return LINDXN;
	else if (str == "LINDXT1") return LINDXT1;
	else if (str == "LINDXT3") return LINDXT3;
	else if (str == "LINDXT5") return LINDXT5;
	else if (str == "LINA8R8G8B8") return LINA8R8G8B8;
	else if (str == "LINX8R8G8B8") return LINX8R8G8B8;
	else if (str == "EDG16R16") return EDG16R16;
	else if (str == "CTX1") return CTX1;
	else if (str == "LINCTX1") return LINCTX1;
	else if (str == "D24FS8") return D24FS8;

	// Wii pixel-formats
	else if (str == "I4") return I4;
	else if (str == "I8") return I8;
	else if (str == "IA4") return IA4;
	else if (str == "IA8") return IA8;
	else if (str == "Z8") return Z8;
	else if (str == "Z16") return Z16;
	else if (str == "Z24X8") return Z24X8;
	else if (str == "R4") return R4;
	else if (str == "RA4") return RA4;
	else if (str == "RA8") return RA8;
	else if (str == "R8") return R8;
	else if (str == "G8") return G8;
	else if (str == "B8") return B8;
	else if (str == "RG8") return RG8;
	else if (str == "GB8") return GB8;
	else if (str == "Z4") return Z4;
	else if (str == "Z8M") return Z8M;
	else if (str == "Z8L") return Z8L;
	else if (str == "Z16L") return Z16L;
	else if (str == "RGBA8") return RGBA8;
	else if (str == "RGB5A3") return RGB5A3;
	else if (str == "RGB565") return RGB565;
	else if (str == "CMPR") return CMPR;

	// PS3 pixel-formats
	else if (str == "X1R5G5B5_Z1R5G5B5")    return X1R5G5B5_Z1R5G5B5;
	else if (str == "X1R5G5B5_O1R5G5B5")    return X1R5G5B5_O1R5G5B5;
	else if (str == "X8R8G8B8_Z8R8G8B8")    return X8R8G8B8_Z8R8G8B8;
	else if (str == "G8B8")                 return G8B8;
	else if (str == "X8B8G8R8_Z8B8G8R8")    return X8B8G8R8_Z8B8G8R8;
	else if (str == "X8B8G8R8_O8B8G8R8")    return X8B8G8R8_O8B8G8R8;
	else if (str == "A8B8G8R8")             return A8B8G8R8;
	else if (str == "R6G5B5")               return R6G5B5;
	else if (str == "DEPTH24D8")            return DEPTH24D8;
	else if (str == "DEPTH24D8F")           return DEPTH24D8F;
	else if (str == "DEPTH16")              return DEPTH16;
	else if (str == "DEPTH16F")             return DEPTH16F;
	else if (str == "X16")                  return X16;
	else if (str == "COMPRESSED_HILO8")     return COMPRESSED_HILO8;
	else if (str == "COMPRESSED_HILO_S8")   return COMPRESSED_HILO_S8;
	else if (str == "COMPRESSED_B8R8_G8R8") return COMPRESSED_B8R8_G8R8;
	else if (str == "COMPRESSED_R8B8_R8G8") return COMPRESSED_R8B8_R8G8;
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
		case SRGBA8:		return "SRGBA8";
		case R11G11B10F:	return "R11G11B10F";
		case R5G6B5:        return "R5G6B5";
		case R5G5B5A1:      return "R5G5B5A1";
		case R4G4B4A4:      return "R4G4B4A4";
		case DXT1:          return "DXT1";
		case DXT1sRGB:		return "DXT1 sRGB";
		case DXT1A:         return "DXT1A";
		case DXT1AsRGB:		return "DXT1A sRGB";
		case DXT3:          return "DXT3";
		case DXT3sRGB:		return "DXT3 sRGB";
		case DXT5:          return "DXT5";
		case DXT5sRGB:		return "DXT5 sRGB";
		case BC7:			return "BC7";
		case BC7sRGB:		return "BC7 sRGB";
		case R16F:          return "R16F";
		case R16G16F:       return "R16G16F";
		case R16G16B16A16F: return "R16G16B16A16F";
		case R16G16B16A16:	return "R16G16B16A16";
		case R32F:          return "R32F";
		case R32G32F:       return "R32G32F";
		case R32G32B32A32F: return "R32G32B32A32F";
		case R32G32B32F:	return "R32G32B32F";
		case A8:            return "A8";
		case R10G10B10X2:   return "R10G10B10X2";
		case R10G10B10A2:   return "R10G10B10A2";
		case R16G16:        return "G16R16";
		case D16S8:         return "D16S8";
		case D24X8:         return "D24X8";
		case D24S8:         return "D24S8";
		case D32S8:			return "D32S8";

		// Xbox360 pixel-formats
		case DXN:               return "DXN";
		case LINDXN:            return "LINDXN";
		case LINDXT1:           return "LINDXT1";
		case LINDXT3:           return "LINDXT3";
		case LINDXT5:           return "LINDXT5";
		case LINA8R8G8B8:       return "LINA8R8G8B8";
		case LINX8R8G8B8:       return "LINX8R8G8B8";
		case LINA16B16G16R16F:  return "LINA16B16G16R16F";
		case EDG16R16:          return "EDG16R16";
		case CTX1:              return "CTX1";
		case LINCTX1:           return "LINCTX1";
		case D24FS8:            return "D24FS8";

		// Wii pixel-formats
		case I4:            return "I4";
		case I8:            return "I8";
		case IA4:           return "IA4";
		case IA8:           return "IA8";
		case Z8:            return "Z8";
		case Z16:           return "Z16";
		case Z24X8:         return "Z24X8";
		case R4:            return "R4";
		case RA4:           return "RA4";
		case RA8:           return "RA8";
		case R8:            return "R8";
		case G8:            return "G8";
		case B8:            return "B8";
		case RG8:           return "RG8";
		case GB8:           return "GB8";
		case Z4:            return "Z4";
		case Z8M:           return "Z8M";
		case Z8L:           return "Z8L";
		case Z16L:          return "Z16L";
		case RGBA8:         return "RGBA8";
		case RGB5A3:        return "RGB5A3";
		case RGB565:        return "RGB565";
		case CMPR:          return "CMPR";

		// PS3 pixel-formats
		case X1R5G5B5_Z1R5G5B5:     return "X1R5G5B5_Z1R5G5B5";
		case X1R5G5B5_O1R5G5B5:     return "X1R5G5B5_O1R5G5B5";
		case X8R8G8B8_Z8R8G8B8:     return "X8R8G8B8_Z8R8G8B8";
		case G8B8:                  return "G8B8";
		case X8B8G8R8_Z8B8G8R8:     return "X8B8G8R8_Z8B8G8R8";
		case X8B8G8R8_O8B8G8R8:     return "X8B8G8R8_O8B8G8R8";
		case A8B8G8R8:              return "A8B8G8R8";
		case R6G5B5:                return "R6G5B5";
		case DEPTH24D8:             return "DEPTH24D8";
		case DEPTH24D8F:            return "DEPTH24D8F";
		case DEPTH16:               return "DEPTH16";
		case DEPTH16F:              return "DEPTH16F";
		case X16:                   return "X16";
		case COMPRESSED_HILO8:      return "COMPRESSED_HILO8";
		case COMPRESSED_HILO_S8:    return "COMPRESSED_HILO_S8";
		case COMPRESSED_B8R8_G8R8:  return "COMPRESSED_B8R8_G8R8";
		case COMPRESSED_R8B8_R8G8:  return "COMPRESSED_R8B8_R8G8";

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
	case R8G8B8X8:      return 4;
	case R8G8B8:        return 3;
	case R8G8B8A8:      return 4;
	case A8B8G8R8:      return 4;
	case SRGBA8:		return 4;
	case R5G6B5:        return 2;
	case R5G5B5A1:      return 2;
	case R4G4B4A4:      return 2;
	case DXT1:          return 3;
	case DXT1sRGB:      return 3;
	case DXT1A:         return 4;
	case DXT1AsRGB:     return 4;
	case DXT3:          return 4;
	case DXT5:          return 4;
	case DXT3sRGB:      return 4;
	case DXT5sRGB:      return 4;
	case BC7:			return 4;
	case BC7sRGB:		return 4;
	case R16F:          return 4;	// this is weird, but 1 channel - 16 bits (2 bytes) and format HALF_FLOAT doesn't work with glGetTexImage!?!?!?!?! Therefore, we select 3 bytes to make a buffer with bigger size...
	case R16G16F:       return 4;
	case R16G16B16A16F: return 8;
	case R16G16B16A16:	return 8;
	case R32F:          return 4;
	case R32G32F:       return 8;
	case R32G32B32A32F: return 16;
	case A8:            return 1;
	case R8:			return 1;
	case R10G10B10X2:   return 4;
	case R10G10B10A2:   return 4;
	case R16G16:        return 4;
	case D24X8:         return 4;
	case D24S8:         return 4;
	case D32S8:			return 5;

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
	case SRGBA8:		return 4;
	case R11G11B10F:	return 3;
	case R8G8B8:        return 3;
	case A8B8G8R8:      return 4;
	case R5G6B5:        return 3;
	case R5G5B5A1:      return 4;
	case R4G4B4A4:      return 4;
	case DXT1:          return 3;
	case DXT1sRGB:		return 3;
	case DXT1A:			return 4;
	case DXT1AsRGB: 	return 4;
	case DXT3:          return 4;
	case DXT3sRGB:		return 4;
	case DXT5:          return 4;
	case DXT5sRGB:		return 4;
	case BC7:			return 4;
	case BC7sRGB:		return 4;
	case R16F:          return 1;
	case R16G16F:       return 2;
	case R16G16B16A16F: return 4;
	case R16G16B16A16:	return 4;
	case R32F:          return 1;
	case R32G32F:       return 2;
	case R32G32B32A32F: return 4;
	case A8:            return 1;
	case R8:			return 1;
	case R10G10B10X2:   return 4;
	case R10G10B10A2:   return 4;
	case R16G16:        return 2;
	case D24X8:         return 2;
	case D24S8:         return 2;
	case D32S8:			return 2;

	default:
		n_error("Invalid pixel format code");
		return 4;
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
	case PixelFormat::A8B8G8R8:         return IL_BGRA;
	case PixelFormat::R5G6B5:           return IL_RGB;
	case PixelFormat::SRGBA8:			return IL_RGBA;
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
	case PixelFormat::BC7:		        return IL_RGBA;
	case PixelFormat::BC7sRGB:          return IL_RGBA;
	case PixelFormat::R16F:             return IL_RED;
	case PixelFormat::R16G16:			return IL_RG;
	case PixelFormat::R16G16F:          return IL_RG;
	case PixelFormat::R16G16B16A16F:    return IL_RGBA;
	case PixelFormat::R16G16B16A16:		return IL_RGBA;
	case PixelFormat::R11G11B10F:		return IL_RGB;
	case PixelFormat::R32F:             return IL_RED;
	case PixelFormat::R32G32F:          return IL_RG;
	case PixelFormat::R32G32B32A32F:    return IL_RGBA;
	case PixelFormat::R32G32B32F:		return IL_RGB;							
	case PixelFormat::A8:               return IL_ALPHA;	
	case PixelFormat::R8:				return IL_RED;
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
	case PixelFormat::A8B8G8R8:         return IL_UNSIGNED_BYTE;
	case PixelFormat::R5G6B5:           return IL_UNSIGNED_BYTE;
	case PixelFormat::SRGBA8:			return IL_UNSIGNED_BYTE;
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
	case PixelFormat::BC7:              return IL_UNSIGNED_BYTE;
	case PixelFormat::BC7sRGB:          return IL_UNSIGNED_BYTE;
	case PixelFormat::R16F:             return IL_HALF;
	case PixelFormat::R16G16:			return IL_SHORT;
	case PixelFormat::R16G16F:          return IL_HALF;
	case PixelFormat::R16G16B16A16F:    return IL_HALF;
	case PixelFormat::R16G16B16A16:		return IL_SHORT;
	case PixelFormat::R11G11B10F:		return IL_FLOAT;
	case PixelFormat::R32F:             return IL_FLOAT;
	case PixelFormat::R32G32F:          return IL_FLOAT;							
	case PixelFormat::R32G32B32A32F:    return IL_FLOAT;
	case PixelFormat::R32G32B32F:		return IL_FLOAT;							
	case PixelFormat::A8:               return IL_UNSIGNED_BYTE;	
	case PixelFormat::R8:				return IL_UNSIGNED_BYTE;
	case PixelFormat::R10G10B10A2:      return IL_UNSIGNED_BYTE;
	case PixelFormat::D24X8:            
	case PixelFormat::D24S8:            return IL_FLOAT;
	case PixelFormat::D32S8:			return IL_FLOAT;
	case PixelFormat::R8G8B8:           return IL_UNSIGNED_BYTE;
	default:                            
		{
			n_error("PixelFormat::ToILType(): invalid pixel components '%d'!", code);
			return IL_UNSIGNED_BYTE;
		}
	}
}
} // namespace CoreGraphics
