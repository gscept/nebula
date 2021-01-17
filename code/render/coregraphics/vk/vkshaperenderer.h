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
    void DrawShapes();

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
    void DrawBufferedPrimitives();
    /// draw buffered indexed primtives
    void DrawBufferedIndexedPrimitives();

    /// grow index buffer
    void GrowIndexBuffer();
    /// grow vertex buffer
    void GrowVertexBuffer();

    /// draw a shape
    void DrawSimpleShape(const Math::mat4& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::vec4& color);
    /// draw debug mesh
    void DrawMesh(const Math::mat4& modelTransform, const CoreGraphics::MeshId mesh, const Math::vec4& color);
    /// draw primitives
    void DrawPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT vertexWidth, const Math::vec4& color);
    /// draw indexed primitives
    void DrawIndexedPrimitives(const Math::mat4& modelTransform, CoreGraphics::PrimitiveTopology::Code topology, SizeT numPrimitives, const void* vertices, SizeT numVertices, SizeT vertexWidth, const void* indices, CoreGraphics::IndexType::Code indexType, const Math::vec4& color);

    /// create a box shape
    void CreateBoxShape();
    /// create a sphere shape
    void CreateSphereShape();
    /// create a cylinder shape
    void CreateCylinderShape();
    /// create a torus shape
    void CreateTorusShape();
    /// create a cone shape
    void CreateConeShape();

    CoreGraphics::ShaderProgramId programs[CoreGraphics::RenderShape::NumDepthFlags * 2];

    Util::FixedArray<Resources::ResourceId> shapeMeshResources;
    CoreGraphics::ShaderId shapeShader;

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

    SizeT numPrimitives;
    SizeT numIndices;

    struct IndexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::vec4> colors;
        Util::Array<Math::mat4> transforms;
    } indexed[CoreGraphics::PrimitiveTopology::NumTopologies];

    struct UnindexedDraws
    {
        Util::Array<CoreGraphics::PrimitiveGroup> primitives;
        Util::Array<Math::vec4> colors;
        Util::Array<Math::mat4> transforms;
    } unindexed[CoreGraphics::PrimitiveTopology::NumTopologies];
};
} // namespace Vulkan
