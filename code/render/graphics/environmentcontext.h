#pragma once
//------------------------------------------------------------------------------
/**
	The environment context deals with anything related to the sky and atmosphere effects

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{

class EnvironmentContext : public GraphicsContext
{
	_DeclarePluginContext();
public:

	/// create context
	static void Create(const Graphics::GraphicsEntityId sun);
	/// update shader server tick params per frame
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks);
private:


};

} // namespace Graphics
