#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::PixelFormat
    
    Pixel format enumeration.

    FIXME: use DX10 notations (more flexible but less readable...)

    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/types.h"
#include "util/string.h"
#include "coregraphics/config.h"

namespace CoreGraphics
{
class PixelFormat
{
public:
    /// enums
    enum Code
    {
        R8G8B8X8 = 0,
        R8G8B8,
        R8G8B8A8,
        R5G6B5,
        R5G5B5A1,
        R4G4B4A4,
        DXT1,
        DXT1A,
        DXT3,
        DXT5,
        DXT1sRGB,
        DXT1AsRGB,
        DXT3sRGB,
        DXT5sRGB,
        BC4,
        BC5,
        BC7,
        BC7sRGB,
        R8,
        R16F,                       // 16 bit float, red only
        R16,                        // 16 bit integer
        R16G16F,                    // 32 bit float, 16 bit red, 16 bit green
        R16G16,                     // 32 bit integer, 16 bit red, 16 bit green
        R16G16B16A16F,              // 64 bit float, 16 bit rgba each
        R16G16B16A16,               // 64 bit int, 16 bit rgba each
        R32F,                       // 32 bit float, red only
        R32,                        // 32 bit int, red only
        R32G32F,                    // 64 bit float, 32 bit red, 32 bit green
        R32G32,                     // 64 bit, 32 bit red, 32 bit green
        R32G32B32A32F,              // 128 bit float, 32 bit rgba each
        R32G32B32A32,               // 128 bit integer, 32 bit rgba each
        R32G32B32F,                 // 96 bit float, 32 bit rgb each
        R32G32B32,                  // 96 bit integer, 32 bit rgb each
        R11G11B10F,                 // 32 bit float, 11 bits red and green, 10 bit blue
        SRGBA8,
        R10G10B10X2,
        R10G10B10A2,
        D24X8,
        D24S8,

        D32S8,
        D16S8,
        D32,
        S8,

        B8G8R8,
        B8G8R8A8,
        B5G6R5,
        B5G6R5A1,
        B4G4R4A4,


        NumPixelFormats,
        InvalidPixelFormat,
    };

    /// convert from string
    static Code FromString(const Util::String& str);
    /// convert to string
    static Util::String ToString(Code code);
    /// convert to byte size
    static uint ToSize(Code code);
    /// convert to number of channesl
    static uint ToChannels(Code code);
    /// figure out if block compressed
    static bool ToCompressed(Code code);
    /// calculate texel dimensions
    static SizeT ToTexelSize(Code code);
    /// Calculate texel block size
    static SizeT ToBlockSize(Code code);
    /// return true if depth format
    static bool IsDepthFormat(Code code);
    /// return true if depth format
    static bool IsStencilFormat(Code code);
    /// Return true if depth/stencil format
    static bool IsDepthStencilFormat(Code code);

    /// Return Image bits from format
    static CoreGraphics::ImageBits ToImageBits(Code code);
};

} // namespace CoreGraphics
//------------------------------------------------------------------------------

