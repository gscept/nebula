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
};
} // namespace Graphics