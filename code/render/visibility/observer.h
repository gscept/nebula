#pragma once
//------------------------------------------------------------------------------
/**
	An observer is some entity which is used to resolve visible objects in the scene.

	The most obvious type of Observer is a camera, which resolves what is to be rendered on the screen.
	A shadow casting light is also a type of observer.

	Whenever the scene has to be rendered from a view, the observer will contain a list of all entities
	visible from this view.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "graphics/graphicsentity.h"
namespace Visibility
{
class Observer : public Core::RefCounted
{
	__DeclareClass(Observer);
public:
	/// constructor
	Observer();
	/// destructor
	virtual ~Observer();
private:

	Ptr<Graphics::GraphicsEntity> entity;
};
} // namespace Visibility