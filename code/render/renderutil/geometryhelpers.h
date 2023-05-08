#pragma once
//------------------------------------------------------------------------------
/**
    Helpers to create geometry

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/buffer.h"
#include "coregraphics/primitivegroup.h"
#include "coregraphics/primitivetopology.h"
namespace RenderUtil
{

struct Geometry
{
    CoreGraphics::BufferId vertexBuffer;
    CoreGraphics::BufferId indexBuffer;
    CoreGraphics::PrimitiveGroup primitiveGroup;
    CoreGraphics::PrimitiveTopology::Code topology;
};

class GeometryHelpers
{
public:
    /// Create rectangle
    static CoreGraphics::MeshId CreateRectangle();
    /// Create circle (disk) with a set amount of points
    static CoreGraphics::MeshId CreateDisk(SizeT numPoints);
};

} // namespace RenderUtil
