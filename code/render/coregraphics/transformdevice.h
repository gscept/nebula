#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::TransformDevice
    
    Manages global transform matrices and their combinations. Input transforms 
    are the view transform (transforms from world to view space),
    the projection transform (describes the projection from view space
    into projection space (pre-div-z)) and the current model matrix
    (transforms from model to world space). From these input transforms,
    the TransformDevice computes all useful combinations and
    inverted versions.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __VULKAN__
#include "coregraphics/vk/vktransformdevice.h"
namespace CoreGraphics
{
class TransformDevice : public Vulkan::VkTransformDevice
{
	__DeclareClass(TransformDevice);
	__DeclareSingleton(TransformDevice);
public:
	/// constructor
	TransformDevice();
	/// destructor
	virtual ~TransformDevice();
};
}
#else
#error "TransformDevice class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

