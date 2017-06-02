#pragma once
//------------------------------------------------------------------------------
/**
	A stage contains a list of graphics entities, which can be rendered through a View.
	
	The stage is also responsible for negotiating resource loading, and visibility resolution.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Graphics
{
class Stage : public Core::RefCounted
{
	__DeclareClass(Stage);
public:
	/// constructor
	Stage();
	/// destructor
	virtual ~Stage();

	/// attach entity to stage, this will cause it to render
	void AttachEntity(const Ptr<GraphicsEntity>& entity);
	/// detach entity from stage
	void DetachEntity(const Ptr<GraphicsEntity>& entity);
private:

	Util::Array<Ptr<GraphicsEntity>> entities;
};
} // namespace Graphics