#pragma once
//------------------------------------------------------------------------------
/**
    Describes a mesh as a separable entity
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/mesh.h"
#include "math/matrix44.h"

namespace Toolkit
{
class ObjectContext : public Core::RefCounted
{
    __DeclareClass(ObjectContext);
public:
    /// constructor
    ObjectContext();
    /// destructor
    virtual ~ObjectContext();

    /// set transform, cannot be changed after setup
    void SetTransform(const Math::matrix44& trans);
    /// set mesh data, cannot be changed after setup
    void SetMesh(
        Util::Array<CoreGraphics::PrimitiveGroup> groups,
        Util::Array<CoreGraphics::VertexComponent> components,
        float* vertices,
        SizeT numVertices,
        SizeT vertexSize,
        ushort* indices,
        SizeT numIndices
        );

    /// setup
    void Setup();

private:
    struct RawMesh
    {
        Util::Array<CoreGraphics::PrimitiveGroup> groups;
        Util::Array<CoreGraphics::VertexComponent> components;
        float* vertices;
        SizeT numVertices;
        SizeT vertexSize;
        ushort* indices;
        SizeT numIndices;
    } mesh;

    Math::bbox globalBox;
    Math::matrix44 transform;
};


//------------------------------------------------------------------------------
/**
*/
inline void
ObjectContext::SetTransform(const Math::matrix44& trans)
{
    this->transform = trans;
}

//------------------------------------------------------------------------------
/**
*/
void
ObjectContext::SetMesh(Util::Array<CoreGraphics::PrimitiveGroup> groups, Util::Array<CoreGraphics::VertexComponent> components, float* vertices, SizeT numVertices, SizeT vertexSize, ushort* indices, SizeT numIndices)
{
    this->mesh.groups = groups;
    this->mesh.components = components;
    this->mesh.vertices = vertices;
    this->mesh.numVertices = numVertices;
    this->mesh.vertexSize = vertexSize;
    this->mesh.indices = indices;
    this->mesh.numIndices = numIndices;
}

} // namespace Toolkit