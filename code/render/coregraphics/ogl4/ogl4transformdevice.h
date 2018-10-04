#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4TransformDevice
    
    OGL4 version of TransformDevice.
    
    (C) 2007 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shadervariable.h"
#include "coregraphics/shaderstate.h"
#include "lighting/csmutil.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{
class OGL4TransformDevice : public Base::TransformDeviceBase
{
    __DeclareClass(OGL4TransformDevice);
    __DeclareSingleton(OGL4TransformDevice);
public:
    /// constructor
    OGL4TransformDevice();
    /// destructor
    virtual ~OGL4TransformDevice();

    /// open the transform device
    bool Open();
    /// close the transform device
    void Close();

    /// updates shared shader variables dependent on view matrix
    void ApplyViewSettings();
    /// apply any model transform needed, implementation is platform dependent
    void ApplyModelTransforms(const Ptr<CoreGraphics::Shader>& shdInst);
	/// set view matrix array
	void ApplyViewMatrixArray(const Math::matrix44* matrices, SizeT num);

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
    Ptr<CoreGraphics::ShaderVariable> cameraBlockVar;

    Ptr<CoreGraphics::ShaderVariable> shadowCameraBlockVar;
	Ptr<CoreGraphics::Shader> sharedShader;
    Ptr<CoreGraphics::ConstantBuffer> cameraBuffer;
    Ptr<CoreGraphics::ConstantBuffer> shadowCameraBuffer;
};

} // namespace OpenGL4
//------------------------------------------------------------------------------
    