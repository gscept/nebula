//------------------------------------------------------------------------------
//  vertexlayoutserver.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexlayoutserver.h"
#if __OGL4__
__ImplementClass(CoreGraphics::VertexLayoutServer, 'VLSV', OpenGL4::OGL4VertexLayoutServer);
#elif __VULKAN__
__ImplementClass(CoreGraphics::VertexLayoutServer, 'VLSV', Vulkan::VkVertexLayoutServer);
#else
__ImplementClass(CoreGraphics::VertexLayoutServer, 'VLSV', Base::VertexLayoutServerBase);
#endif

