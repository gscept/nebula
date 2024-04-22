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
#include "materials/material_interfaces.h"

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

struct CMDMaterialSetTexture : public Edit::Command
{
    ~CMDMaterialSetTexture() {};
    const char* Name() override
    {
        return "Material Set Texture";
    };

    bool Execute() override
    {
        n_assert(this->previewer->assetType == Previewer::PreviewAssetType::Material);
        this->res = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, true, false);
        ImageHolder* textureInfo = (ImageHolder*)previewerState.images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(previewerState.constants + this->bindlessOffset, &handle, sizeof(handle));
            Materials::MaterialSetConstant(this->previewer->asset.material, &handle, sizeof(handle), this->bindlessOffset);
            Materials::MaterialInvalidate(this->previewer->asset.material);
        }
        else
        {
            Materials::MaterialSetTexture(this->previewer->asset.material, this->hash, this->res);
        }
        return true;
    };

    bool Unexecute() override
    {
        Resources::DiscardResource(this->res);
        this->res = Resources::CreateResource(this->oldAsset, "editor", nullptr, nullptr, true, false);
        ImageHolder* textureInfo = (ImageHolder*)previewerState.images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(previewerState.constants + this->bindlessOffset, &handle, sizeof(handle));
            Materials::MaterialSetConstant(this->previewer->asset.material, &handle, sizeof(handle), this->bindlessOffset);
            Materials::MaterialInvalidate(this->previewer->asset.material);
        }
        else
        {
            Materials::MaterialSetTexture(this->previewer->asset.material, this->hash, this->res);
        }
        return true;
    };
    Previewer* previewer;
    uint hash;
    uint index;
    uint bindlessOffset;
    Resources::ResourceName asset;

    Resources::ResourceName oldAsset;

private:
    Resources::ResourceId res;
};

struct CMDMaterialSetConstant : public Edit::Command
{
    ~CMDMaterialSetConstant() {};

    const char* Name() override
    {
        return "Material Set Constant";
    };

    bool Execute() override
    {
        n_assert(this->previewer->assetType == Previewer::PreviewAssetType::Material);
        memcpy(previewerState.constants, this->data, this->dataSize);
        Materials::MaterialSetConstants(this->previewer->asset.material, this->data, this->dataSize);
        Materials::MaterialInvalidate(this->previewer->asset.material);
        return true;
    };

    bool Unexecute() override
    {
        memcpy(previewerState.constants, this->oldData, this->dataSize);
        Materials::MaterialSetConstants(this->previewer->asset.material, this->oldData, this->dataSize);
        Materials::MaterialInvalidate(this->previewer->asset.material);
        return true;
    };

    ubyte* data;
    ubyte* oldData;
    uint dataSize;
    Previewer* previewer;
};

//------------------------------------------------------------------------------
/**
*/
void
MaterialEditor(Previewer* previewer)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(previewer->asset.material);
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
                auto cmd = new CMDMaterialSetTexture;
                cmd->bindlessOffset = value->bindlessOffset;
                cmd->index = i;
                cmd->hash = value->hashedName;
                cmd->previewer = previewer;
                cmd->asset = path;
                cmd->oldAsset = name;
                Edit::CommandManager::Execute(cmd);
            }
        }
    }

    uint textureCounter = 0;
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
        const MaterialTemplates::MaterialTemplateValue* value = kvp.Value();
        ubyte* imguiState = (ubyte*)StackAlloc(materialTemplate->bufferSize);
        ubyte* currentState = Materials::MaterialGetConstants(previewer->asset.material);
        memcpy(imguiState, currentState, materialTemplate->bufferSize);

        switch (value->type)
        {
            case MaterialTemplates::MaterialTemplateValue::Type::Bool:
            {
                ImGui::Checkbox(kvp.Key(), (bool*)(imguiState + value->offset));
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Scalar:
            {
                ImGui::SliderFloat(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec2:
            {
                ImGui::SliderFloat2(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec3:
            {
                ImGui::SliderFloat3(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec4:
            {
                ImGui::SliderFloat4(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Color:
            {
                ImGui::ColorEdit4(kvp.Key(), (float*)(imguiState + value->offset), ImGuiColorEditFlags_Float);
                break;
            }
            default:
                // TODO: Implement all types
                break;
        }

        // Issue command
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            CMDMaterialSetConstant* cmd = new CMDMaterialSetConstant;
            cmd->data = (ubyte*)Edit::CommandManager::AllocScratch(materialTemplate->bufferSize);
            cmd->oldData = (ubyte*)Edit::CommandManager::AllocScratch(materialTemplate->bufferSize);
            cmd->dataSize = materialTemplate->bufferSize;
            cmd->previewer = previewer;
            memcpy(cmd->data, imguiState, cmd->dataSize);
            memcpy(cmd->oldData, previewerState.constants, cmd->dataSize);
            Edit::CommandManager::Execute(cmd);
        }

        // Update live state
        if (ImGui::IsItemEdited())
        {
            Materials::MaterialSetConstants(previewer->asset.material, imguiState, materialTemplate->bufferSize);
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && kvp.Value()->desc != nullptr)
        {
            ImGui::SetTooltip(kvp.Value()->desc);
        }
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
            MaterialEditor(this);
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
        if (res.resourceId == Resources::InvalidResourceId.resourceId)
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
                this->res = id;
                Setup(id.resourceId, type);
            }
        );
        return true;
    };

    bool Unexecute() override
    {
        Resources::DiscardResource(this->res);
        this->previewer->assetType = Previewer::PreviewAssetType::None;
        return true;
    };
    Previewer* previewer;
    Resources::ResourceId res;
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
