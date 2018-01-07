#pragma once
//------------------------------------------------------------------------------
/**
	Implements the transform device to manage object and camera transforms in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shader.h"

namespace Vulkan
{
class VkTransformDevice : public Base::TransformDeviceBase
{
	__DeclareClass(VkTransformDevice);
	__DeclareSingleton(VkTransformDevice);
public:
	/// constructor
	VkTransformDevice();
	/// destructor
	virtual ~VkTransformDevice();

	/// open the transform device
	bool Open();
	/// close the transform device
	void Close();

	/// updates shared shader variables dependent on view matrix
	void ApplyViewSettings();

	/// bind descriptors for view
	void BindCameraDescriptorSets();
private:

	Math::matrix44 viewMatrixArray[6];

	CoreGraphics::ShaderVariableId viewVar;
	CoreGraphics::ShaderVariableId invViewVar;
	CoreGraphics::ShaderVariableId viewProjVar;
	CoreGraphics::ShaderVariableId invViewProjVar;
	CoreGraphics::ShaderVariableId projVar;
	CoreGraphics::ShaderVariableId invProjVar;
	CoreGraphics::ShaderVariableId eyePosVar;
	CoreGraphics::ShaderVariableId focalLengthVar;
	CoreGraphics::ShaderVariableId viewMatricesVar;
	CoreGraphics::ShaderVariableId timeAndRandomVar;

	CoreGraphics::ShaderVariableId shadowCameraBlockVar;
	CoreGraphics::ShaderStateId sharedShader;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSets()
{
	ShaderStateApply(this->sharedShader);
}

} // namespace Vulkan