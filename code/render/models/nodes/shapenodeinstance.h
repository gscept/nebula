#pragma once
//------------------------------------------------------------------------------
/**
    @class Models::ShapeNodeInstance
  
    Offers an alternative to the ShapeNodeInstance, where an object would rely on a material instead of a shader
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/
#include "models/nodes/statenodeinstance.h"
//------------------------------------------------------------------------------
namespace Models
{
class ShapeNodeInstance : public StateNodeInstance
{
    __DeclareClass(ShapeNodeInstance);
public:
    /// constructor
    ShapeNodeInstance();
    /// destructor
    virtual ~ShapeNodeInstance();
    
    /// called during visibility resolve
	virtual void OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distToViewer);
    /// perform rendering
    virtual void Render();
    /// perform instanced rendering
    virtual void RenderInstanced(SizeT numInstances);
};

} // namespace Models
//------------------------------------------------------------------------------

