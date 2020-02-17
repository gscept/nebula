#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::GraphicsFeatureUnit

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "graphicsfeature/components/graphicscomponent.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "input/inputserver.h"

//------------------------------------------------------------------------------
namespace GraphicsFeature
{

class GraphicsFeatureUnit : public Game::FeatureUnit
{
	__DeclareClass(GraphicsFeatureUnit)
	__DeclareSingleton(GraphicsFeatureUnit)

public:

    /// constructor
    GraphicsFeatureUnit();
    /// destructor
    ~GraphicsFeatureUnit();

	/// Called upon activation of feature unit
	void OnActivate();
	/// Called upon deactivation of feature unit
	void OnDeactivate();

    /// called on begin of frame
    void OnBeginFrame();
    /// called in the middle of the feature trigger cycle
    void OnFrame();
    /// called at the end of the feature trigger cycle
    void OnEndFrame();

    /// called when game debug visualization is on
    void OnRenderDebug();



    Ptr<Graphics::View> defaultView;
    Ptr<Graphics::Stage> defaultStage;

    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Input::InputServer> inputServer;
    CoreGraphics::WindowId wnd;
    //FIXME
    Graphics::GraphicsEntityId globalLight;

};

} // namespace GraphicsFeature
//------------------------------------------------------------------------------
