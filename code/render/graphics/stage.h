#pragma once
//------------------------------------------------------------------------------
/**
	A stage contains a list of graphics entities, which can be rendered through a View.
	
	The stage is also responsible for negotiating resource loading, and visibility resolution.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "graphicsentity.h"
namespace Graphics
{
class Stage : public Core::RefCounted
{
	__DeclareClass(Stage);
public:

	/// attach entity to stage, this will cause it to render
	void AttachEntity(const GraphicsEntityId entity);
	/// detach entity from stage
	void DetachEntity(const GraphicsEntityId entity);
private:
	friend class GraphicsServer;

	/// constructor
	Stage();
	/// destructor
	virtual ~Stage();

	Util::Array<GraphicsEntityId> entities;
};
} // namespace Graphics