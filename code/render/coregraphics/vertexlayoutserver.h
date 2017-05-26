#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexLayoutServer    
    
    The VertexLayoutServer creates VertexLayout objects shared by their
    vertex component signature. On some platforms it is more efficient
    to share VertexLayout objects across meshes with identical
    vertex structure. Note that there is no way to manually discard
    vertex components. Vertex components stay alive for the life time
    of the application until the Close() method of the VertexLayoutServer
    is called.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#if __OGL4__
#include "coregraphics/ogl4/ogl4vertexlayoutserver.h"
namespace CoreGraphics
{
class VertexLayoutServer : public OpenGL4::OGL4VertexLayoutServer
{
	__DeclareClass(VertexLayoutServer);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkvertexlayoutserver.h"
namespace CoreGraphics
{
class VertexLayoutServer : public Vulkan::VkVertexLayoutServer
{
	__DeclareClass(VertexLayoutServer);
};
}
#else
#include "coregraphics/base/vertexlayoutserverbase.h"
namespace CoreGraphics
{
class VertexLayoutServer : public Base::VertexLayoutServerBase
{
    __DeclareClass(VertexLayoutServer);
};
}
#endif
//------------------------------------------------------------------------------



    