#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::GraphicsFeatureUnit

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
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

    /// retrieve the default view
    Ptr<Graphics::View> GetDefaultView() const;
    /// retrieve the default stage
    Ptr<Graphics::Stage> GetDefaultStage() const;

    /// set framescript. must be done before OnActivate!
    void SetFrameScript(IO::URI const& uri);
    
    using UIRenderFunc = std::function<void()>;
    /// add a custom UI render function
    void AddRenderUICallback(UIRenderFunc func);

private:
    IO::URI defaultFrameScript;
    Ptr<Graphics::View> defaultView;
    Ptr<Graphics::Stage> defaultStage;

    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Input::InputServer> inputServer;
    CoreGraphics::WindowId wnd;
    //FIXME
    Graphics::GraphicsEntityId globalLight;

    Util::Array<UIRenderFunc> uiCallbacks;
};

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Graphics::View>
GraphicsFeatureUnit::GetDefaultView() const
{
    return this->defaultView;
}

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Graphics::Stage>
GraphicsFeatureUnit::GetDefaultStage() const
{
    return this->defaultStage;
}

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsFeatureUnit::SetFrameScript(IO::URI const& uri)
{
    this->defaultFrameScript = uri;
}

} // namespace GraphicsFeature
//------------------------------------------------------------------------------
