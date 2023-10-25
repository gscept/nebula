#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::GraphicsFeatureUnit

    Sets up the core rendering system and provides properties and managers for
    default usage, such as rendering models, animations, particles, etc.

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "game/featureunit.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/stage.h"
#include "input/inputserver.h"
#include "managers/cameramanager.h"
#include "core/cvar.h"

#include "terrain/terraincontext.h"

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

    void OnAttach();

    /// Called upon activation of feature unit
    void OnActivate() override;
    /// Called upon deactivation of feature unit
    void OnDeactivate() override;

    /// called on begin of frame
    void OnBeginFrame() override;
    /// Called before views
    void OnBeforeViews() override;
    /// called in the middle of the feature trigger cycle
    void OnFrame() override;
    /// called at the end of the feature trigger cycle
    void OnEndFrame() override;

    /// called when game debug visualization is on
    void OnRenderDebug() override;

    /// Set window title
    void SetWindowTitle(const Util::StringAtom& title);

    /// Set Graphics Debugging on/off
    void SetGraphicsDebugging(bool value);

    /// retrieve the default view
    Ptr<Graphics::View> GetDefaultView() const;
    /// retrieve the default stage
    Ptr<Graphics::Stage> GetDefaultStage() const;

    /// retrieve the default view handle
    ViewHandle GetDefaultViewHandle() const;

    /// set framescript. must be done before OnActivate!
    void SetFrameScript(IO::URI const& uri);

    /// Setup terrain biome, run before OnActivate
    void SetupTerrainBiome(const Terrain::BiomeSettings& biomeParameters);

    using UIRenderFunc = std::function<void()>;
    /// add a custom UI render function
    void AddRenderUICallback(UIRenderFunc func);
    
    Graphics::GraphicsEntityId globalLight;

private:
    Util::StringAtom title;
    IO::URI defaultFrameScript;
    Ptr<Graphics::View> defaultView;
    Ptr<Graphics::Stage> defaultStage;

    Ptr<Graphics::GraphicsServer> gfxServer;
    Ptr<Input::InputServer> inputServer;
    CoreGraphics::WindowId wnd;
    //FIXME

    Util::Array<UIRenderFunc> uiCallbacks;

    Game::ManagerHandle graphicsManagerHandle;
    Game::ManagerHandle cameraManagerHandle;

    Util::Array<Terrain::TerrainBiomeId> biomes;
    ViewHandle defaultViewHandle;

    Core::CVar* r_debug;
    Core::CVar* r_show_frame_inspector;
};

//------------------------------------------------------------------------------
/**
*/
inline void
GraphicsFeature::GraphicsFeatureUnit::SetGraphicsDebugging(bool value)
{
    Core::CVarWriteInt(this->r_debug, 2);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
GraphicsFeatureUnit::SetWindowTitle(const Util::StringAtom& title)
{
    this->title = title;
}

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
inline ViewHandle
GraphicsFeatureUnit::GetDefaultViewHandle() const
{
    return this->defaultViewHandle;
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
