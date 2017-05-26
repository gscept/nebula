#pragma once
//------------------------------------------------------------------------------
/**
	Implements the transform device to manage object and camera transforms in Vulkan.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shaderstate.h"
namespace CoreGraphics
{
	class ShaderState;
	class ShaderVariable;
	class ConstantBuffer;
}
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
	/// apply any model transform needed, implementation is platform dependent
	void ApplyModelTransforms(const Ptr<CoreGraphics::ShaderState>& shdInst);

	/// bind descriptors for view
	void BindCameraDescriptorSets();
private:

	Math::matrix44 viewMatrixArray[6];

	Ptr<CoreGraphics::ShaderVariable> viewVar;
	Ptr<CoreGraphics::ShaderVariable> invViewVar;
	Ptr<CoreGraphics::ShaderVariable> viewProjVar;
	Ptr<CoreGraphics::ShaderVariable> invViewProjVar;
	Ptr<CoreGraphics::ShaderVariable> projVar;
	Ptr<CoreGraphics::ShaderVariable> invProjVar;
	Ptr<CoreGraphics::ShaderVariable> eyePosVar;
	Ptr<CoreGraphics::ShaderVariable> focalLengthVar;
	Ptr<CoreGraphics::ShaderVariable> viewMatricesVar;
	Ptr<CoreGraphics::ShaderVariable> timeAndRandomVar;

	Ptr<CoreGraphics::ShaderVariable> shadowCameraBlockVar;
	Ptr<CoreGraphics::ShaderState> sharedShader;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSets()
{
	this->sharedShader->Commit();
}

} // namespace Vulkan