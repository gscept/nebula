#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamTextureSaver
    
    Allows to save texture data in a standard file format into a stream.    
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11streamtexturesaver.h"
namespace CoreGraphics
{
class StreamTextureSaver : public Direct3D11::D3D11StreamTextureSaver
{
	__DeclareClass(StreamTextureSaver);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4streamtexturesaver.h"
namespace CoreGraphics
{
class StreamTextureSaver : public OpenGL4::OGL4StreamTextureSaver
{
	__DeclareClass(StreamTextureSaver);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkstreamtexturesaver.h"
namespace CoreGraphics
{
class StreamTextureSaver : public Vulkan::VkStreamTextureSaver
{
	__DeclareClass(StreamTextureSaver);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9streamtexturesaver.h"
namespace CoreGraphics
{
class StreamTextureSaver : public Direct3D9::D3D9StreamTextureSaver
{
    __DeclareClass(StreamTextureSaver);
};
}

#else
#error "StreamTextureSaver class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

    