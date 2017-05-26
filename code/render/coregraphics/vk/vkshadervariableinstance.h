#pragma once
//------------------------------------------------------------------------------
/**
	Implements a shader variable instance in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/shadervariableinstancebase.h"
namespace Vulkan
{
class VkShaderVariableInstance : public Base::ShaderVariableInstanceBase
{
	__DeclareClass(VkShaderVariableInstance);
public:
	/// constructor
	VkShaderVariableInstance();
	/// destructor
	virtual ~VkShaderVariableInstance();

	/// apply local value to shader variable
	void Apply();
	/// apply local value to specific shader variable
	void ApplyTo(const Ptr<CoreGraphics::ShaderVariable>& var);

protected:
	friend class Base::ShaderVariableBase;
};
} // namespace Vulkan