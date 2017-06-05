#pragma once
//------------------------------------------------------------------------------
/**
	A VisibilityContext adds a module to a GraphicsEntity which makes it take part
	in visibility resolution. Most graphics entities should use this, but some entities,
	like the skybox, or UI elements, does not need to be checked for visibility. 

	The same goes for lights
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "graphics/graphicscontext.h"
namespace Visibility
{
class VisibilityContext : public Graphics::GraphicsContext
{
	__DeclareClass(VisibilityContext);
	__DeclareSingleton(VisibilityContext);
public:
	/// constructor
	VisibilityContext();
	/// destructor
	virtual ~VisibilityContext();

	/// register entity
	int64_t RegisterEntity(const int64_t& entity);
	/// unregister entity
	void UnregisterEntity(const int64_t& entity);
private:
};
} // namespace Visibility