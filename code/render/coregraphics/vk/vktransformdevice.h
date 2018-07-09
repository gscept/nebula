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
#include "coregraphics/graphicsdevice.h"

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

	CoreGraphics::ShaderConstantId viewVar;
	CoreGraphics::ShaderConstantId invViewVar;
	CoreGraphics::ShaderConstantId viewProjVar;
	CoreGraphics::ShaderConstantId invViewProjVar;
	CoreGraphics::ShaderConstantId projVar;
	CoreGraphics::ShaderConstantId invProjVar;
	CoreGraphics::ShaderConstantId eyePosVar;
	CoreGraphics::ShaderConstantId focalLengthVar;
	CoreGraphics::ShaderConstantId viewMatricesVar;
	CoreGraphics::ShaderConstantId timeAndRandomVar;

	CoreGraphics::ShaderConstantId shadowCameraBlockVar;
	CoreGraphics::ShaderStateId sharedShader;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSets()
{
	CoreGraphics::SetShaderState(this->sharedShader);
}

} // namespace Vulkan