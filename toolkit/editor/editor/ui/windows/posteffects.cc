//------------------------------------------------------------------------------
//  posteffects.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "posteffects.h"
#include "graphics/environmentcontext.h"
#include "posteffects/bloomcontext.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/assetbrowser.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
PostEffectSettings::PostEffectSettings()
{
}

//------------------------------------------------------------------------------
/**
*/
PostEffectSettings::~PostEffectSettings()
{
    if (this->bloomDirt != Resources::InvalidResourceId)
        Resources::DiscardResource(this->bloomDirt);
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectSettings::Run(SaveMode save)
{
    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float intensity = Graphics::EnvironmentContext::GetBloomIntensity();
        if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
            Graphics::EnvironmentContext::SetBloomIntensity(intensity);

        Math::vec4 color = Graphics::EnvironmentContext::GetBloomColor();
        if (ImGui::ColorEdit3("Color", &color.x))
            Graphics::EnvironmentContext::SetBloomColor(color);

        if (this->bloomDirt == Resources::InvalidResourceId)
            ImGui::Text("Dirt: none");
        else
            ImGui::Text("Dirt: %s", CoreGraphics::TextureGetName(CoreGraphics::TextureId(this->bloomDirt)).Value());
        if (ImGui::Button("Set dirt"))
        {
            auto browser = (Presentation::AssetBrowser*)Presentation::AssetBrowserPickerWindow;
            browser->PickFile(IO::Path(), [this](const IO::Path& filePath)
            {
                n_assert(filePath.IsFile());
                if (filePath.GetType() != "tex")
                {
                    n_warning("Lens dirt must be a texture\n");
                    return;
                }
                Resources::CreateResource(filePath, "editor", [this](Resources::ResourceId id)
                {
                    if (this->bloomDirt != Resources::InvalidResourceId)
                        Resources::DiscardResource(this->bloomDirt);
                    this->bloomDirt = id;
                    PostEffects::BloomContext::SetDirtMask(CoreGraphics::TextureId(id));
                }, nullptr);
            });
        }
        if (this->bloomDirt != Resources::InvalidResourceId && ImGui::Button("Clear dirt"))
        {
            Resources::DiscardResource(this->bloomDirt);
            this->bloomDirt = Resources::InvalidResourceId;
            PostEffects::BloomContext::SetDirtMask(CoreGraphics::White2D);
        }
    }
}

} // namespace Presentation
