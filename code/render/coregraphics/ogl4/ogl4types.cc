//------------------------------------------------------------------------------
//  ogl4types.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "math/float4.h"

namespace OpenGL4
{
using namespace Base;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::AsOGL4PixelFormat(PixelFormat::Code p)
{
    switch (p)
    {
	case PixelFormat::X8R8G8B8:         return GL_RGB8;
	case PixelFormat::A8R8G8B8:         return GL_RGBA8;	
    case PixelFormat::R8G8B8:           return GL_RGB8;
	case PixelFormat::R5G6B5:           return GL_RGB565;
	case PixelFormat::SRGBA8:			return GL_SRGB8_ALPHA8;
	case PixelFormat::A1R5G5B5:         return GL_RGB5_A1;						
	case PixelFormat::A4R4G4B4:         return GL_RGBA8;
	case PixelFormat::DXT1:             return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	case PixelFormat::DXT1A:            return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case PixelFormat::DXT3:             return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case PixelFormat::DXT5:             return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	case PixelFormat::DXT1sRGB:         return GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
	case PixelFormat::DXT1AsRGB:        return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
	case PixelFormat::DXT3sRGB:         return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
	case PixelFormat::DXT5sRGB:         return GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
	case PixelFormat::BC7:				return GL_COMPRESSED_RGBA_BPTC_UNORM;
	case PixelFormat::BC7sRGB:			return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
	case PixelFormat::R16F:             return GL_R16F;
	case PixelFormat::G16R16F:          return GL_RG16F;
	case PixelFormat::R16G16B16A16F:    return GL_RGBA16F;
	case PixelFormat::R16G16B16A16:		return GL_RGBA16;
	case PixelFormat::R11G11B10F:		return GL_R11F_G11F_B10F;
	case PixelFormat::R32F:             return GL_R32F;
	case PixelFormat::G32R32F:          return GL_RG32F;							
	case PixelFormat::R32G32B32A32F:    return GL_RGBA32F;
	case PixelFormat::R32G32B32F:		return GL_RGB32F;							
	case PixelFormat::A8:               return GL_ALPHA8;						
	case PixelFormat::R8:               return GL_R8;						
	case PixelFormat::G8:               return GL_R8;						
	case PixelFormat::B8:               return GL_R8;						
	case PixelFormat::A2R10G10B10:      return GL_RGB10_A2;						     
	case PixelFormat::G16R16:           return GL_RG16;				 
	case PixelFormat::D24X8:            
	case PixelFormat::D24S8:            return GL_DEPTH24_STENCIL8;
    default:                            
		{
			n_error("OGL4Types::AsOGL4PixelFormat(): invalid pixel format '%d'", p);
			return GL_RGBA8;
		}
    }
}


//------------------------------------------------------------------------------
/**
*/
GLenum 
OGL4Types::AsOGL4Compression( ILenum f )
{
	switch (f)
	{
	case IL_DXT1:
		return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	case IL_DXT1A:
		return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	case IL_DXT3:
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	case IL_DXT5:
		return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	default:
		{
			n_error("OGL4Types::AsOGL4Compression(): invalid compression '%d'", f);
			return 0;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
PixelFormat::Code
OGL4Types::AsNebulaPixelFormat(GLenum f)
{
    switch (f)
    {
		case GL_RGB:
        case GL_RGB8:									return PixelFormat::X8R8G8B8;
		case GL_RGBA:
        case GL_RGBA8:									return PixelFormat::A8R8G8B8;
        case GL_RGB565:									return PixelFormat::R5G6B5;
		case GL_SRGB8_ALPHA8:							return PixelFormat::SRGBA8;
        case GL_RGB5_A1:								return PixelFormat::A1R5G5B5;
        case GL_RGBA4:									return PixelFormat::A4R4G4B4;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:			return PixelFormat::DXT1;
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:			return PixelFormat::DXT1A;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:			return PixelFormat::DXT3;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:			return PixelFormat::DXT5;		
		case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:			return PixelFormat::DXT1sRGB;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:    return PixelFormat::DXT1AsRGB;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:	return PixelFormat::DXT3sRGB;
		case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:	return PixelFormat::DXT5sRGB;
		case GL_COMPRESSED_RGBA_BPTC_UNORM:				return PixelFormat::BC7;
		case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:	return PixelFormat::BC7sRGB;
		case GL_R16F:									return PixelFormat::R16F;
		case GL_RG16F:									return PixelFormat::G16R16F;
		case GL_RGBA16F:								return PixelFormat::R16G16B16A16F;
		case GL_RGBA16:									return PixelFormat::R16G16B16A16;
		case GL_R11F_G11F_B10F:							return PixelFormat::R11G11B10F;
		case GL_R32F:									return PixelFormat::R32F;
		case GL_RG32F:									return PixelFormat::G32R32F;
		case GL_RGBA32F:								return PixelFormat::R32G32B32A32F;
		case GL_ALPHA8:									return PixelFormat::A8;
		case GL_RGB10_A2:								return PixelFormat::A2R10G10B10;
		case GL_RG16:									return PixelFormat::G16R16;
		case GL_DEPTH24_STENCIL8:						return PixelFormat::D24S8;
        default:                        
			{
				n_error("OGL4Types::AsNebulaPixelFormta(): invalid pixel format '%d'!", f);
				return PixelFormat::InvalidPixelFormat;
			}
    }
}

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::AsOGL4PixelComponents(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::X8R8G8B8:         return GL_RGB;
	case PixelFormat::A8R8G8B8:         return GL_RGBA;		
    case PixelFormat::R8G8B8:           return GL_RGB;
	case PixelFormat::R5G6B5:           return GL_RGB;
	case PixelFormat::SRGBA8:			return GL_RGBA;
	case PixelFormat::A1R5G5B5:         return GL_RGB;						
	case PixelFormat::A4R4G4B4:         return GL_RGBA;
	case PixelFormat::DXT1:             return GL_RGB;
	case PixelFormat::DXT1A:            return GL_RGBA;
	case PixelFormat::DXT3:             return GL_RGBA;
	case PixelFormat::DXT5:             return GL_RGBA;
	case PixelFormat::DXT1sRGB:         return GL_RGB;
	case PixelFormat::DXT1AsRGB:        return GL_RGBA;
	case PixelFormat::DXT3sRGB:         return GL_RGBA;
	case PixelFormat::DXT5sRGB:         return GL_RGBA;
	case PixelFormat::BC7:				return GL_RGBA;
	case PixelFormat::BC7sRGB:			return GL_RGBA;
	case PixelFormat::R16F:             return GL_RED;
	case PixelFormat::G16R16:			return GL_RG;
	case PixelFormat::G16R16F:          return GL_RG;
	case PixelFormat::R16G16B16A16F:    return GL_RGBA;
	case PixelFormat::R16G16B16A16:		return GL_RGBA;
	case PixelFormat::R11G11B10F:		return GL_RGB;
	case PixelFormat::R32F:             return GL_RED;
	case PixelFormat::G32R32F:          return GL_RG;							
	case PixelFormat::R32G32B32A32F:    return GL_RGBA;
	case PixelFormat::R32G32B32F:		return GL_RGB;							
	case PixelFormat::A8:               return GL_ALPHA;						
	case PixelFormat::R8:               return GL_RED;
	case PixelFormat::G8:               return GL_GREEN;
	case PixelFormat::B8:               return GL_BLUE;
	case PixelFormat::A2R10G10B10:      return GL_RGBA;						     
	case PixelFormat::D24X8:            
	case PixelFormat::D24S8:            return GL_DEPTH_STENCIL;
	default:                            
		{
			n_error("OGL4Types::AsOGL4PixelComponents(): invalid pixel components '%d'!", p);
			return GL_RGBA;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::AsOGL4PixelType(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::X8R8G8B8:         return GL_UNSIGNED_BYTE;
	case PixelFormat::A8R8G8B8:         return GL_UNSIGNED_BYTE;			
    case PixelFormat::R8G8B8:           return GL_UNSIGNED_BYTE;			
	case PixelFormat::R5G6B5:           return GL_UNSIGNED_SHORT_5_6_5;
	case PixelFormat::R16G16B16A16:		return GL_UNSIGNED_SHORT;
	case PixelFormat::R16G16B16A16F:	return GL_HALF_FLOAT;
    case PixelFormat::R32G32B32A32F:    return GL_FLOAT;
	case PixelFormat::A1R5G5B5:         return GL_UNSIGNED_SHORT_1_5_5_5_REV;						
	case PixelFormat::DXT1:             return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT1A:            return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT3:             return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT5:             return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT1sRGB:         return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT1AsRGB:        return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT3sRGB:         return GL_UNSIGNED_BYTE;
	case PixelFormat::DXT5sRGB:         return GL_UNSIGNED_BYTE;
	case PixelFormat::BC7:				return GL_UNSIGNED_BYTE;
	case PixelFormat::BC7sRGB:			return GL_UNSIGNED_BYTE;
	case PixelFormat::SRGBA8:			return GL_UNSIGNED_BYTE;
	case PixelFormat::R11G11B10F:		return GL_UNSIGNED_INT_10F_11F_11F_REV;
	case PixelFormat::R16F:             return GL_HALF_FLOAT;
	case PixelFormat::G16R16:			return GL_UNSIGNED_SHORT;
	case PixelFormat::G16R16F:          return GL_HALF_FLOAT;
	case PixelFormat::G32R32F:			return GL_FLOAT;
	case PixelFormat::R32F:             return GL_FLOAT;
	case PixelFormat::A8:               return GL_UNSIGNED_BYTE;
	case PixelFormat::R8:               return GL_UNSIGNED_BYTE;
	case PixelFormat::G8:               return GL_UNSIGNED_BYTE;
	case PixelFormat::B8:               return GL_UNSIGNED_BYTE;
	case PixelFormat::A2R10G10B10:      return GL_UNSIGNED_INT_2_10_10_10_REV;						     

	default:                            
		{
			n_error("OGL4Types::AsOGL4PixelType(): invalid pixel type '%d'!", p);
			return GL_UNSIGNED_INT_8_8_8_8;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
GLuint
OGL4Types::AsOGL4PixelByteAlignment(CoreGraphics::PixelFormat::Code p)
{
	switch (p)
	{
	case PixelFormat::X8R8G8B8:         return 4;
	case PixelFormat::A8R8G8B8:         return 4;
	case PixelFormat::R8G8B8:           return 3;
	case PixelFormat::R5G6B5:           return 2;
	case PixelFormat::R16G16B16A16:		return 8;
	case PixelFormat::R16G16B16A16F:	return 8;
	case PixelFormat::R32G32B32A32F:    return 4;
	case PixelFormat::A1R5G5B5:         return 2;
	case PixelFormat::DXT1:             return 4;
	case PixelFormat::DXT1A:            return 4;
	case PixelFormat::DXT3:             return 4;
	case PixelFormat::DXT5:             return 4;
	case PixelFormat::DXT1sRGB:         return 4;
	case PixelFormat::DXT1AsRGB:        return 4;
	case PixelFormat::DXT3sRGB:         return 4;
	case PixelFormat::DXT5sRGB:         return 4;
	case PixelFormat::BC7:				return 4;
	case PixelFormat::BC7sRGB:			return 4;
	case PixelFormat::SRGBA8:			return 4;
	case PixelFormat::R11G11B10F:		return 4;
	case PixelFormat::R16F:             return 2;
	case PixelFormat::G16R16:			return 4;
	case PixelFormat::G16R16F:          return 4;
	case PixelFormat::G32R32F:			return 8;
	case PixelFormat::R32F:             return 4;
	case PixelFormat::A8:               return 1;
	case PixelFormat::R8:               return 1;
	case PixelFormat::G8:               return 1;
	case PixelFormat::B8:               return 1;
	case PixelFormat::A2R10G10B10:      return 4;

	default:
	{
		n_error("OGL4Types::AsOGL4PixelByteAlignment(): invalid pixel type '%d'!", p);
		return GL_UNSIGNED_INT_8_8_8_8;
	}
	}
}

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::AsOGL4SymbolicType(VertexComponent::Format f)
{
    switch (f)
    {
        case VertexComponent::Float:    return GL_FLOAT;
        case VertexComponent::Float2:   return GL_FLOAT;
        case VertexComponent::Float3:   return GL_FLOAT;
        case VertexComponent::Float4:   return GL_FLOAT;
        case VertexComponent::UByte4:   return GL_UNSIGNED_BYTE;
		case VertexComponent::Byte4:	return GL_BYTE;
        case VertexComponent::Short2:   return GL_SHORT;
        case VertexComponent::Short4:   return GL_SHORT;
		case VertexComponent::UByte4N:  return GL_UNSIGNED_BYTE;
		case VertexComponent::Byte4N:	return GL_BYTE;
        case VertexComponent::Short2N:  return GL_SHORT;
        case VertexComponent::Short4N:  return GL_SHORT;
        default:                        
            n_error("OGL4Types::AsOGL4SymbolicType(): invalid input parameter!");
            return GL_INT;
    }
}


//------------------------------------------------------------------------------
/**
*/
GLint
OGL4Types::AsOGL4Size(CoreGraphics::VertexComponent::Format f)
{
	switch (f)
	{
	case VertexComponent::Float:    return 4;
	case VertexComponent::Float2:   return 8;
	case VertexComponent::Float3:   return 12;
	case VertexComponent::Float4:   return 16;
	case VertexComponent::UByte4:   return 4;
	case VertexComponent::Byte4:    return 4;
	case VertexComponent::Short2:   return 4;
	case VertexComponent::Short4:   return 8;
	case VertexComponent::UByte4N:  return 4;
	case VertexComponent::Byte4N:   return 4;
	case VertexComponent::Short2N:  return 4;
	case VertexComponent::Short4N:  return 8;
	default:                        
		n_error("OGL4Types::AsOGL4Size(): invalid input parameter!");
		return 1;
	}
}

//------------------------------------------------------------------------------
/**
*/
GLint 
OGL4Types::AsOGL4NumComponents(CoreGraphics::VertexComponent::Format f)
{
	switch (f)
	{
	case VertexComponent::Float:    return 1;
	case VertexComponent::Float2:   return 2;
	case VertexComponent::Float3:   return 3;
	case VertexComponent::Float4:   return 4;
	case VertexComponent::UByte4:   return 4;
	case VertexComponent::Byte4:    return 4;
	case VertexComponent::Short2:   return 2;
	case VertexComponent::Short4:   return 4;
	case VertexComponent::UByte4N:  return 4;
	case VertexComponent::Byte4N:   return 4;
	case VertexComponent::Short2N:  return 2;
	case VertexComponent::Short4N:  return 4;
	default:                        
		n_error("OGL4Types::AsOGL4Size(): invalid input parameter!");
		return 1;
	}
}

//------------------------------------------------------------------------------
/**
	Not required in GL4
*/
GLenum
OGL4Types::AsOGL4VertexDeclarationUsage(VertexComponent::SemanticName n)
{
	return 0;
}

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::AsOGL4PrimitiveType(PrimitiveTopology::Code t)
{
    switch (t)
    {
        case PrimitiveTopology::PointList:      return GL_POINTS;
        case PrimitiveTopology::LineList:       return GL_LINES;
		case PrimitiveTopology::LineStrip:      return GL_LINE_STRIP;
        case PrimitiveTopology::TriangleList:   return GL_TRIANGLES;
        case PrimitiveTopology::TriangleStrip:  return GL_TRIANGLE_STRIP;
		case PrimitiveTopology::QuadList:		return GL_QUADS;
		case PrimitiveTopology::QuadStrip:		return GL_QUAD_STRIP;
		case PrimitiveTopology::PatchList:		return GL_PATCHES;

        default:
            n_error("OGL4Types::AsOGL4PrimitiveType(): unsupported topology '%s'!",
                PrimitiveTopology::ToString(t).AsCharPtr());
            return GL_TRIANGLES;
    }
}


//------------------------------------------------------------------------------
/**
*/
GLuint
OGL4Types::AsOGL4MultiSampleType(AntiAliasQuality::Code c)
{
    switch (c)
    {
        case AntiAliasQuality::None:    
            return 0;
        case AntiAliasQuality::Low:     
            return 2;
        case AntiAliasQuality::Medium:
            return 4;    
        case AntiAliasQuality::High:    
            return 8;    
        
        default:
            n_error("OGL4Types::AsOGL4MultiSampleType(): unsupported AntiAliasQuality!");
            return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
GLenum
OGL4Types::IndexTypeAsOGL4Format(IndexType::Code indexType)
{
    return (IndexType::Index16 == indexType) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

//------------------------------------------------------------------------------
/**
*/
GLuint 
OGL4Types::AsOGL4Access( Base::ResourceBase::Access access )
{
	switch (access)
	{
	case ResourceBase::AccessNone:
		return 0;
	case ResourceBase::AccessWrite:
		return GL_WRITE_ONLY;
	case ResourceBase::AccessRead:
		return GL_READ_ONLY;
	case ResourceBase::AccessReadWrite:
		return GL_READ_WRITE;
		
	default:
		n_error("OGL4Types::AsOGL4Access(): invalid input access!");
		return 0;
		
	}
}


//------------------------------------------------------------------------------
/**
*/
GLuint
OGL4Types::AsOGL4Usage(Base::ResourceBase::Usage usage, Base::ResourceBase::Access access)
{
	switch (usage)
	{
	case ResourceBase::UsageImmutable:
		switch (access)
		{
		case ResourceBase::AccessRead:
			return GL_STATIC_READ;
		case ResourceBase::AccessWrite:
		case ResourceBase::AccessReadWrite:
			return GL_STATIC_COPY;
		case ResourceBase::AccessNone:
			return GL_STATIC_DRAW;
		}
		return GL_STATIC_DRAW;
	case ResourceBase::UsageCpu:
		switch (access)
		{
		case ResourceBase::AccessRead:
			return GL_STREAM_READ;
		case ResourceBase::AccessWrite:
		case ResourceBase::AccessReadWrite:
			return GL_STREAM_COPY;
		case ResourceBase::AccessNone:
			return GL_STREAM_DRAW;
		}
	case ResourceBase::UsageDynamic:
		switch (access)
		{
		case ResourceBase::AccessRead:
			return GL_DYNAMIC_READ;
		case ResourceBase::AccessWrite:
		case ResourceBase::AccessReadWrite:
			return GL_DYNAMIC_COPY;
		case ResourceBase::AccessNone:
			return GL_DYNAMIC_DRAW;
		}
	default:
		n_error("OGL4Types::AsOGL4Usage: invalid usage flag!");
		return 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
GLuint
OGL4Types::AsOGL4Syncing(Base::ResourceBase::Syncing syncing)
{
	switch (syncing)
	{
	case ResourceBase::SyncingSimple:
		return 0;
	case ResourceBase::SyncingPersistent:
		return GL_MAP_PERSISTENT_BIT;
	case ResourceBase::SyncingCoherentPersistent:
		return GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

	default:
		n_error("OGL4Types::AsOGL4Syncing: invalid syncing flag!");
		return 0;
	}
}

//------------------------------------------------------------------------------
/**
*/
GLuint
OGL4Types::AsByteSize( GLuint semantic )
{

	switch (semantic)
	{
	case VertexComponentBase::Position:
        return sizeof(GLfloat) * 4;
	case VertexComponentBase::Normal:
        return sizeof(GLfloat) * 3;
	case VertexComponentBase::Tangent:
        return sizeof(GLfloat) * 3;
	case VertexComponentBase::Binormal:
        return sizeof(GLfloat) * 3;
	case VertexComponentBase::TexCoord1:
        return sizeof(GLfloat) * 2;
	case VertexComponentBase::Color:
        return sizeof(GLint);
	case VertexComponentBase::SkinWeights:
        return sizeof(GLfloat) * 4;
	case VertexComponentBase::SkinJIndices:
        return sizeof(GLint) * 4;
	default:
		n_error("Unknown vertex input semantic!");
		return 0;
	}
}



} // namespace OpenGL4
