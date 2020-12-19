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

    /// bind descriptors for view in the graphics pipeline
    void BindCameraDescriptorSetsGraphics();
    /// bind descriptors for view in the compute pipeline
    void BindCameraDescriptorSetsCompute(const CoreGraphics::QueueType queue = CoreGraphics::GraphicsQueueType);
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
    uint32_t frameOffset;

    IndexT shadowCameraBlockVar;
    CoreGraphics::BufferId viewConstants;

    Util::FixedArray<CoreGraphics::ResourceTableId> viewTables; 
    IndexT viewConstantsSlot;
    IndexT shadowConstantsSlot;
    CoreGraphics::ResourcePipelineId tableLayout;
};

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSetsGraphics()
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::SetResourceTable(this->viewTables[bufferedFrameIndex], NEBULA_FRAME_GROUP, CoreGraphics::GraphicsPipeline, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
inline void
VkTransformDevice::BindCameraDescriptorSetsCompute(const CoreGraphics::QueueType queue)
{
    IndexT bufferedFrameIndex = CoreGraphics::GetBufferedFrameIndex();
    CoreGraphics::SetResourceTable(this->viewTables[bufferedFrameIndex], NEBULA_FRAME_GROUP, CoreGraphics::ComputePipeline, nullptr, queue);
}
} // namespace Vulkan
