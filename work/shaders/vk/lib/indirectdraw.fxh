//------------------------------------------------------------------------------
//  indirectdraw.fxh
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef INDIRECT_DRAW_FXH
#define INDIRECT_DRAW_FXH

// struct defined by the Vulkan spec
struct DrawCommand
{
    uint vertexCount;
    uint instanceCount;
    uint startVertex;
    uint startInstance;
};

struct DrawIndexedCommand
{
    uint indexCount;
    uint instanceCount;
    uint startIndex;
    uint offsetVertex;
    uint startInstance;
};

#endif // INDIRECT_DRAW_FXH
