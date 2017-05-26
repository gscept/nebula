#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan depth-stencil renderable texture.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/depthstenciltargetbase.h"
namespace Vulkan
{
class VkDepthStencilTarget : public Base::DepthStencilTargetBase
{
	__DeclareClass(VkDepthStencilTarget);
public:
	/// constructor
	VkDepthStencilTarget();
	/// destructor
	virtual ~VkDepthStencilTarget();

	/// setup depth-stencil target
	void Setup();
	/// discard depth-stencil target
	void Discard();

	/// called after we change the display size
	void OnDisplayResized(SizeT width, SizeT height);

	/// begins pass
	void BeginPass();
	/// ends pass
	void EndPass();

	/// get the vulkan viewports
	const VkViewport& GetVkViewport() const;
	/// get the vulkan scissor rectangles
	const VkRect2D& GetVkScissorRect() const;
	/// return handle to the view
	VkImageView GetVkImageView();
private:

	VkViewport viewport;
	VkRect2D scissor;
	VkImage image;
	VkDeviceMemory mem;
	VkImageView view;
};

//------------------------------------------------------------------------------
/**
*/
inline const VkViewport&
VkDepthStencilTarget::GetVkViewport() const
{
	return this->viewport;
}

//------------------------------------------------------------------------------
/**
*/
inline const VkRect2D&
VkDepthStencilTarget::GetVkScissorRect() const
{
	return this->scissor;
}

//------------------------------------------------------------------------------
/**
*/
inline VkImageView
VkDepthStencilTarget::GetVkImageView()
{
	return this->view;
}

} // namespace Vulkan