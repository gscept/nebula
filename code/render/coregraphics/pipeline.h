#pragma once
//------------------------------------------------------------------------------
/**
    A pipeline object describes the GPU state required to 
    perform a compute or graphics job

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "shader.h"
#include "pass.h"
namespace CoreGraphics
{

ID_24_8_TYPE(PipelineId);

struct PipelineCreateInfo
{
    CoreGraphics::ShaderProgramId shader;
    CoreGraphics::PassId pass;
    uint subpass;
    CoreGraphics::InputAssemblyKey inputAssembly;
};

/// Create new pipeline
PipelineId CreatePipeline(const PipelineCreateInfo& info);
/// Destroy pipeline
void DestroyPipeline(const PipelineId pipeline);

} // namespace CoreGraphics
