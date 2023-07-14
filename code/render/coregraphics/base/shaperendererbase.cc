//------------------------------------------------------------------------------
//  shaperendererbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/base/shaperendererbase.h"
#include "math/point.h"

namespace Base
{
__ImplementClass(Base::ShapeRendererBase, 'SRBS', Core::RefCounted);
__ImplementSingleton(Base::ShapeRendererBase);

using namespace Math;
using namespace Util;
using namespace Threading;
using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
ShapeRendererBase::ShapeRendererBase() 
    : isOpen(false)
    , numIndicesThisFrame(0)
    , numVerticesThisFrame(0)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShapeRendererBase::~ShapeRendererBase()
{
    n_assert(!this->isOpen);
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::Close()
{
    n_assert(this->isOpen);
    this->isOpen = false;
    for (IndexT i = 0; i < ShaderTypes::NumShaders; i++)
    {
        this->shapes[i].Clear();
        this->primitives[i].Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::ClearShapes()
{
    n_assert(this->IsOpen());
    IndexT t;   
    for (t = 0; t < ShaderTypes::NumShaders; t++)
    {
        this->primitives[t].Clear();
        this->shapes[t].Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::AddShape(const RenderShape& shape)
{
    n_assert(this->IsOpen());
    if (shape.GetShapeType() == RenderShape::Primitives)
    {
        switch (shape.GetDepthFlag())
        {
            case RenderShape::CheckDepth:
                this->primitives[ShaderTypes::Primitives].Append(shape);
                break;
            case RenderShape::AlwaysOnTop:
                this->primitives[ShaderTypes::PrimitivesNoDepth].Append(shape);
                break;
            case RenderShape::Wireframe:
                switch (shape.GetTopology())
                {
                    case PrimitiveTopology::LineList:
                    case PrimitiveTopology::LineStrip:
                        this->primitives[ShaderTypes::PrimitivesWireframeLines].Append(shape);
                        break;
                    case PrimitiveTopology::TriangleList:
                    case PrimitiveTopology::TriangleStrip:
                        this->primitives[ShaderTypes::PrimitivesWireframeTriangles].Append(shape);
                        break;
                }
                break;
        }

        this->numVerticesThisFrame += shape.GetNumVertices();
    }
    else if (shape.GetShapeType() == RenderShape::IndexedPrimitives)
    {
        switch (shape.GetDepthFlag())
        {
            case RenderShape::CheckDepth:
                this->primitives[ShaderTypes::Primitives].Append(shape);
                break;
            case RenderShape::AlwaysOnTop:
                this->primitives[ShaderTypes::PrimitivesNoDepth].Append(shape);
                break;
            case RenderShape::Wireframe:
                switch (shape.GetTopology())
                {
                    case PrimitiveTopology::LineList:
                    case PrimitiveTopology::LineStrip:
                        this->primitives[ShaderTypes::PrimitivesWireframeLines].Append(shape);
                        break;
                    case PrimitiveTopology::TriangleList:
                    case PrimitiveTopology::TriangleStrip:
                        this->primitives[ShaderTypes::PrimitivesWireframeTriangles].Append(shape);
                        break;
                    default:
                        n_error("Topology not supported");
                }
                break;
        }

        this->numVerticesThisFrame += shape.GetNumVertices();
        this->numIndicesThisFrame += shape.GetNumIndices();
    }
    else
    {
        switch (shape.GetDepthFlag())
        {
            case RenderShape::CheckDepth:
                this->shapes[ShaderTypes::Mesh].Append(shape);
                break;
            case RenderShape::AlwaysOnTop:
                this->shapes[ShaderTypes::MeshNoDepth].Append(shape);
                break;
            case RenderShape::Wireframe:
                this->shapes[ShaderTypes::MeshWireframe].Append(shape);
                break;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::AddShapes(const Array<RenderShape>& shapeArray)
{
    n_assert(this->IsOpen());
    for (int i = 0; i < shapeArray.Size(); i++)
    {
        const RenderShape& shape = shapeArray[i];
        if (shape.GetShapeType() == RenderShape::Primitives || shape.GetShapeType() == RenderShape::IndexedPrimitives)
        {
            this->primitives[shape.GetDepthFlag()].Append(shape);
            this->numVerticesThisFrame += shape.GetNumVertices();
            this->numIndicesThisFrame += shape.GetNumIndices();
        }
        else
            this->shapes[shape.GetDepthFlag()].Append(shape);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::DrawShapes()
{
    // override in subclass!
    n_error("ShapeRendererBase::DrawShapes() called!");
}

//------------------------------------------------------------------------------
/**
*/
void 
ShapeRendererBase::AddWireFrameBox(const Math::bbox& boundingBox, const Math::vec4& color)
{
    // render lines around bbox
    const Math::point& center = boundingBox.center();
    const Math::vector& extends = boundingBox.extents();
    const Math::vector corners[] = {
        vector(1,1,1),
        vector(1,1,-1),
        vector(1,1,-1),
        vector(-1,1,-1),
        vector(-1,1,-1),
        vector(-1,1,1),
        vector(-1,1,1),
        vector(1,1,1),

        vector(1,-1,1),
        vector(1,-1,-1),
        vector(1,-1,-1),
        vector(-1,-1,-1),
        vector(-1,-1,-1),
        vector(-1,-1,1),
        vector(-1,-1,1),
        vector(1,-1,1),

        vector(1,1,1),
        vector(1,-1,1),
        vector(1,1,-1),
        vector(1,-1,-1),
        vector(-1,1,-1),
        vector(-1,-1,-1),
        vector(-1,1,1),
        vector(-1,-1,1)};

    Util::Array<CoreGraphics::RenderShape::RenderShapeVertex> lineList;
    IndexT i;
    for (i = 0; i < 24; ++i)
    {
        CoreGraphics::RenderShape::RenderShapeVertex vert;
        vert.pos = center + extends * corners[i];
        lineList.Append(vert);      
    }       
    RenderShape shape;
    shape.SetupPrimitives(
        lineList
        , PrimitiveTopology::LineList
        , CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe));

    this->AddShape(shape);    
}


} // namespace Base
