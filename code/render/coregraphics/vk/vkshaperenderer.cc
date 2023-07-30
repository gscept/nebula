//------------------------------------------------------------------------------
// vkshaperenderer.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkshaperenderer.h"
#include "coregraphics/mesh.h"
#include "coregraphics/vertexcomponent.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/meshresource.h"
#include "resources/resourceid.h"
#include "resources/resourceserver.h"
#include "frame/framesubgraph.h"

using namespace Base;
using namespace Threading;
using namespace Math;
using namespace CoreGraphics;
using namespace Threading;
using namespace Resources;
namespace Vulkan
{

__ImplementClass(Vulkan::VkShapeRenderer, 'VKSR', Base::ShapeRendererBase);
//------------------------------------------------------------------------------
/**
*/
VkShapeRenderer::VkShapeRenderer() 
    : indexBufferPtr(nullptr)
    , vertexBufferPtr(nullptr)
    , ibos{ InvalidBufferId }
    , vbos{ InvalidBufferId }
    , indexBufferActiveIndex()
    , vertexBufferActiveIndex()
    , indexBufferCapacity(0)
    , vertexBufferCapacity(0)
    , vertexBufferOffset(0)
    , indexBufferOffset(0)

{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
VkShapeRenderer::~VkShapeRenderer()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::Open()
{
    n_assert(!this->IsOpen());

    ShapeRendererBase::Open();

    // create shape shader instance
    CoreGraphics::ShaderId shapesShader = ShaderServer::Instance()->GetShader("shd:shapes.fxb"_atm);
    this->shapeMeshResources.SetSize(CoreGraphics::RenderShape::NumShapeTypes);
    this->meshes.SetSize(CoreGraphics::RenderShape::NumShapeTypes);

    /*
            Box,
        Sphere,
        Cylinder,
        Torus,
        Cone,
        Arrow,
    */
    const Util::String meshResourceNames[] = {
        "sysmsh:box.nvx"
        , "sysmsh:sphere.nvx"
        , "sysmsh:cylinder.nvx"
        , "sysmsh:torus.nvx"
        , "sysmsh:cone.nvx"
    };

    const RenderShape::Type types[] = {
        RenderShape::Box
        , RenderShape::Sphere
        , RenderShape::Cylinder
        , RenderShape::Torus
        , RenderShape::Cone
    };

    for (IndexT i = 0; i < RenderShape::Primitives - 1; i++)
    {
        this->shapeMeshResources[types[i]] = CreateResource(meshResourceNames[i], "render_system", nullptr, nullptr, true);
        this->meshes[types[i]] = MeshResourceGetMesh(this->shapeMeshResources[i], 0);
    }

    // lookup ModelViewProjection shader variable
    this->model = ShaderGetConstantBinding(shapesShader, "ShapeModel");
    this->diffuseColor = ShaderGetConstantBinding(shapesShader, "ShapeColor");
    this->lineWidth = ShaderGetConstantBinding(shapesShader, "LineWidth");

    this->programs[ShaderTypes::Primitives] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Primitives"));
    this->programs[ShaderTypes::PrimitivesNoDepth] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Primitives|NoDepth"));
    this->programs[ShaderTypes::PrimitivesWireframeTriangles] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Primitives|Wireframe|Triangles"));
    this->programs[ShaderTypes::PrimitivesWireframeLines] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Primitives|Wireframe|Lines"));
    this->programs[ShaderTypes::Mesh] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Mesh"));
    this->programs[ShaderTypes::MeshNoDepth] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Mesh|NoDepth"));
    this->programs[ShaderTypes::MeshWireframe] = ShaderGetProgram(shapesShader, ShaderServer::Instance()->FeatureStringToMask("Mesh|Wireframe|Triangles"));

    this->comps.Append(VertexComponent(VertexComponent::Position, VertexComponent::Float3));
    this->comps.Append(VertexComponent(VertexComponent::Color, VertexComponent::UByte4N));

    // also create an extra vertex layout, in case we get a mesh which doesn't fit with our special layout
    this->vertexLayout = CreateVertexLayout(VertexLayoutCreateInfo{ comps });

    Frame::FrameCode* op = this->frameOpAllocator.Alloc<Frame::FrameCode>();
    op->domain = CoreGraphics::BarrierDomain::Pass;
    op->func = [](const CoreGraphics::CmdBufferId cmdBuf, const IndexT frame, const IndexT bufferIndex)
    {
        auto thisPtr = static_cast<Vulkan::VkShapeRenderer*>(VkShapeRenderer::Instance());
        thisPtr->DrawShapes(cmdBuf);
        thisPtr->numIndicesThisFrame = 0;
        thisPtr->numVerticesThisFrame = 0;
    };
    Frame::AddSubgraph("Debug Shapes", { op });
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::Close()
{
    n_assert(this->IsOpen());

    // unload shape meshes
    DiscardResource(this->shapeMeshResources[RenderShape::Box]);
    DiscardResource(this->shapeMeshResources[RenderShape::Sphere]);
    DiscardResource(this->shapeMeshResources[RenderShape::Cylinder]);
    DiscardResource(this->shapeMeshResources[RenderShape::Torus]);
    DiscardResource(this->shapeMeshResources[RenderShape::Cone]);
    //DiscardResource(this->shapeMeshResources[RenderShape::Arrow]);
    this->shapeMeshResources.Clear();

    // unload dynamic buffers
    // we don't assert here as the buffer is lazy allocated
    CoreGraphics::BufferId activeVBId = this->vbos[this->vertexBufferActiveIndex];
    CoreGraphics::BufferId activeIBId = this->ibos[this->indexBufferActiveIndex];
    if (activeVBId != CoreGraphics::InvalidBufferId)
    {
        BufferUnmap(activeVBId);
    }
    if (activeIBId != CoreGraphics::InvalidBufferId)
    {
        BufferUnmap(activeIBId);
    }

    for (IndexT i = 0; i < MaxVertexIndexBuffers; i++)
    {
        if (this->vbos[i] != InvalidBufferId)
            DestroyBuffer(this->vbos[i]);
        if (this->ibos[i] != InvalidBufferId)
        DestroyBuffer(this->ibos[i]);
    }
    
    this->vertexBufferPtr = this->indexBufferPtr = nullptr;
    this->frameOpAllocator.Release();

    // call parent class
    ShapeRendererBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawShapes(const CoreGraphics::CmdBufferId cmdBuf)
{
    n_assert(this->IsOpen());

    if (this->numIndicesThisFrame > this->indexBufferCapacity)
        this->GrowIndexBuffer();
    if (this->numVerticesThisFrame > this->vertexBufferCapacity)
        this->GrowVertexBuffer();

    for (int shaderType = 0; shaderType < ShaderTypes::NumShaders; shaderType++)
    {
        if (this->primitives[shaderType].Size() > 0)
        {
            IndexT i;
            for (i = 0; i < this->primitives[shaderType].Size(); i++)
            {
                const RenderShape& curShape = this->primitives[shaderType][i];
                if (curShape.GetShapeType() == RenderShape::Primitives)
                    this->DrawPrimitives(
                        curShape.GetModelTransform(),
                        curShape.GetTopology(),
                        curShape.GetNumVertices(),
                        curShape.GetVertexData(),
                        curShape.GetLineThickness());
                else if (curShape.GetShapeType() == RenderShape::IndexedPrimitives)
                    this->DrawIndexedPrimitives(
                        curShape.GetModelTransform(),
                        curShape.GetTopology(),
                        curShape.GetNumVertices(),
                        curShape.GetVertexData(),
                        curShape.GetNumIndices(),
                        curShape.GetIndexData(),
                        curShape.GetIndexType(),
                        curShape.GetLineThickness());
                else n_error("Shape type %d is not a primitive!", curShape.GetShapeType());
            }

            // apply shader
            CoreGraphics::CmdSetShaderProgram(cmdBuf, this->programs[shaderType]);
            CoreGraphics::CmdSetVertexLayout(cmdBuf, this->vertexLayout);

            // flush any buffered primitives
            this->DrawBufferedIndexedPrimitives(cmdBuf);
            this->DrawBufferedPrimitives(cmdBuf);
        }
    }

    if (this->vertexBufferOffset > 0)
        CoreGraphics::BufferFlush(this->vbos[this->vertexBufferActiveIndex]);
    if (this->indexBufferOffset > 0)
        CoreGraphics::BufferFlush(this->ibos[this->indexBufferActiveIndex]);

    // reset index and primitive counters
    this->indexBufferOffset = 0;
    this->vertexBufferOffset = 0;

    for (int shaderType = 0; shaderType < ShaderTypes::NumShaders; shaderType++)
    {
        if (this->shapes[shaderType].Size() > 0)
        {
            CoreGraphics::CmdSetShaderProgram(cmdBuf, this->programs[shaderType]);
            CoreGraphics::CmdSetVertexLayout(cmdBuf, this->vertexLayout);
            CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

            IndexT i;
            for (i = 0; i < this->shapes[shaderType].Size(); i++)
            {
                const RenderShape& curShape = this->shapes[shaderType][i];
                if (curShape.GetShapeType() == RenderShape::RenderMesh)
                    this->DrawMesh(cmdBuf, curShape.GetModelTransform(), curShape.GetMesh(), curShape.GetColor(), curShape.GetLineThickness());
                else
                    this->DrawSimpleShape(cmdBuf, curShape.GetModelTransform(), curShape.GetShapeType(), curShape.GetColor(), curShape.GetLineThickness());
            }
        }       
    }

    // clear shapes
    this->ClearShapes();
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawSimpleShape(const CoreGraphics::CmdBufferId cmdBuf, const Math::mat4& modelTransform, CoreGraphics::RenderShape::Type shapeType, const Math::vec4& color, float lineThickness)
{
    n_assert(this->shapeMeshResources[shapeType] != ResourceId::Invalid());
    n_assert(shapeType < RenderShape::NumShapeTypes);

    // resolve model-view-projection matrix and update shader
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->lineWidth, sizeof(float), &lineThickness);

    const MeshId mesh = this->meshes[shapeType];
    CoreGraphics::CmdSetIndexBuffer(cmdBuf, MeshGetIndexType(mesh), MeshGetIndexBuffer(mesh), MeshGetIndexOffset(mesh));
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, MeshGetVertexBuffer(mesh, 0), MeshGetVertexOffset(mesh, 0));
    const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(mesh);

    IndexT i;
    for (i = 0; i < groups.Size(); i++)
    {
        // set resources
        

        // draw
        CoreGraphics::CmdDraw(cmdBuf, groups[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawMesh(const CoreGraphics::CmdBufferId cmdBuf, const Math::mat4& modelTransform, const CoreGraphics::MeshId mesh, const Math::vec4& color, float lineThickness)
{
    n_assert(mesh != InvalidMeshId);

    // resolve model-view-projection matrix and update shader
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
    CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->lineWidth, sizeof(float), &lineThickness);

    const Util::Array<CoreGraphics::PrimitiveGroup>& groups = MeshGetPrimitiveGroups(mesh);

    // set resources
    CoreGraphics::CmdSetIndexBuffer(cmdBuf, MeshGetIndexType(mesh), MeshGetIndexBuffer(mesh), MeshGetIndexOffset(mesh));
    CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, MeshGetVertexBuffer(mesh, 0), MeshGetVertexOffset(mesh, 0));

    IndexT i;
    for (i = 0; i < groups.Size(); i++)
    {

        // draw
        CoreGraphics::CmdDraw(cmdBuf, groups[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawPrimitives(
    const Math::mat4& modelTransform
    , CoreGraphics::PrimitiveTopology::Code topology
    , SizeT numVertices
    , const void* vertices
    , float lineThickness)
{
    n_assert(0 != vertices);
    static const SizeT vertexWidth = sizeof(RenderShape::RenderShapeVertex);

    // calculate vertex count
    byte* verts = this->vertexBufferPtr;

    // unlock buffer to avoid stomping data
    memcpy(verts + this->vertexBufferOffset, vertices, numVertices * vertexWidth);

    // set vertex offset in primitive group
    CoreGraphics::PrimitiveGroup group;
    group.SetBaseVertex(this->vertexBufferOffset);
    group.SetNumVertices(numVertices);
    group.SetNumIndices(0);
    this->unindexed[topology].firstVertexOffset.Append(this->vertexBufferOffset);
    this->unindexed[topology].primitives.Append(group);
    this->unindexed[topology].transforms.Append(modelTransform);
    this->unindexed[topology].lineThicknesses.Append(lineThickness);
    this->vertexBufferOffset += numVertices * vertexWidth;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawIndexedPrimitives(
    const Math::mat4& modelTransform
    , CoreGraphics::PrimitiveTopology::Code topology
    , SizeT numVertices
    , const void* vertices
    , SizeT numIndices
    , const void* indices
    , CoreGraphics::IndexType::Code indexType
    , float lineThickness)
{
    n_assert(0 != vertices);
    n_assert(0 != indices);
    static const SizeT vertexWidth = sizeof(RenderShape::RenderShapeVertex);

    // calculate index count and size of index type
    SizeT indexSize = CoreGraphics::IndexType::SizeOf(indexType);

    byte* verts = this->vertexBufferPtr;
    byte* inds = this->indexBufferPtr;

    // unlock buffer and copy data
    memcpy(verts + this->vertexBufferOffset, vertices, numVertices * vertexWidth);
    memcpy(inds + this->indexBufferOffset, indices, numIndices * indexSize);

    // set vertex offset in primitive group
    CoreGraphics::PrimitiveGroup group;
    group.SetBaseVertex(0);
    group.SetBaseIndex(0);
    group.SetNumIndices(numIndices);
    this->indexed[topology].firstIndexOffset.Append(this->indexBufferOffset);
    this->indexed[topology].firstVertexOffset.Append(this->vertexBufferOffset);
    this->indexed[topology].primitives.Append(group);
    this->indexed[topology].transforms.Append(modelTransform);
    this->indexed[topology].indexType.Append(indexType);
    this->indexed[topology].lineThicknesses.Append(lineThickness);
    this->vertexBufferOffset += numVertices * vertexWidth;
    this->indexBufferOffset += numIndices * indexSize;
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedPrimitives(const CoreGraphics::CmdBufferId cmdBuf)
{
    IndexT j;
    for (j = 1; j < CoreGraphics::PrimitiveTopology::NumTopologies; j++)
    {
        CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, CoreGraphics::PrimitiveTopology::Code(j));
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

        IndexT i;
        for (i = 0; i < this->unindexed[j].primitives.Size(); i++)
        {
            const CoreGraphics::PrimitiveGroup& group = this->unindexed[j].primitives[i];
            const Math::mat4& modelTransform = this->unindexed[j].transforms[i];
            const Math::vec4 color(1);
            const float lineThickness = this->unindexed[j].lineThicknesses[i];
            const uint64& vertexOffset = this->unindexed[j].firstVertexOffset[i];

            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->lineWidth, sizeof(float), &lineThickness);

            CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, this->vbos[this->vertexBufferActiveIndex], vertexOffset);
            CoreGraphics::CmdDraw(cmdBuf, group);
        }
        this->unindexed[j].primitives.Clear();
        this->unindexed[j].transforms.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkShapeRenderer::DrawBufferedIndexedPrimitives(const CoreGraphics::CmdBufferId cmdBuf)
{
    IndexT j;
    for (j = 1; j < CoreGraphics::PrimitiveTopology::NumTopologies; j++)
    {
        auto topology = CoreGraphics::PrimitiveTopology::Code(j);
        CoreGraphics::CmdSetPrimitiveTopology(cmdBuf, topology);
        CoreGraphics::CmdSetGraphicsPipeline(cmdBuf);

        IndexT i;
        for (i = 0; i < this->indexed[j].primitives.Size(); i++)
        {
            const CoreGraphics::PrimitiveGroup& group = this->indexed[j].primitives[i];
            const Math::mat4& modelTransform = this->indexed[j].transforms[i];
            const Math::vec4 color(1);
            const float lineThickness = this->unindexed[j].lineThicknesses[i];
            const uint64& vertexOffset = this->indexed[j].firstVertexOffset[i];
            const uint64& indexOffset = this->indexed[j].firstIndexOffset[i];

            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->model, sizeof(modelTransform), (byte*)&modelTransform);
            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->diffuseColor, sizeof(color), (byte*)&color);
            CoreGraphics::CmdPushConstants(cmdBuf, CoreGraphics::GraphicsPipeline, this->lineWidth, sizeof(float), &lineThickness);

            CoreGraphics::CmdSetIndexBuffer(cmdBuf, this->indexed[j].indexType[i], this->ibos[this->indexBufferActiveIndex], indexOffset);
            CoreGraphics::CmdSetVertexBuffer(cmdBuf, 0, this->vbos[this->vertexBufferActiveIndex], vertexOffset);

            CoreGraphics::CmdDraw(cmdBuf, group);
        }
        this->indexed[j].primitives.Clear();
        this->indexed[j].transforms.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShapeRenderer::GrowIndexBuffer()
{
    BufferCreateInfo iboInfo;
    iboInfo.name = "ShapeRenderer IBO"_atm;
    iboInfo.size = this->numIndicesThisFrame;
    iboInfo.elementSize = IndexType::SizeOf(IndexType::Index32);
    iboInfo.mode = CoreGraphics::HostCached;
    iboInfo.usageFlags = CoreGraphics::IndexBuffer;
    iboInfo.data = nullptr;
    iboInfo.dataSize = 0;

    // unmap current pointer
    if (this->indexBufferPtr)
        BufferUnmap(this->ibos[this->indexBufferActiveIndex]);

    // delete the next buffer if one exists
    this->indexBufferActiveIndex = (this->indexBufferActiveIndex + 1) % MaxVertexIndexBuffers;
    if (this->ibos[this->indexBufferActiveIndex] != InvalidBufferId)
        DestroyBuffer(this->ibos[this->indexBufferActiveIndex]);

    this->indexBufferCapacity = this->numIndicesThisFrame;

    // finally allocate new buffer
    this->ibos[this->indexBufferActiveIndex] = CreateBuffer(iboInfo);
    this->indexBufferPtr = (byte*)BufferMap(this->ibos[this->indexBufferActiveIndex]);
    n_assert(0 != this->indexBufferPtr);
}

//------------------------------------------------------------------------------
/**
*/
void 
VkShapeRenderer::GrowVertexBuffer()
{
    BufferCreateInfo vboInfo;
    vboInfo.name = "ShapeRenderer VBO"_atm;
    vboInfo.size = this->numVerticesThisFrame;
    vboInfo.elementSize = VertexLayoutGetSize(this->vertexLayout);
    vboInfo.mode = CoreGraphics::HostCached;
    vboInfo.usageFlags = CoreGraphics::VertexBuffer;
    vboInfo.data = nullptr;
    vboInfo.dataSize = 0;

    // unmap current pointer
    if (this->vertexBufferPtr)
        BufferUnmap(this->vbos[this->vertexBufferActiveIndex]);

    // delete the next buffer if one exists
    this->vertexBufferActiveIndex = (this->vertexBufferActiveIndex + 1) % MaxVertexIndexBuffers;
    if (this->vbos[this->vertexBufferActiveIndex] != InvalidBufferId)
        DestroyBuffer(this->vbos[this->vertexBufferActiveIndex]);

    this->vertexBufferCapacity = this->numVerticesThisFrame;

    // finally allocate new buffer
    this->vbos[this->vertexBufferActiveIndex] = CreateBuffer(vboInfo);
    this->vertexBufferPtr = (byte*)BufferMap(this->vbos[this->vertexBufferActiveIndex]);
    n_assert(0 != this->vertexBufferPtr);
}

} // namespace Vulkan
