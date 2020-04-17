//------------------------------------------------------------------------------
//  shaperendererbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/base/shaperendererbase.h"
#include "threading/threadid.h"
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
    n_assert(this->shapes[RenderShape::AlwaysOnTop].IsEmpty());
	n_assert(this->shapes[RenderShape::CheckDepth].IsEmpty());
	n_assert(this->shapes[RenderShape::Wireframe].IsEmpty());
	n_assert(this->primitives[RenderShape::AlwaysOnTop].IsEmpty());
	n_assert(this->primitives[RenderShape::CheckDepth].IsEmpty());
	n_assert(this->primitives[RenderShape::Wireframe].IsEmpty());
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
	this->shapes[RenderShape::AlwaysOnTop].Clear();
    this->shapes[RenderShape::CheckDepth].Clear();
	this->shapes[RenderShape::Wireframe].Clear();
	this->primitives[RenderShape::AlwaysOnTop].Clear();
	this->primitives[RenderShape::CheckDepth].Clear();
	this->primitives[RenderShape::Wireframe].Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ShapeRendererBase::ClearShapes()
{
    n_assert(this->IsOpen());
	IndexT t;	
	for(t = 0; t<CoreGraphics::RenderShape::NumDepthFlags; t++)
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
    if (shape.GetShapeType() == RenderShape::Primitives || shape.GetShapeType() == RenderShape::IndexedPrimitives)
    {
        this->primitives[shape.GetDepthFlag()].Append(shape);
        this->numVerticesThisFrame += shape.GetNumVertices();
        this->numIndicesThisFrame += PrimitiveTopology::NumberOfVertices(shape.GetTopology(), shape.GetNumPrimitives());
    }
	else
        this->shapes[shape.GetDepthFlag()].Append(shape);
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
            this->numIndicesThisFrame += PrimitiveTopology::NumberOfVertices(shape.GetTopology(), shape.GetNumPrimitives());
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
    shape.SetupPrimitives(mat4(),
        PrimitiveTopology::LineList,
        lineList.Size() / 2,
        &(lineList.Front()),
        color,
        CoreGraphics::RenderShape::CheckDepth);
    this->AddShape(shape);    
}


} // namespace Base
