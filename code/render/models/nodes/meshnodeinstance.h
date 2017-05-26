#pragma once
//------------------------------------------------------------------------------
/**
	Implements a renderable instance of a mesh node.
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "statenodeinstance.h"
namespace Models
{
class MeshNodeInstance : public StateNodeInstance
{
	__DeclareClass(MeshNodeInstance);
public:
	/// constructor
	MeshNodeInstance();
	/// destructor
	virtual ~MeshNodeInstance();

	/// called during visibility resolve
	virtual void OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, float distToViewer);
	/// perform rendering
	virtual void Render();
	/// perform instanced rendering
	virtual void RenderInstanced(SizeT numInstances);
private:
};
} // namespace Models