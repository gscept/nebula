#pragma once
//------------------------------------------------------------------------------
/**
	Adds a light probe component to graphics entities
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{
class LightProbeContext : public GraphicsContext
{
	__DeclareClass(LightProbeContext);
public:
	/// constructor
	LightProbeContext();
	/// destructor
	virtual ~LightProbeContext();
private:

	/// allocate a new slice for this context
	ContextEntityId Alloc();
	/// deallocate a slice
	void Dealloc(ContextEntityId id);
};
} // namespace Graphics