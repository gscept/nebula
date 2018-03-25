#pragma once
//------------------------------------------------------------------------------
/**
	Adds a light representation to the graphics entity
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{
class LightContext : public GraphicsContext
{
	__DeclareClass(LightContext);
public:
	/// constructor
	LightContext();
	/// destructor
	virtual ~LightContext();
private:

	/// allocate a new slice for this context
	ContextEntityId Alloc();
	/// deallocate a slice
	void Dealloc(ContextEntityId id);
};
} // namespace Graphics