#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::EnvironmentPanel
    The environment the scene is rendered in: the sun, the global fog, the bloom and whether the
    terrain is drawn.

    The sun is the game's directional light. Its direction and color move the light of the editor's
    Scene view along with it, so the scene is lit from the same side; its intensity is the game's,
    the editor's view is not exposed the same way. Fog, bloom and the sun's shadows are drawn by the
    game's frame script, which the viewport shows in its Rendering view.

    What the panel changes is the running renderer's. It is not saved with a scene.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "neweditor/core/editorpanel.h"
#include "io/path.h"
#include "math/vec3.h"
#include "resources/resourceid.h"

namespace GraphicsFeature
{

class EnvironmentPanel final : public Editor::EditorPanel
{
    __DeclareClass(EnvironmentPanel)
public:
    EnvironmentPanel();
    virtual ~EnvironmentPanel();

    void Draw() override;

protected:
    bool OnSetup() override;

private:
    /// put the sun where the azimuth and the zenith say, on the game's light and the viewport's
    void ApplySunDirection();
    /// load a texture for the bloom's dirt, the one before is let go
    void SetBloomDirt(const IO::Path& texture);

    /// environment [sun <azimuth> <zenith>|intensity <value>|fog <turbidity>|bloom <intensity>|terrain on|off]
    void CommandEnvironment(const Util::Array<Util::String>& args);

    /// the sun's direction in degrees, kept here because the light only knows its position
    float azimuth;
    float zenith;
    IO::Path bloomDirt;
    Resources::ResourceId bloomDirtResource;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
