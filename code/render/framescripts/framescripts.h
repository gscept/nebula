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
    uint index;
    CoreGraphics::TextureId tex;
    CoreGraphics::PipelineStage stage;
};

struct TextureImport
{
    CoreGraphics::TextureId tex;
    CoreGraphics::PipelineStage stage = CoreGraphics::PipelineStage::AllShadersRead;

    explicit TextureImport(const CoreGraphics::TextureId id)
    {
        this->tex = id;
    }

    static TextureImport FromExport(const TextureExport& exp)
    {
        auto ret = TextureImport(exp.tex);
        ret.stage = exp.stage;
        return ret;
    }
};

/// Draw
void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const IndexT bufferIndex);
/// Draw instanced
void DrawBatch(const CoreGraphics::CmdBufferId cmdBuf, MaterialTemplates::BatchGroup batch, const Graphics::GraphicsEntityId id, const SizeT numInstances, const IndexT baseInstance, const IndexT bufferIndex);

} // namespace Frame
