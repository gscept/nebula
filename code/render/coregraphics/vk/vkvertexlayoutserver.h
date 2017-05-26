#pragma once
//------------------------------------------------------------------------------
/**
	Implements a vertex layout server for Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/vertexlayoutserverbase.h"
namespace Vulkan
{
class VkVertexLayoutServer : public Base::VertexLayoutServerBase
{
	__DeclareClass(VkVertexLayoutServer);
public:
	/// constructor
	VkVertexLayoutServer();
	/// destructor
	virtual ~VkVertexLayoutServer();
private:
};
} // namespace Vulkan