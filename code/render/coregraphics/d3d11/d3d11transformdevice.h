#pragma once
//------------------------------------------------------------------------------
/**
    @class Direct3D11::D3D11TransformDevice
    
    D3D11 version of TransformDevice.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shaderstate.h"
#include "core/singleton.h"
#include "lighting/csmutil.h"

namespace Direct3D11
{
class D3D11TransformDevice : public Base::TransformDeviceBase
{
    __DeclareClass(D3D11TransformDevice);
    __DeclareSingleton(D3D11TransformDevice);
public:
    /// constructor
    D3D11TransformDevice();
    /// destructor
    virtual ~D3D11TransformDevice();

    /// open the transform device
    bool Open();
    /// close the transform device
    void Close();

    /// updates shared shader variables dependent on view matrix
    void ApplyViewSettings();
    /// apply any model transform needed, implementation is platform dependent
	void ApplyModelTransforms(const Ptr<CoreGraphics::ShaderInstance>& shdInst);

	/// set cascade view projection array (for CSM)
	void SetCSMSplitMatrices(const Math::matrix44* matrices, SizeT count);

private:

	Math::matrix44 csmSplitMatrices[Lighting::CSMUtil::NumCascades];
    CoreGraphics::ShaderVariable::Semantic mvpSemantic;
    CoreGraphics::ShaderVariable::Semantic modelSemantic;
    CoreGraphics::ShaderVariable::Semantic modelViewSemantic;
    CoreGraphics::ShaderVariable::Semantic viewSemantic; 
    CoreGraphics::ShaderVariable::Semantic invViewSemantic;
	CoreGraphics::ShaderVariable::Semantic invModelSemantic;
    CoreGraphics::ShaderVariable::Semantic invModelViewSemantic;
    CoreGraphics::ShaderVariable::Semantic viewProjSemantic;
	CoreGraphics::ShaderVariable::Semantic invViewProjSemantic;
    CoreGraphics::ShaderVariable::Semantic eyePosSemantic;
    CoreGraphics::ShaderVariable::Semantic halfPixelSizeSemantic;
    CoreGraphics::ShaderVariable::Semantic projSemantic;
    CoreGraphics::ShaderVariable::Semantic invProjectionSemantic;
	CoreGraphics::ShaderVariable::Semantic csmSplitMatricesSemantic;
};

} // namespace Direct3D11
//------------------------------------------------------------------------------
    