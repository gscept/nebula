#pragma once
//------------------------------------------------------------------------------
/**
    Implements the transform device to manage object and camera transforms in Vulkan.
    
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "coregraphics/base/transformdevicebase.h"
#include "coregraphics/shader.h"
#include "coregraphics/graphicsdevice.h"
#include "coregraphics/resourcetable.h"
#include "shared.h"

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
    void SetProjTransform(const Math::mat4& m);

    /// updates shared shader variables dependent on view matrix
    void ApplyViewSettings();
    /// update the csm matrix block
    void ApplyShadowSettings(const Shared::ShadowMatrixBlock& block);

private:

    Math::mat4 viewMatrixArray[6];

    IndexT viewVar;
    IndexT invViewVar;
    IndexT viewProjVar;
    IndexT invViewProjVar;
    IndexT projVar;
    IndexT invProjVar;
    IndexT eyePosVar;
    IndexT focalLengthNearFarVar;
    IndexT viewMatricesVar;
    IndexT timeAndRandomVar;
    IndexT nearFarPlaneVar;

    IndexT shadowCameraBlockVar;
    CoreGraphics::BufferId viewConstants;

    IndexT viewConstantsSlot;
    IndexT shadowConstantsSlot;
    CoreGraphics::ResourcePipelineId tableLayout;
};

} // namespace Vulkan
