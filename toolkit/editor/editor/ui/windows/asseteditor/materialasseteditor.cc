//------------------------------------------------------------------------------
//  @file materialasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "materialasseteditor.h"
#include "editor/commandmanager.h"
#include "materials/material.h"
#include "materials/gpulang/materialtemplatesgpulang.h"
#include "coregraphics/textureloader.h"
#include "dynui/imguicontext.h"
#include "tinyfiledialogs.h"
#include "editor/tools/pathconverter.h"

#include "io/filestream.h"
#include "io/xmlwriter.h"
#include "toolkit-common/converters/binaryxmlconverter.h"

#include "toolkitutil/surface/surfacebuilder.h"

namespace Presentation
{

struct MaterialEditorItemData
{
    ImageHolder* images;
    ubyte* constants;

    ImageHolder* originalImages;
    ubyte* originalConstants;
};

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
        auto itemData = (const MaterialEditorItemData*)item->data;

        ImageHolder* textureInfo = &itemData->images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(itemData->constants + this->bindlessOffset, &handle, sizeof(handle));
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
        auto itemData = (const MaterialEditorItemData*)item->data;

        ImageHolder* textureInfo = &itemData->images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(itemData->constants + this->bindlessOffset, &handle, sizeof(handle));
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
        auto itemData = (const MaterialEditorItemData*)item->data;

        memcpy(itemData->constants, this->data, this->dataSize);
        Materials::MaterialSetConstants(this->item->asset.material, this->data, this->dataSize);
        Materials::MaterialInvalidate(this->item->asset.material);
        return true;
    };

    bool Unexecute() override
    {
        assetEditor->Unedit();
        item->editCounter--;
        item->grabFocus = true;
        auto itemData = (const MaterialEditorItemData*)item->data;

        memcpy(itemData->constants, this->oldData, this->dataSize);
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
MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
    auto itemData = (const MaterialEditorItemData*)item->data;
    ImGui::PushFont(Dynui::ImguiContext::state.boldFont);
    ImGui::Text(materialTemplate->name);
    ImGui::PopFont();

    ImGui::Separator();

    bool applyChange = false;
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        auto& kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
        const MaterialTemplatesGPULang::MaterialTemplateTexture* value = kvp.Value();

        ImageHolder* textureInfo = &itemData->images[i];
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
            pressed |= ImGui::Button(Util::Format("%s###BUTTON%d", name.AsCharPtr(), i).AsCharPtr());
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_None))
            {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    ImGui::SetClipboardText(name.AsCharPtr());
                    pressed = false;
                }
                else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                {
                    const char* clipboard = ImGui::GetClipboardText();
                    if (clipboard != nullptr)
                    {
                        auto cmd = new CMDMaterialSetTexture;
                        cmd->bindlessOffset = value->bindlessOffset;
                        cmd->index = i;
                        cmd->hash = value->hashedName;
                        cmd->assetEditor = assetEditor;
                        cmd->asset = clipboard;
                        cmd->item = item;
                        cmd->oldAsset = name;
                        Edit::CommandManager::Execute(cmd);
                    }
                    pressed = false;
                }
            }
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
        const MaterialTemplatesGPULang::MaterialTemplateValue* value = kvp.Value();
        ubyte* imguiState = ArrayAllocStack<ubyte>(materialTemplate->bufferSize);
        ubyte* currentState = Materials::MaterialGetConstants(item->asset.material);
        memcpy(imguiState, currentState, materialTemplate->bufferSize);

        // FIXME remove when done with gpulang transition
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-compare-switch"
#endif
        switch (value->type)
        {
            case MaterialTemplates::MaterialTemplateValue::Type::Bool:
            {
                ImGui::Checkbox(kvp.Key(), (bool*)(imguiState + value->offset));
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Scalar:
            {
                ImGui::SliderFloat(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1000.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec2:
            {
                ImGui::SliderFloat2(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1000.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec3:
            {
                ImGui::SliderFloat3(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1000.0f);
                break;
            }
            case MaterialTemplates::MaterialTemplateValue::Type::Vec4:
            {
                ImGui::SliderFloat4(kvp.Key(), (float*)(imguiState + value->offset), 0.0f, 1000.0f);
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
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif

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
            memcpy(cmd->oldData, itemData->constants, cmd->dataSize);
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

        ArrayFreeStack(materialTemplate->bufferSize, imguiState);
    }
}


//------------------------------------------------------------------------------
/**
*/
void
MaterialSetup(AssetEditorItem* item)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);

    // Allocate editor specific data
    MaterialEditorItemData* itemData = item->allocator.Alloc<MaterialEditorItemData>();
    itemData->constants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
    itemData->images = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
    itemData->originalConstants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
    itemData->originalImages = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
    item->data = itemData;

    // Copy over material constants
    ubyte* currentData = Materials::MaterialGetConstants(item->asset.material);
    memcpy(itemData->constants, currentData, materialTemplate->bufferSize);
    memcpy(itemData->originalConstants, currentData, materialTemplate->bufferSize);

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        auto kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
        Resources::ResourceId res = Materials::MaterialGetTexture(item->asset.material, kvp.Value()->textureIndex);
        if (res.resourceId == Resources::InvalidResourceId.resourceId)
            res = Resources::CreateResource(kvp.Value()->resource, "editor");
        Resources::CreateResourceListener(res, [itemData, i](Resources::ResourceId res)
        {
            itemData->images[i].res = res;
            itemData->images[i].texture.layer = 0;
            itemData->images[i].texture.mip = 0;
            itemData->images[i].texture.nebulaHandle = res.resource;
            memcpy(&itemData->originalImages[i], &itemData->images[i], sizeof(ImageHolder));
        });

        itemData->images[i].res = res;
        itemData->images[i].texture.layer = 0;
        itemData->images[i].texture.mip = 0;
        itemData->images[i].texture.nebulaHandle = res.resource;
        memcpy(&itemData->originalImages[i], &itemData->images[i], sizeof(ImageHolder));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();
    auto itemData = (const MaterialEditorItemData*)item->data;
    memcpy(itemData->originalConstants, itemData->constants, materialTemplate->bufferSize);
    memcpy(itemData->originalImages, itemData->images, sizeof(ImageHolder) * materialTemplate->numTextures);

    Util::String output = Editor::PathConverter::StripAssetName(item->name.AsString());

    Util::String outFile = Util::Format("assets:%s.sur", output.AsCharPtr());
    Ptr<IO::FileStream> stream = IO::FileStream::Create();
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    stream->SetURI(outFile);

    Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
    writer->SetStream(stream);
    bool isOpen = writer->Open();
    n_assert(isOpen);
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
                    const MaterialTemplatesGPULang::MaterialTemplateTexture* value = kvp.Value();

                    ImageHolder* textureInfo = &itemData->images[i];
                    Util::String name = Editor::PathConverter::MapToCompactPath(texLoader->GetName(textureInfo->res).Value());
                    name.StripFileExtension();
                    writer->BeginNode(kvp.Key());
                        writer->SetString("value", name);
                    writer->EndNode();
                }
                for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
                {
                    auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
                    const MaterialTemplatesGPULang::MaterialTemplateValue* value = kvp.Value();
                    ubyte* currentState = Materials::MaterialGetConstants(item->asset.material);

                    writer->BeginNode(kvp.Key());
                    switch (value->type)
                    {
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Bool:
                        {
                            writer->SetString("value", Util::String::FromBool(*(bool*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Scalar:
                        {
                            writer->SetString("value", Util::String::FromFloat(*(float*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec2:
                        {
                            writer->SetString("value", Util::String::FromFloat2(*(Math::float2*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec3:
                        {
                            writer->SetString("value", Util::String::FromFloat3(*(Math::float3*)(currentState + value->offset)));
                            break;
                        }
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec4:
                        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Color:
                        {
                            writer->SetString("value", Util::String::FromFloat4(*(Math::float4*)(currentState + value->offset)));
                            break;
                        }
                        default:
                            n_warning("unhandled material template type");
                            break;
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

//------------------------------------------------------------------------------
/**
*/
void 
MaterialDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
    auto itemData = (const MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(item->asset.material);
    memcpy(itemData->constants, itemData->originalConstants, materialTemplate->bufferSize);
    Materials::MaterialSetConstants(item->asset.material, itemData->originalConstants, materialTemplate->bufferSize);
    Materials::MaterialInvalidate(item->asset.material);

    for (IndexT i = 0; i < materialTemplate->numTextures; i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* texBind = materialTemplate->textures.ValueAtIndex(i);
        ImageHolder* currentImage = &itemData->images[i];
        ImageHolder* originalImage = &itemData->originalImages[i];
        if (currentImage->res != originalImage->res)
        {
            Resources::DiscardResource(currentImage->res);
            if (texBind->bindlessOffset == 0xFFFFFFFF)
            {
                Materials::MaterialSetTexture(item->asset.material, texBind->hashedName, originalImage->res);
            }
        }
    }
}

} // namespace Presentation
