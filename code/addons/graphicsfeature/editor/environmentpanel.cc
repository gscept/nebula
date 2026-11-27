//------------------------------------------------------------------------------
//  @file environmentpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "graphicsfeature/editor/environmentpanel.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/editor/graphicseditor.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "editorassets/assetfield.h"
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "materials/material.h"
#include "lighting/lightcontext.h"
#include "fog/volumetricfogcontext.h"
#include "graphics/environmentcontext.h"
#include "posteffects/bloomcontext.h"
#include "terrain/terraincontext.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::EnvironmentPanel, 'ENVP', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
    The panel starts closed, the Window menu opens it
*/
EnvironmentPanel::EnvironmentPanel() :
    azimuth(0.0f),
    zenith(70.0f),
    bloomDirtResource(Resources::InvalidResourceId)
{
    this->name = "environment";
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
EnvironmentPanel::~EnvironmentPanel()
{
    if (this->bloomDirtResource != Resources::InvalidResourceId)
    {
        PostEffects::BloomContext::SetDirtMask(CoreGraphics::White2D);
        Resources::DiscardResource(this->bloomDirtResource);
    }
}

//------------------------------------------------------------------------------
/**
    The light only knows where it is, so the direction is read back from its position once, the way
    the old environment window did, and kept as the two angles from then on
*/
bool
EnvironmentPanel::OnSetup()
{
    const Graphics::GraphicsEntityId sun = GraphicsFeatureUnit::Instance()->globalLight;
    if (Lighting::LightContext::IsEntityRegistered(sun))
    {
        const Math::point direction = Lighting::LightContext::GetPosition(sun);
        const float zenithRad = Math::acos(Math::clamp(direction.y, -1.0f, 1.0f));
        this->zenith = Math::rad2deg(zenithRad);
        this->azimuth = Math::nearequal(zenithRad, 0.0f, TINY) ? 0.0f : Math::rad2deg(atan2f(direction.z, direction.x));
        if (this->azimuth < 0.0f)
            this->azimuth += 360.0f;
    }

    this->core->GetCommands().Register("environment", "environment [sun <azimuth> <zenith>|intensity <value>|fog <turbidity>|bloom <intensity>|terrain on|off] - print or change the sun, the fog, the bloom and the terrain",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<EnvironmentPanel, &EnvironmentPanel::CommandEnvironment>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentPanel::ApplySunDirection()
{
    const float az = Math::deg2rad(this->azimuth);
    const float ze = Math::deg2rad(this->zenith);
    Lighting::LightContext::SetTransform(GraphicsFeatureUnit::Instance()->globalLight, az, ze);
    const Graphics::GraphicsEntityId viewportLight = GraphicsViewportPanel::GetViewportLight();
    if (viewportLight != Graphics::InvalidGraphicsEntityId && Lighting::LightContext::IsEntityRegistered(viewportLight))
        Lighting::LightContext::SetTransform(viewportLight, az, ze);
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentPanel::SetBloomDirt(const IO::Path& texture)
{
    if (this->bloomDirtResource != Resources::InvalidResourceId)
    {
        PostEffects::BloomContext::SetDirtMask(CoreGraphics::White2D);
        Resources::DiscardResource(this->bloomDirtResource);
        this->bloomDirtResource = Resources::InvalidResourceId;
    }
    this->bloomDirt = texture;
    if (!texture.IsFile())
        return;
    this->bloomDirtResource = Resources::CreateResource(texture, "editor", nullptr, nullptr, { .immediate = true });
    if (this->bloomDirtResource != Resources::InvalidResourceId)
        PostEffects::BloomContext::SetDirtMask(CoreGraphics::TextureId(this->bloomDirtResource));
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentPanel::Draw()
{
    const Graphics::GraphicsEntityId sun = GraphicsFeatureUnit::Instance()->globalLight;
    const Graphics::GraphicsEntityId viewportLight = GraphicsViewportPanel::GetViewportLight();
    const bool hasViewportLight = viewportLight != Graphics::InvalidGraphicsEntityId && Lighting::LightContext::IsEntityRegistered(viewportLight);

    if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool moved = ImGui::SliderFloat("Azimuth", &this->azimuth, 0.0f, 360.0f, "%.1f deg");
        moved |= ImGui::SliderFloat("Zenith", &this->zenith, 0.0f, 90.0f, "%.1f deg");
        ImGui::SetItemTooltip("0 is straight down, 90 is at the horizon");
        if (moved)
            this->ApplySunDirection();

        Math::vec3 color = Lighting::LightContext::GetColor(sun);
        if (ImGui::ColorEdit3("Color", &color.x))
        {
            Lighting::LightContext::SetColor(sun, color);
            if (hasViewportLight)
                Lighting::LightContext::SetColor(viewportLight, color);
        }
        float intensity = Lighting::LightContext::GetIntensity(sun);
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 60000.0f, "%.0f", ImGuiSliderFlags_Logarithmic))
            Lighting::LightContext::SetIntensity(sun, intensity);
        ImGui::SetItemTooltip("The game's sun. The Scene view keeps its own brightness, it isn't exposed like the game");
    }

    if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Math::vec3 absorption = Fog::VolumetricFogContext::GetGlobalFogAbsorption();
        if (ImGui::ColorEdit3("Absorption", &absorption.x))
            Fog::VolumetricFogContext::SetGlobalFogAbsorption(absorption);
        float turbidity = Fog::VolumetricFogContext::GetGlobalFogTurbidity();
        if (ImGui::DragFloat("Turbidity", &turbidity, 1.0f, 0.0f, 200.0f))
            Fog::VolumetricFogContext::SetGlobalFogTurbidity(turbidity);
    }

    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float intensity = Graphics::EnvironmentContext::GetBloomIntensity();
        if (ImGui::SliderFloat("Intensity##bloom", &intensity, 0.0f, 1.0f))
            Graphics::EnvironmentContext::SetBloomIntensity(intensity);
        Math::vec4 color = Graphics::EnvironmentContext::GetBloomColor();
        if (ImGui::ColorEdit3("Color##bloom", &color.x))
            Graphics::EnvironmentContext::SetBloomColor(color);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Dirt");
        ImGui::SameLine();
        Editor::AssetField("bloomdirt", this->bloomDirt, "tex", [this](const IO::Path& texture) { this->SetBloomDirt(texture); }, 220.0f);
        if (this->bloomDirt.IsFile())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear##bloomdirt"))
                this->SetBloomDirt(IO::Path());
        }
    }

    if (ImGui::CollapsingHeader("Terrain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool visible = Terrain::TerrainContext::GetVisible();
        if (ImGui::Checkbox("Draw the terrain", &visible))
            Terrain::TerrainContext::SetVisible(visible);
    }

    ImGui::Separator();
    ImGui::TextDisabled("Fog, bloom and shadows show in the viewport's Rendering view.");
    ImGui::TextDisabled("The changes are the running renderer's, they aren't saved with the scene.");
}

//------------------------------------------------------------------------------
/**
*/
void
EnvironmentPanel::CommandEnvironment(const Util::Array<Util::String>& args)
{
    const Graphics::GraphicsEntityId sun = GraphicsFeatureUnit::Instance()->globalLight;
    if (args.Size() == 3 && args[0] == "sun")
    {
        this->azimuth = Math::clamp(args[1].AsFloat(), 0.0f, 360.0f);
        this->zenith = Math::clamp(args[2].AsFloat(), 0.0f, 90.0f);
        this->ApplySunDirection();
    }
    else if (args.Size() == 2 && args[0] == "intensity")
        Lighting::LightContext::SetIntensity(sun, args[1].AsFloat());
    else if (args.Size() == 2 && args[0] == "fog")
        Fog::VolumetricFogContext::SetGlobalFogTurbidity(args[1].AsFloat());
    else if (args.Size() == 2 && args[0] == "bloom")
        Graphics::EnvironmentContext::SetBloomIntensity(args[1].AsFloat());
    else if (args.Size() == 2 && args[0] == "terrain")
        Terrain::TerrainContext::SetVisible(args[1] == "on");
    else if (!args.IsEmpty())
    {
        this->core->Log(Editor::LogMessageType::N_WARNING, "environment [sun <azimuth> <zenith>|intensity <value>|fog <turbidity>|bloom <intensity>|terrain on|off]");
        return;
    }

    const Math::point direction = Lighting::LightContext::GetPosition(sun);
    this->core->Log(Editor::LogMessageType::N_SYSTEM, Util::Format("sun azimuth %.1f zenith %.1f (direction %.2f %.2f %.2f), intensity %.0f, fog turbidity %.1f, bloom %.2f, terrain %s",
        this->azimuth, this->zenith, direction.x, direction.y, direction.z, Lighting::LightContext::GetIntensity(sun),
        Fog::VolumetricFogContext::GetGlobalFogTurbidity(), Graphics::EnvironmentContext::GetBloomIntensity(),
        Terrain::TerrainContext::GetVisible() ? "on" : "off"));
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
