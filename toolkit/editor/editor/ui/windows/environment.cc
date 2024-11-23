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
    Math::point direction = LightContext::GetPosition(globalLight);
    // mildly hacky
    float zenithrad = Math::acos(direction.y);
    float zenith = Math::rad2deg(zenithrad);
    float azimuth = Math::nearequal(zenithrad, 0.0f, TINY) ? 0.0f : Math::rad2deg(Math::asin(direction.z / Math::sin(zenithrad)));
    bool changed = ImGui::SliderFloat("Azimut", &azimuth, 0.0f, 360.0f);
    changed |= ImGui::SliderFloat("Zenith", &zenith, 0.0f, 90.0f);
    if (changed)
    {
        LightContext::SetTransform(globalLight, Math::deg2rad(azimuth ), Math::deg2rad(zenith));
    }
    if (ImGui::ColorEdit3("Global light Color", &color.x))
    {
        LightContext::SetColor(globalLight, color);
    }
    if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1000.0f))
    {
        LightContext::SetIntensity(globalLight, intensity);
    }

    Math::vec3 ambient = LightContext::GetAmbient(globalLight);
    if (ImGui::ColorEdit3("Ambient", &ambient.x))
    {
        LightContext::SetAmbient(globalLight, ambient);
    }

    bool visible = Terrain::TerrainContext::GetVisible();
    if (ImGui::Checkbox("Render terrain", &visible))
    {
        Terrain::TerrainContext::SetVisible(visible);
    }
}

} // namespace Presentation
