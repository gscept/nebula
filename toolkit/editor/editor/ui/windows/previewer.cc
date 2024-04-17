//------------------------------------------------------------------------------
//  previewer.cc
//  (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "previewer.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"
#include "materials/material.h"
#include "materials/materialtemplates.h"
#include "dynui/imguicontext.h"
#include "tinyfiledialogs.h"
#include "resources/resourceserver.h"
#include "coregraphics/textureloader.h"

using namespace Editor;

struct ImageHolder
{
    Resources::ResourceId res;
    Dynui::ImguiTextureId texture;
};

struct
{
    Memory::ArenaAllocator<2048> assetAllocator;
    Util::Array<ImageHolder*> images;
    ubyte* constants;

} previewerState;
namespace Presentation
{
__ImplementClass(Presentation::Previewer, 'PrvW', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
Previewer::Previewer()
{
}

//------------------------------------------------------------------------------
/**
*/
Previewer::~Previewer()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialEditor(const Materials::MaterialId mat)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(mat);
    ImGui::PushFont(Dynui::ImguiContext::state.boldFont);
    ImGui::Text(materialTemplate->name);
    ImGui::PopFont();

    ImGui::Separator();

    bool applyChange = false;
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        auto& kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
        const MaterialTemplates::MaterialTemplateTexture* value = kvp.Value();

        ImageHolder* textureInfo = (ImageHolder*)previewerState.images[i];
        const char* name = texLoader->GetName(textureInfo->res).Value();
        if (name)
        {
            ImGui::Text(kvp.Key());
            bool pressed = false;
            pressed |= ImGui::ImageButton(name, &textureInfo->texture, ImVec2{ 32, 32 });
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (ImGui::BeginTooltip())
                {
                    ImGui::Image(&textureInfo->texture, ImVec2{ 256, 256 });
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine();
            pressed |= ImGui::Button(name);
            if (pressed)
            {
                // TODO: Replace file dialog with asset browser view/instance
                const char* patterns[] = { "*.dds" };
                const char* path = tinyfd_openFileDialog(kvp.Key(), IO::URI(name).LocalPath().AsCharPtr(), 1, patterns, "Texture files (DDS)", false);
                Resources::ResourceId newTex = Resources::CreateResource(path, "editor", [mat, textureInfo, value, bufferSize = materialTemplate->bufferSize](Resources::ResourceId res)
                {
                    textureInfo->res = res;
                    textureInfo->texture.nebulaHandle = res.resourceId;
                    if (value->bindlessOffset != 0xFFFFFFFF)
                    {
                        uint handle = CoreGraphics::TextureGetBindlessHandle(res);
                        memcpy(previewerState.constants + value->bindlessOffset, &handle, sizeof(handle));
                        Materials::MaterialSetConstants(mat, previewerState.constants, bufferSize);
                        Materials::MaterialInvalidate(mat);
                    }
                    else
                    {
                        Materials::MaterialSetTexture(mat, value->hashedName, res);
                    }
                });
            }
        }
    }

    uint textureCounter = 0;
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
        const MaterialTemplates::MaterialTemplateValue* value = kvp.Value();

        switch (value->type)
        {
            case MaterialTemplates::MaterialTemplateValue::Type::Bool:
            {
                if (ImGui::Checkbox(kvp.Key(), (bool*)(previewerState.constants + value->offset)))
                {
                    applyChange = true;
                }
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Scalar:
            {
                if (ImGui::SliderFloat(kvp.Key(), (float*)(previewerState.constants + value->offset), 0.0f, 1.0f))
                {
                    applyChange = true;
                }
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec2:
            {
                if (ImGui::SliderFloat2(kvp.Key(), (float*)(previewerState.constants + value->offset), 0.0f, 1.0f))
                {
                    applyChange = true;
                }
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec3:
            {
                if (ImGui::SliderFloat3(kvp.Key(), (float*)(previewerState.constants + value->offset), 0.0f, 1.0f))
                {
                    applyChange = true;
                }
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec4:
            {
                if (ImGui::SliderFloat4(kvp.Key(), (float*)(previewerState.constants + value->offset), 0.0f, 1.0f))
                {
                    applyChange = true;
                }
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Color:
            {
                if (ImGui::ColorEdit4(kvp.Key(), (float*)(previewerState.constants + value->offset), ImGuiColorEditFlags_Float))
                {
                    applyChange = true;
                }
                break;
            }
            default:
                // TODO: Implement all types
                break;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && kvp.Value()->desc != nullptr)
        {
            ImGui::SetTooltip(kvp.Value()->desc);
        }
    }

    if (applyChange)
    {
        Materials::MaterialSetConstants(mat, previewerState.constants, materialTemplate->bufferSize);
        Materials::MaterialInvalidate(mat);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EmptyEditor()
{
    ImGuiStyle& style = ImGui::GetStyle();

    static const char* EmptyString = "Nothing selected";
    float sizeX = ImGui::CalcTextSize(EmptyString).x + style.FramePadding.x * 2.0f;
    float availX = ImGui::GetContentRegionAvail().x;
    float sizeY = ImGui::CalcTextSize(EmptyString).y + style.FramePadding.y * 2.0f;
    float availY = ImGui::GetContentRegionAvail().y;

    float off = (availX - sizeX) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
    off = (availY - sizeY) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + off);

    ImGui::Text(EmptyString);
}

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Run()
{
    switch (this->assetType)
    {
        case PreviewAssetType::Material:
            MaterialEditor(this->asset.material);
            break;
        case PreviewAssetType::Mesh:
            break;
        case PreviewAssetType::Skeleton:
            break;
        case PreviewAssetType::Model:
            break;
        case Previewer::PreviewAssetType::None:
            EmptyEditor();
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialSetup(const Materials::MaterialId mat)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(mat);

    // Copy over material constants
    ubyte* mem = (ubyte*)previewerState.assetAllocator.Alloc(materialTemplate->bufferSize);
    ubyte* data = Materials::MaterialGetConstants(mat);
    memcpy(mem, data, materialTemplate->bufferSize);
    previewerState.constants = mem;

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        auto kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
        Resources::ResourceId res = Materials::MaterialGetTexture(mat, kvp.Value()->textureIndex);
        if (res == Resources::InvalidResourceId)
            res = Resources::CreateResource(kvp.Value()->resource, "editor");
        ImageHolder* data = previewerState.assetAllocator.Alloc<ImageHolder>();
        Resources::CreateResourceListener(res, [data](Resources::ResourceId res)
        {
            data->res = res;
            data->texture.layer = 0;
            data->texture.mip = 0;
            data->texture.nebulaHandle.resourceId = res.resourceId;
        });

        data->res = res;
        data->texture.layer = 0;
        data->texture.mip = 0;
        data->texture.nebulaHandle.resourceId = res.resourceId;
        previewerState.images.Append(data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Setup(const Materials::MaterialId mat, const Previewer::PreviewAssetType type)
{
    previewerState.assetAllocator.Release();
    previewerState.constants = nullptr;
    previewerState.images.Clear();
    switch (type)
    {
        case Previewer::PreviewAssetType::Material:
            MaterialSetup(mat);
            break;
        default:
            // TODO: implement other editors
            break;
    }
}

struct CMDPreviewAsset : public Edit::Command
{
    ~CMDPreviewAsset()
    {
    };
    const char* Name() override
    {
        return "Preview asset";
    };

    bool Execute() override
    {
        Resources::CreateResource(this->asset, "editor",
            [this](Resources::ResourceId id)
            {
                this->previewer->asset.id = id.resourceId;
                this->previewer->assetType = type;
                Setup(id.resourceId, type);
            }
        );
        return true;
    };

    bool Unexecute() override
    {
        this->previewer->assetType = Previewer::PreviewAssetType::None;
        return true;
    };
    Previewer* previewer;
    Resources::ResourceName asset;
    Previewer::PreviewAssetType type;
};

//------------------------------------------------------------------------------
/**
*/
void
Previewer::Preview(const Resources::ResourceName& asset, const PreviewAssetType type)
{
    auto cmd = new CMDPreviewAsset;
    cmd->previewer = this;
    cmd->asset = asset;
    cmd->type = type;
    Edit::CommandManager::Execute(cmd);
}

} // namespace Presentation
