#pragma once
//------------------------------------------------------------------------------
/**
    Turbobadger UI Batch

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/buffer.h"
#include "coregraphics/texture.h"
#include "util/array.h"
#include "math/rectangle.h"
#include "tbuivertex.h"

//------------------------------------------------------------------------------
namespace TBUI
{
struct TBUIBatch
{
    CoreGraphics::TextureId texture;
    Util::Array<TBUIVertex> vertices;
    Math::intRectangle clipRect;
};
} // namespace TBUI
