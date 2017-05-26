//------------------------------------------------------------------------------
//  streamtexturesaver.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/streamtexturesaver.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTextureSaver, 'STXS', Direct3D11::D3D11StreamTextureSaver);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTextureSaver, 'STXS', OpenGL4::OGL4StreamTextureSaver);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTextureSaver, 'STXS', Vulkan::VkStreamTextureSaver);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::StreamTextureSaver, 'STXS', Direct3D9::D3D9StreamTextureSaver);
}
#else
#error "StreamTextureSaver class not implemented on this platform!"
#endif
