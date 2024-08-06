#pragma once
//------------------------------------------------------------------------------
/**
    Include file for frame scripts

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/commandbuffer.h"
#include "graphics/graphicsentity.h"
#include "materials/materialtemplates.h"

namespace Frame
{

struct TextureExport
{
    CoreGraphics::TextureId tex;
    CoreGraphics::PipelineStage stage;
};

/// Draw
void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const IndexT bufferIndex);
/// Draw instanced
void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance, const IndexT bufferIndex);

} // namespace Frame
