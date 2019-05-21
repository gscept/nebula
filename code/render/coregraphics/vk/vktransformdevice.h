#pragma once
//------------------------------------------------------------------------------
/**
	Implements the transform device to manage object and camera transforms in Vulkan.
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shader.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/constantbuffer.h"
#include "coregraphics/resourcetable.h"

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

	/// set projection transform
	void SetProjTransform(const Math::matrix44& m);

	/// updates shared shader variables dependent on view matrix
	void ApplyViewSettings();

	/// bind descriptors for view in the graphics pipeline
	void BindCameraDescriptorSetsGraphics();
	/// bind descriptors for view in the compute pipeline
	void BindCameraDescriptorSetsCompute();
private:

	Math::matrix44 viewMatrixArray[6];

	CoreGraphics::ConstantBinding viewVar;
	CoreGraphics::ConstantBinding invViewVar;
	CoreGraphics::ConstantBinding viewProjVar;
	CoreGraphics::ConstantBinding invViewProjVar;
	CoreGraphics::ConstantBinding projVar;
	CoreGraphics::ConstantBinding invProjVar;
	CoreGraphics::ConstantBinding eyePosVar;
	CoreGraphics::ConstantBinding focalLengthNearFarVar;
	CoreGraphics::ConstantBinding viewMatricesVar;
	CoreGraphics::ConstantBinding timeAndRandomVar;
	CoreGraphics::ConstantBinding nearFarPlaneVar;

	CoreGraphics::ConstantBinding shadowCameraBlockVar;
	CoreGraphics::ConstantBufferId viewConstants;
	CoreGraphics::ResourceTableId viewTable;
	CoreGraphics::ResourcePipelineId tableLayout;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSetsGraphics()
{
	CoreGraphics::SetResourceTable(this->viewTable, NEBULA_FRAME_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSetsCompute()
{
	CoreGraphics::SetResourceTable(this->viewTable, NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr);
}
} // namespace Vulkan