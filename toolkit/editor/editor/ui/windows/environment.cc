//------------------------------------------------------------------------------
//  environment.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "environment.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editor/cmds.h"
#include "imgui_internal.h"
#include "lighting/lightcontext.h"

using namespace Editor;
using namespace Lighting;

namespace Presentation
{
__ImplementClass(Presentation::Environment, 'EnWn', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Environment::Environment()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Environment::~Environment()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Environment::Update()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Environment::Run(SaveMode save)
{
    Graphics::GraphicsEntityId globalLight = GraphicsFeature::GraphicsFeatureUnit::Instance()->globalLight;

    //const Math::vec3& color, const float intensity, const Math::vec3& ambient, const Math::vec3& backlight, const float backlightFactor, const float zenith, const float azimuth, bool castShadows = false);
    Math::vec3 color = LightContext::GetColor(globalLight);
    float intensity = LightContext::GetIntensity(globalLight);
    if (ImGui::InputFloat3("Global light Color", &color.x))
    {
        LightContext::SetColor(globalLight, color);
    }
    if (ImGui::InputFloat("Intensity", &intensity))
    {
        LightContext::SetIntensity(globalLight, intensity);
    }

    Math::vec3 ambient = LightContext::GetAmbient(globalLight);
    if (ImGui::InputFloat3("Ambient", &ambient.x))
    {
        LightContext::SetAmbient(globalLight, ambient);
    }
}

} // namespace Presentation
