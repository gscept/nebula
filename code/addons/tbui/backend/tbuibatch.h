#pragma once

#include "coregraphics/buffer.h"
#include "coregraphics/texture.h"
#include "util/array.h"
#include "math/rectangle.h"
#include "tbuivertex.h"

namespace TBUI
{
struct TBUIBatch
{
    CoreGraphics::TextureId texture;
    //CoreGraphics::BufferId vertexBuffer;
    Util::Array<TBUIVertex> vertices;
    Math::intRectangle clipRect;
};
} // namespace TBUI
