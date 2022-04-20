#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::RenderShape
    
    Describes a shape which is rendered through the ShapeRenderer singleton.
    Shape rendering may be relatively inefficient and should only be used
    for debug visualizations.
    Please note that vertex and index data will be copied into a memory
    stream, so that it is safe to release or alter the original data
    once the shape object has been created. You have to be aware of
    the performance and memory-footprint implications though.
    Shape objects can be copied, but they will share the internal 
    vertex/index data copy.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/mat4.h"
#include "math/vec4.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/indextype.h"
#include "io/memorystream.h"
#include "threading/threadid.h"
#include "mesh.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class RenderShape
{
public:
    /// shape types
    enum Type
    {
        Box,
        Sphere,
        Cylinder,
        Torus,
        Cone,
        Primitives,
        IndexedPrimitives,
        RenderMesh,

        NumShapeTypes,
        InvalidShapeType,
    };

    enum RenderFlag
    {
        CheckDepth = 0,
        AlwaysOnTop,
        Wireframe,

        NumDepthFlags
    };

    struct RenderShapeVertex
    {
        Math::vec4 pos;
        Math::vec4 color;

        RenderShapeVertex() : color(Math::vec4(1)) {}
    };

    /// default constructor
    RenderShape();
    /// shortcut constructor for simple shapes
    RenderShape(Type shapeType, RenderFlag depthFlag, const Math::mat4& modelTransform, const Math::vec4& color);
    /// return true if object has been setup
    bool IsValid() const;
    /// setup simple shape
    void SetupSimpleShape(
        Type shapeType
        , RenderFlag depthFlag
        , const Math::vec4& color
        , const Math::mat4& modelTransform = Math::mat4::identity
        , float lineThickness = 1.0f);

    /// Setup primitives draw
    void SetupPrimitives(
        const RenderShape::RenderShapeVertex* vertices
        , SizeT numVertices
        , PrimitiveTopology::Code topology
        , RenderFlag depthFlag
        , const Math::mat4 transform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// Setup primitives draw
    void SetupPrimitives(
        const Util::Array<RenderShape::RenderShapeVertex> vertices
        , PrimitiveTopology::Code topology
        , RenderFlag depthFlag
        , const Math::mat4 transform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// Setup indexed primitives draw
    void SetupIndexPrimitives(
        const RenderShape::RenderShapeVertex* vertices
        , SizeT numVertices
        , const void* indices
        , SizeT numIndices
        , IndexType::Code indexType
        , PrimitiveTopology::Code topology
        , RenderFlag depthFlag
        , const Math::mat4 transform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// Setup indexed primitives draw
    void SetupIndexPrimitives(
        const Util::Array<RenderShape::RenderShapeVertex> vertices
        , const Util::Array<uint16> indices
        , PrimitiveTopology::Code topology
        , RenderFlag depthFlag
        , const Math::mat4 transform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// Setup indexed primitives draw
    void SetupIndexPrimitives(
        const Util::Array<RenderShape::RenderShapeVertex> vertices
        , const Util::Array<uint32> indices
        , PrimitiveTopology::Code topology
        , RenderFlag depthFlag
        , const Math::mat4 transform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// setup mesh
    void SetupMesh(
        const MeshId mesh
        , const IndexT groupIndex
        , const Math::vec4& color
        , RenderFlag depthFlag
        , const Math::mat4& modelTransform = Math::mat4::identity
        , float lineThickness = 1.0f
    );

    /// get shape type
    Type GetShapeType() const;
    /// get depth flag
    RenderFlag GetDepthFlag() const;
    /// get model transform
    const Math::mat4& GetModelTransform() const;
    /// get primitive topology
    PrimitiveTopology::Code GetTopology() const;
    /// get number of vertices
    SizeT GetNumVertices() const;
    /// get number of indices
    SizeT GetNumIndices() const;
    /// get pointer to vertex data (returns 0 if none exist)
    const void* GetVertexData() const;
    /// get index data (returns 0 if none exists)
    const void* GetIndexData() const;
    /// get the index type (16 or 32 bit)
    IndexType::Code GetIndexType() const;
    /// get shape color
    const Math::vec4& GetColor() const;
    /// get vertex layout, returns NULL if none exist
    const VertexLayoutId GetVertexLayout() const;
    /// get mesh
    const MeshId GetMesh() const;
    /// get primitive group
    const IndexT& GetPrimitiveGroupIndex() const;
    /// Get line thickness
    const float GetLineThickness() const;

private:
    Type shapeType;
    RenderFlag depthFlag;
    Math::mat4 modelTransform;
    PrimitiveTopology::Code topology;
    SizeT numIndices, numVertices;
    SizeT vertexDataOffset;
    IndexType::Code indexType;
    Math::vec4 color;
    float lineThickness;

    IndexT groupIndex;
    MeshId mesh;
    VertexLayoutId vertexLayout;
    Ptr<IO::MemoryStream> dataStream;       // contains vertex/index data
};

//------------------------------------------------------------------------------
/**
*/
inline bool
RenderShape::IsValid() const
{
    return (InvalidShapeType != this->shapeType);
}

//------------------------------------------------------------------------------
/**
*/
inline RenderShape::Type
RenderShape::GetShapeType() const
{
    return this->shapeType;
}

//------------------------------------------------------------------------------
/**
*/
inline RenderShape::RenderFlag
RenderShape::GetDepthFlag() const
{
    return this->depthFlag;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::mat4&
RenderShape::GetModelTransform() const
{
    return this->modelTransform;
}

//------------------------------------------------------------------------------
/**
*/
inline PrimitiveTopology::Code
RenderShape::GetTopology() const
{
    n_assert((Primitives == this->shapeType) || (IndexedPrimitives == this->shapeType));
    return this->topology;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
RenderShape::GetNumVertices() const
{
    n_assert((Primitives == this->shapeType) || (IndexedPrimitives == this->shapeType));
    return this->numVertices;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
RenderShape::GetNumIndices() const
{
    n_assert(IndexedPrimitives == this->shapeType);
    return this->numIndices;
}

//------------------------------------------------------------------------------
/**
*/
inline const void*
RenderShape::GetVertexData() const
{
    n_assert((Primitives == this->shapeType) || (IndexedPrimitives == this->shapeType));
    const void* ptr = ((uchar*)this->dataStream->GetRawPointer()) + this->vertexDataOffset;
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline const void*
RenderShape::GetIndexData() const
{
    n_assert(IndexedPrimitives == this->shapeType);
    const void* ptr = this->dataStream->GetRawPointer();
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexType::Code
RenderShape::GetIndexType() const
{
    n_assert(IndexedPrimitives == this->shapeType);
    return this->indexType;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vec4&
RenderShape::GetColor() const
{
    return this->color;
}

//------------------------------------------------------------------------------
/**
*/
inline const VertexLayoutId
RenderShape::GetVertexLayout() const
{
    return this->vertexLayout;
}

//------------------------------------------------------------------------------
/**
*/
inline const MeshId
RenderShape::GetMesh() const
{
    return this->mesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT&
RenderShape::GetPrimitiveGroupIndex() const
{
    return this->groupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline const float 
RenderShape::GetLineThickness() const
{
    return this->lineThickness;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------

