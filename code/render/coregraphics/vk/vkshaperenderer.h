#pragma once
//------------------------------------------------------------------------------
/**
    Implements a Vulkan immediate shape and primitive renderer (debug meshes, random primitives, wireframes etc)
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/base/shaperendererbase.h"
#include "coregraphics/vertexlayout.h"
#include "coregraphics/resourcetable.h"
#include "util/fixedarray.h"
#include "frame/framecode.h"

namespace Vulkan
{
class VkShapeRenderer : public Base::ShapeRendererBase
{
    __DeclareClass(VkShapeRenderer);
public:
    /// constructor
    VkShapeRenderer();
    /// destructor
    virtual ~VkShapeRenderer();

    /// open the shape renderer
    void Open();
    /// close the shape renderer
    void Close();

    /// draw attached shapes and clear deferred stack, must be called inside render loop
    void DrawShapes(const CoreGraphics::CmdBufferId cmdBuf);

    /// maximum amount of vertices to be rendered by drawprimitives and drawindexedprimitives
    static const int MaxNumVertices = 262140;
    /// maximum amount of indices to be rendered by drawprimitives and drawindexedprimitives
    static const int MaxNumIndices = 262140;

    /// maximum size for primitive size (4 floats for position, 4 floats for color)
    static const int MaxVertexWidth = 8 * sizeof(float);
    /// maximum size for an index
    static const int MaxIndexWidth = sizeof(int);
private:

    /// draw buffered primitives
    void DrawBufferedPrimitives(const CoreGraphics::CmdBufferId cmdBuf);
    /// draw buffered indexed primtives
    void DrawBufferedIndexedPrimitives(const CoreGraphics::CmdBufferId cmdBuf);

    /// grow index buffer
    void GrowIndexBuffer();
    /// grow vertex buffer
    void GrowVertexBuffer();

    /// draw a shape
    void DrawSimpleShape(const CoreGraphics::CmdBufferId cmdBuf, const Math::mat4& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::vec4& color, const float lineThickness);
    /// draw debug mesh
    void DrawMesh(const CoreGraphics::CmdBufferId cmdBuf, const Math::mat4& modelTransform, const CoreGraphics::MeshId mesh, const Math::vec4& color, const float lineThickness);
    /// draw primitives
    void DrawPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numVertices, const void* vertices, const float lineThickness);
    /// draw indexed primitives
    void DrawIndexedPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numVertices, const void* vertices, SizeT numIndices, const void* indices, CoreGraphics::IndexType::Code indexType, const float lineThickness);

    Util::FixedArray<Resources::ResourceId> shapeMeshResources;
    Util::FixedArray<CoreGraphics::MeshId> meshes;

    Util::Array<CoreGraphics::VertexComponent> comps;
    static const SizeT MaxVertexIndexBuffers = 2;
    CoreGraphics::BufferId ibos[MaxVertexIndexBuffers];
    CoreGraphics::BufferId vbos[MaxVertexIndexBuffers];
    byte* vertexBufferPtr;
    byte* indexBufferPtr;
    IndexT indexBufferActiveIndex;
    IndexT vertexBufferActiveIndex;
    SizeT vertexBufferCapacity;
    SizeT indexBufferCapacity;

    CoreGraphics::VertexLayoutId vertexLayout;
    IndexT model;
    IndexT diffuseColor;
    IndexT lineWidth;

    SizeT vertexBufferOffset;
    SizeT indexBufferOffset;

    Memory::ArenaAllocator<sizeof(Frame::FrameCode)> frameOpAllocator;

    struct IndexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::mat4> transforms;
        Util::Array<uint64> firstVertexOffset;
        Util::Array<uint64> firstIndexOffset;
        Util::Array<CoreGraphics::IndexType::Code> indexType;
        Util::Array<float> lineThicknesses;
    } indexed[CoreGraphics::PrimitiveTopology::NumTopologies];

    struct UnindexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::mat4> transforms;
        Util::Array<uint64> firstVertexOffset;
        Util::Array<float> lineThicknesses;
    } unindexed[CoreGraphics::PrimitiveTopology::NumTopologies];
};
} // namespace Vulkan
