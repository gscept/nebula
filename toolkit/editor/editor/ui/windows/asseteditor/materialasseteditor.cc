//------------------------------------------------------------------------------
//  @file materialasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "materialasseteditor.h"
#include "editor/commandmanager.h"
#include "materials/material.h"
#include "materials/materialtemplates.h"
#include "coregraphics/textureloader.h"
#include "dynui/imguicontext.h"
#include "tinyfiledialogs.h"
#include "editor/tools/pathconverter.h"

#include "io/filestream.h"
#include "io/xmlwriter.h"
#include "toolkit/toolkit-common/converters/binaryxmlconverter.h"

#include "toolkitutil/surface/surfacebuilder.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
struct CMDMaterialSetTexture : public Edit::Command
{
    ~CMDMaterialSetTexture() {};
    const char* Name() override
    {
        return "Material Set Texture";
    };

    bool Execute() override
    {
        n_assert(this->item->assetType == AssetEditor::AssetType::Material);
        assetEditor->Edit();
        item->grabFocus = true;
        item->editCounter++;
        this->res = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, true, false);
        ImageHolder* textureInfo = (ImageHolder*)item->images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(item->constants + this->bindlessOffset, &handle, sizeof(handle));
            Materials::MaterialSetConstant(this->item->asset.material, &handle, sizeof(handle), this->bindlessOffset);
            Materials::MaterialInvalidate(this->item->asset.material);
        }
        else
        {
            Materials::MaterialSetTexture(this->item->asset.material, this->hash, this->res);
        }
        return true;
    };

    bool Unexecute() override
    {
        Resources::DiscardResource(this->res);
        assetEditor->Unedit();
        item->grabFocus = true;
        item->editCounter--;
        this->res = Resources::CreateResource(this->oldAsset, "editor", nullptr, nullptr, true, false);
        ImageHolder* textureInfo = (ImageHolder*)item->images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(item->constants + this->bindlessOffset, &handle, sizeof(handle));
            Materials::MaterialSetConstant(this->item->asset.material, &handle, sizeof(handle), this->bindlessOffset);
            Materials::MaterialInvalidate(this->item->asset.material);
        }
        else
        {
            Materials::MaterialSetTexture(this->item->asset.material, this->hash, this->res);
        }
        return true;
    };
    AssetEditorItem* item;
    AssetEditor* assetEditor;
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
        n_assert(this->item->assetType == AssetEditor::AssetType::Material);
        assetEditor->Edit();
        item->editCounter++;
        item->grabFocus = true;
        memcpy(item->constants, this->data, this->dataSize);
        Materials::MaterialSetConstants(this->item->asset.material, this->data, this->dataSize);
        Materials::MaterialInvalidate(this->item->asset.material);
        return true;
    };

    bool Unexecute() override
    {
        assetEditor->Unedit();
        item->editCounter--;
        item->grabFocus = true;
        memcpy(item->constants, this->oldData, this->dataSize);
        Materials::MaterialSetConstants(this->item->asset.material, this->oldData, this->dataSize);
        Materials::MaterialInvalidate(this->item->asset.material);
        return true;
    };

    ubyte* data;
    ubyte* oldData;
    uint dataSize;
    AssetEditorItem* item;
    AssetEditor* assetEditor;
};

//------------------------------------------------------------------------------
/**
*/
void
MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item, BaseWindow::SaveMode save)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
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

        ImageHolder* textureInfo = (ImageHolder*)item->images[i];
        Util::String name = Editor::PathConverter::MapToCompactPath(texLoader->GetName(textureInfo->res).Value());
        if (!name.IsEmpty())
        {
            ImGui::Text(kvp.Key());
            bool pressed = false;
            pressed |= ImGui::ImageButton(Util::Format("%s###IMAGE%s", name.AsCharPtr(), name.AsCharPtr()).AsCharPtr(), &textureInfo->texture, ImVec2{ 32, 32 });
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            {
                if (ImGui::BeginTooltip())
                {
                    ImGui::Image(&textureInfo->texture, ImVec2{ 256, 256 });
                    ImGui::EndTooltip();
                }
            }
            ImGui::SameLine();
            pressed |= ImGui::Button(Util::Format("%s###BUTTON%s", name.AsCharPtr(), name.AsCharPtr()).AsCharPtr());
            if (pressed)
            {
                // TODO: Replace file dialog with asset browser view/instance
                const char* patterns[] = { "*.dds" };
                const char* path = tinyfd_openFileDialog(kvp.Key(), IO::URI(name).LocalPath().AsCharPtr(), 1, patterns, "Texture files (DDS)", false);

                if (path != nullptr)
                {
                    auto cmd = new CMDMaterialSetTexture;
                    cmd->bindlessOffset = value->bindlessOffset;
                    cmd->index = i;
                    cmd->hash = value->hashedName;
                    cmd->assetEditor = assetEditor;
                    cmd->asset = path;
                    cmd->item = item;
                    cmd->oldAsset = name;
                    Edit::CommandManager::Execute(cmd);
                }
            }
        }
    }

    uint textureCounter = 0;
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
        const MaterialTemplates::MaterialTemplateValue* value = kvp.Value();
        ubyte* imguiState = (ubyte*)StackAlloc(materialTemplate->bufferSize);
        ubyte* currentState = Materials::MaterialGetConstants(item->asset.material);
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
            cmd->assetEditor = assetEditor;
            cmd->item = item;
            memcpy(cmd->data, imguiState, cmd->dataSize);
            memcpy(cmd->oldData, item->constants, cmd->dataSize);
            Edit::CommandManager::Execute(cmd);
        }

        // Update live state
        if (ImGui::IsItemEdited())
        {
            Materials::MaterialSetConstants(item->asset.material, imguiState, materialTemplate->bufferSize);
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
MaterialSetup(AssetEditorItem* item)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);

    // Copy over material constants
    ubyte* mem = (ubyte*)item->allocator.Alloc(materialTemplate->bufferSize);
    ubyte* data = Materials::MaterialGetConstants(item->asset.material);
    memcpy(mem, data, materialTemplate->bufferSize);
    item->constants = mem;

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        auto kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
        Resources::ResourceId res = Materials::MaterialGetTexture(item->asset.material, kvp.Value()->textureIndex);
        if (res.resourceId == Resources::InvalidResourceId.resourceId)
            res = Resources::CreateResource(kvp.Value()->resource, "editor");
        ImageHolder* data = item->allocator.Alloc<ImageHolder>();
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
        item->images.Append(data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    const MaterialTemplates::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();

    Util::String output = Editor::PathConverter::StripAssetName(item->name.AsString());

    Util::String outFile = Util::Format("assets:%s.sur", output.AsCharPtr());
    Ptr<IO::FileStream> stream = IO::FileStream::Create();
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    stream->SetURI(outFile);

    Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
    writer->SetStream(stream);
    writer->Open();
    writer->BeginNode("Nebula");
    {
        writer->BeginNode("Surface");
        {
            writer->SetString("template", materialTemplate->name);
            writer->BeginNode("Params");
            {
                for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
                {
                    auto& kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
                    const MaterialTemplates::MaterialTemplateTexture* value = kvp.Value();

                    ImageHolder* textureInfo = (ImageHolder*)item->images[i];
                    Util::String name = Editor::PathConverter::MapToCompactPath(texLoader->GetName(textureInfo->res).Value());
                    name.StripFileExtension();
                    writer->BeginNode(kvp.Key());
                        writer->SetString("value", name);
                    writer->EndNode();
                }
                for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
                {
                    auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
                    const MaterialTemplates::MaterialTemplateValue* value = kvp.Value();
                    ubyte* currentState = Materials::MaterialGetConstants(item->asset.material);

                    writer->BeginNode(kvp.Key());
                    switch (value->type)
                    {
                        case MaterialTemplates::MaterialTemplateValue::Type::Bool:
                        {
                            writer->SetString("value", Util::String::FromBool(*(bool*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplates::MaterialTemplateValue::Type::Scalar:
                        {
                            writer->SetString("value", Util::String::FromFloat(*(float*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplates::MaterialTemplateValue::Type::Vec2:
                        {
                            writer->SetString("value", Util::String::FromFloat2(*(Math::float2*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplates::MaterialTemplateValue::Type::Vec3:
                        {
                            writer->SetString("value", Util::String::FromFloat3(*(Math::float3*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplates::MaterialTemplateValue::Type::Vec4:
                        case MaterialTemplates::MaterialTemplateValue::Type::Color:
                        {
                            writer->SetString("value", Util::String::FromFloat4(*(Math::float4*)(currentState + value->offset)));
                            break;
                        }
                    }
                    writer->EndNode();
                }
            }
            writer->EndNode();
        }
        writer->EndNode();
    }
    writer->EndNode();
    writer->Close();

    // Also perform export
    ToolkitUtil::BinaryXmlConverter converter;
    Util::String expFile = Util::Format("sur:%s.sur", output.AsCharPtr());
    ToolkitUtil::Logger logger;
    converter.ConvertFile(outFile, expFile, logger);

    assetEditor->Unedit(item->editCounter);
    item->editCounter = 0;
}

} // namespace Presentation
