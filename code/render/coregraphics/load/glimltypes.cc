//------------------------------------------------------------------------------
//  glimltypes.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "glimltypes.h"


namespace CoreGraphics
{

//------------------------------------------------------------------------------
/**
    Convert a gliml format/dds format into a pixel format code.
*/
CoreGraphics::PixelFormat::Code
Gliml::ToPixelFormat(gliml::context const& ctx)
{
    if (ctx.image_format() == GLIML_FOURCC_DXT10)
    {
        switch ((gliml::DXGI_FORMAT)ctx.image_internal_format())
        {
        case gliml::DXGI_FORMAT_R8G8B8A8_UNORM:         return PixelFormat::R8G8B8A8;
        case gliml::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:    return PixelFormat::SRGBA8;
        case gliml::DXGI_FORMAT_BC1_UNORM:              return PixelFormat::DXT1;
        case gliml::DXGI_FORMAT_BC2_UNORM:              return PixelFormat::DXT3;
        case gliml::DXGI_FORMAT_BC3_UNORM:              return PixelFormat::DXT5;
        case gliml::DXGI_FORMAT_BC1_UNORM_SRGB:         return PixelFormat::DXT1sRGB;
        case gliml::DXGI_FORMAT_BC2_UNORM_SRGB:         return PixelFormat::DXT3sRGB;
        case gliml::DXGI_FORMAT_BC3_UNORM_SRGB:         return PixelFormat::DXT5sRGB;
        case gliml::DXGI_FORMAT_BC5_UNORM:              return PixelFormat::BC5;
        case gliml::DXGI_FORMAT_BC7_UNORM:              return PixelFormat::BC7;
        case gliml::DXGI_FORMAT_BC7_UNORM_SRGB:         return PixelFormat::BC7sRGB;

        case gliml::DXGI_FORMAT_R8_TYPELESS:
        case gliml::DXGI_FORMAT_R8_UINT:
        case gliml::DXGI_FORMAT_R8_SNORM:
        case gliml::DXGI_FORMAT_R8_SINT:
        case gliml::DXGI_FORMAT_R8_UNORM:               return PixelFormat::R8;

        case gliml::DXGI_FORMAT_R16_FLOAT:              return PixelFormat::R16F;

        case gliml::DXGI_FORMAT_R16_SINT:
        case gliml::DXGI_FORMAT_R16_UINT:
        case gliml::DXGI_FORMAT_R16_TYPELESS:
        case gliml::DXGI_FORMAT_R16_UNORM:              return PixelFormat::R16;

        case gliml::DXGI_FORMAT_R16G16_FLOAT:           return PixelFormat::R16G16F;

        case gliml::DXGI_FORMAT_R16G16_SINT:
        case gliml::DXGI_FORMAT_R16G16_UINT:
        case gliml::DXGI_FORMAT_R16G16_TYPELESS:
        case gliml::DXGI_FORMAT_R16G16_UNORM:           return PixelFormat::R16G16;

        case gliml::DXGI_FORMAT_R16G16B16A16_FLOAT:     return PixelFormat::R16G16B16A16F;

        case gliml::DXGI_FORMAT_R16G16B16A16_SINT:
        case gliml::DXGI_FORMAT_R16G16B16A16_UINT:
        case gliml::DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case gliml::DXGI_FORMAT_R16G16B16A16_UNORM:     return PixelFormat::R16G16B16A16;

        case gliml::DXGI_FORMAT_R32_FLOAT:              return PixelFormat::R32F;


        case gliml::DXGI_FORMAT_R32_SINT:
        case gliml::DXGI_FORMAT_R32_UINT:
        case gliml::DXGI_FORMAT_R32_TYPELESS:           return PixelFormat::R32;
        case gliml::DXGI_FORMAT_R32G32_FLOAT:           return PixelFormat::R32G32F;

        case gliml::DXGI_FORMAT_R32G32_SINT:
        case gliml::DXGI_FORMAT_R32G32_TYPELESS:
        case gliml::DXGI_FORMAT_R32G32_UINT:            return PixelFormat::R32G32;

        case gliml::DXGI_FORMAT_R32G32B32A32_FLOAT:     return PixelFormat::R32G32B32A32F;

        case gliml::DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case gliml::DXGI_FORMAT_R32G32B32A32_SINT:
        case gliml::DXGI_FORMAT_R32G32B32A32_UINT:      return PixelFormat::R32G32B32A32;

        case gliml::DXGI_FORMAT_R32G32B32_FLOAT:        return PixelFormat::R32G32B32F;

        case gliml::DXGI_FORMAT_R32G32B32_SINT:
        case gliml::DXGI_FORMAT_R32G32B32_TYPELESS:
        case gliml::DXGI_FORMAT_R32G32B32_UINT:         return PixelFormat::R32G32B32;

        case gliml::DXGI_FORMAT_R11G11B10_FLOAT:        return PixelFormat::R11G11B10F;

        // not sure about this one
        case gliml::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:     return PixelFormat::R10G10B10X2;

        case gliml::DXGI_FORMAT_R10G10B10A2_UINT:
        case gliml::DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case gliml::DXGI_FORMAT_R10G10B10A2_UNORM:      return PixelFormat::R10G10B10A2;

        case gliml::DXGI_FORMAT_D24_UNORM_S8_UINT:      return PixelFormat::D24S8;
        case gliml::DXGI_FORMAT_B8G8R8A8_UNORM:         return PixelFormat::B8G8R8A8;
        case gliml::DXGI_FORMAT_B5G6R5_UNORM:           return PixelFormat::B5G6R5;
        case gliml::DXGI_FORMAT_B5G5R5A1_UNORM:         return PixelFormat::B5G6R5A1;
        case gliml::DXGI_FORMAT_B4G4R4A4_UNORM:         return PixelFormat::B4G4R4A4;
        default:
            return PixelFormat::InvalidPixelFormat;
        }
    }
    else
    {
        switch (ctx.image_internal_format())
        {
            case GLIML_GL_RGBA:
            {
                switch (ctx.image_type())
                {
                    case GLIML_GL_UNSIGNED_BYTE:            return PixelFormat::R8G8B8A8;
                    case GLIML_GL_UNSIGNED_SHORT_4_4_4_4:   return PixelFormat::R4G4B4A4;
                    case GLIML_GL_UNSIGNED_SHORT_5_5_5_1:   return PixelFormat::R5G5B5A1;
                    default:
                    {
                        n_error("Gliml::ToPixelFormat(): invalid image_type %d for RGBA", ctx.image_type());
                        return PixelFormat::InvalidPixelFormat;
                    }
                }
            }
            case GLIML_GL_BGRA:
            {
                switch (ctx.image_type())
                {
                    case GLIML_GL_UNSIGNED_BYTE:            return PixelFormat::B8G8R8A8;
                    case GLIML_GL_UNSIGNED_SHORT_4_4_4_4:   return PixelFormat::B4G4R4A4;
                    case GLIML_GL_UNSIGNED_SHORT_5_5_5_1:   return PixelFormat::B5G6R5A1;
                    default:
                    {
                        n_error("Gliml::ToPixelFormat(): invalid image_type %d for BGRA", ctx.image_type());
                        return PixelFormat::InvalidPixelFormat;
                    }
                }
            }
            case GLIML_GL_RGB:
            {
                switch (ctx.image_type())
                {
                    case GLIML_GL_UNSIGNED_BYTE:            return PixelFormat::R8G8B8;
                    case GLIML_GL_UNSIGNED_SHORT_5_6_5:     return PixelFormat::R5G6B5;
                    default:
                    {
                        n_error("Gliml::ToPixelFormat(): invalid image_type %d for RGB", ctx.image_type());
                        return PixelFormat::InvalidPixelFormat;
                    }
                }
            }
            case GLIML_GL_BGR:
            {
                switch (ctx.image_type())
                {
                    case GLIML_GL_UNSIGNED_BYTE:            return PixelFormat::B8G8R8;
                    case GLIML_GL_UNSIGNED_SHORT_5_6_5:     return PixelFormat::B5G6R5;
                    default:
                    {
                        n_error("Gliml::ToPixelFormat(): invalid image_type %d for BGR", ctx.image_type());
                        return PixelFormat::InvalidPixelFormat;
                    }
                }
            }
            case GLIML_FOURCC_BC7U:                         return PixelFormat::BC7;
            case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:    return PixelFormat::DXT1;
            case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:    return PixelFormat::DXT3;
            case GLIML_GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:    return PixelFormat::DXT5;
            default:
            {
                n_error("Gliml::ToPixelFormat(): invalid image_type %d/%d", ctx.image_format(), ctx.image_internal_format());
                return PixelFormat::InvalidPixelFormat;
            }
        }
    }
}
}
