#pragma once
//------------------------------------------------------------------------------
/**
	A graphics entity is simply an ID handle and a transform, which is used with 
	its attached GraphicsContexts to actually perform specific rendering tasks.

	The entity itself stores a list of all associated graphics contexts, such that
	they can be requested later. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Graphics
{
class GraphicsContext;
class GraphicsEntity : public Core::RefCounted
{
	__DeclareClass(GraphicsEntity);
public:
	/// constructor
	GraphicsEntity();
	/// destructor
	virtual ~GraphicsEntity();


private:

	static int64_t UniqueIdCounter;
	Util::Array<Ptr<GraphicsContext>> contexts;
	int64_t id; 
	Math::matrix44 transform;
};
} // namespace Graphics