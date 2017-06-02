#pragma once
//------------------------------------------------------------------------------
/**
	The visibility server is the main hub for the visibility subsystem.

	The visibility server should be the called at the beginning of a frame, which
	fires off visibility queries for all observers.

	At some later point during the frame, each observer can obtain its list of graphics
	entities to render. 
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "observer.h"
#include "graphics/graphicsentity.h"
namespace Visibility
{

enum class ObserverMask : uint8_t
{

};

class VisibilityServer : public Core::RefCounted
{
	__DeclareClass(VisibilityServer);
	__DeclareSingleton(VisibilityServer);
public:
	/// constructor
	VisibilityServer();
	/// destructor
	virtual ~VisibilityServer();

	/// update visibility server - once per frame - walks through all observers and builds a visibility list
	void Update();

	/// register graphics entity as an observer
	void RegisterObserver(const Ptr<Graphics::GraphicsEntity>& obs, ObserverMask);
	/// unregister graphics entity 
	void UnregisterObserver(const Ptr<Graphics::GraphicsEntity>& obs);
private:

	Util::Array<Ptr<Observer>> observers;
};
} // namespace Visibility