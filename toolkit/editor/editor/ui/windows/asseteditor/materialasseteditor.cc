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
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/assetbrowser.h"
#include "editor/tools/pathconverter.h"

#include "io/filestream.h"
#include "io/xmlwriter.h"
#include "toolkitutil/converters/binaryxmlconverter.h"

#include "toolkitutil/surface/surfacebuilder.h"
#include "toolkitutil/asset/assetpackager.h"

#include "visibility/visibilitycontext.h"

namespace Presentation
{



static CoreGraphics::MeshResourceId MaterialSphere = CoreGraphics::InvalidMeshResourceId;

//------------------------------------------------------------------------------
/**
    Formats a material constant the way the material loader parses it, an empty string if the type isn't handled
*/
static Util::String
FormatMaterialValue(const MaterialTemplatesGPULang::MaterialTemplateValue* value, const ubyte* constants)
{
    switch (value->type)
    {
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Bool:
            return Util::String::FromBool(*(bool*)(constants + value->offset));
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Scalar:
            return Util::String::FromFloat(*(float*)(constants + value->offset));
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec2:
            return Util::String::FromFloat2(*(Math::float2*)(constants + value->offset));
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec3:
            return Util::String::FromFloat3(*(Math::float3*)(constants + value->offset));
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Vec4:
        case MaterialTemplatesGPULang::MaterialTemplateValue::Type::Color:
            return Util::String::FromFloat4(*(Math::float4*)(constants + value->offset));
        default:
            n_warning("unhandled material template type");
            return Util::String();
    }
}

//------------------------------------------------------------------------------
/**
    Materials store textures as URNs, folder:file:tex. A texture the editor loaded is named after the file it
    was loaded from, which has to lie in the texture export folder to have one
*/
static bool
TextureURN(const Util::String& resourceName, Util::String& outURN)
{
    return AssetURN(resourceName, "tex", outURN);
}

//------------------------------------------------------------------------------
/**
*/
static Util::String
FindMaterialValue(const ToolkitUtil::MaterialResourceT& asset, const char* name)
{
    for (size_t i = 0; i < asset.value_names.size() && i < asset.values.size(); i++)
    {
        if (asset.value_names[i] == name)
            return asset.values[i];
    }
    return Util::String();
}

//------------------------------------------------------------------------------
/**
    Template textures and values are keyed by their names, which aren't shared between templates
*/
template <typename T>
static IndexT
FindTemplateEntry(const Util::Dictionary<const char*, T>& entries, const char* name)
{
    const Util::String key = name;
    for (IndexT i = 0; i < entries.Size(); i++)
    {
        if (key == entries.KeyAtIndex(i))
            return i;
    }
    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    True while the editor shows the template the live material was loaded with, which edits are previewed on
*/
static bool
EditsLiveMaterial(const AssetEditorItem* item)
{
    const MaterialEditorItemData* itemData = (const MaterialEditorItemData*)item->data;
    return itemData->editTemplate == Materials::MaterialGetTemplate(item->asset.material);
}

//------------------------------------------------------------------------------
/**
    The constants the widgets show, the live material's own while it has the template being edited
*/
static ubyte*
ShownConstants(const AssetEditorItem* item)
{
    if (EditsLiveMaterial(item))
        return Materials::MaterialGetConstants(item->asset.material);
    return ((const MaterialEditorItemData*)item->data)->shownConstants;
}

//------------------------------------------------------------------------------
/**
*/
static void
SetShownConstants(const AssetEditorItem* item, const void* data, uint size)
{
    if (EditsLiveMaterial(item))
    {
        Materials::MaterialSetConstants(item->asset.material, data, size);
        Materials::MaterialInvalidate(item->asset.material);
    }
    else
    {
        memcpy(((const MaterialEditorItemData*)item->data)->shownConstants, data, size);
    }
}

//------------------------------------------------------------------------------
/**
    Points an editor image at a texture, the image keeps its imgui texture id
*/
static void
SetImage(ImageHolder& image, Resources::ResourceId res)
{
    image.res = res;
    image.texture.layer = 0;
    image.texture.mip = 0;
    image.texture.nebulaHandle = res.resource;
    Dynui::SetImguiTextureIdData(image.textureId, image.texture);
}

//------------------------------------------------------------------------------
/**
    Puts the live material back the way it was loaded or last saved, while the editor shows its template.
    The textures the editor loaded are released when it's done with them
*/
static void
RestoreLiveMaterial(AssetEditorItem* item, bool releaseTextures)
{
    const MaterialEditorItemData* itemData = (const MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* liveTemplate = Materials::MaterialGetTemplate(item->asset.material);
    Materials::MaterialSetConstants(item->asset.material, itemData->originalConstants, liveTemplate->bufferSize);
    Materials::MaterialInvalidate(item->asset.material);
    for (IndexT i = 0; i < liveTemplate->textures.Size(); i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* texBind = liveTemplate->textures.ValueAtIndex(i);
        if (itemData->images[i].res != itemData->liveImages[i].res)
        {
            if (releaseTextures)
            {
                Resources::DiscardResource(itemData->images[i].res);
            }
            if (texBind->bindlessOffset == 0xFFFFFFFF)
            {
                Materials::MaterialSetTexture(item->asset.material, texBind->hashedName, itemData->liveImages[i].res);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Gives the live material the editor's textures and constants, when the editor shows its template again
*/
static void
ApplyToLiveMaterial(AssetEditorItem* item)
{
    const MaterialEditorItemData* itemData = (const MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* liveTemplate = Materials::MaterialGetTemplate(item->asset.material);
    Materials::MaterialSetConstants(item->asset.material, itemData->constants, liveTemplate->bufferSize);
    Materials::MaterialInvalidate(item->asset.material);
    for (IndexT i = 0; i < liveTemplate->textures.Size(); i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* texBind = liveTemplate->textures.ValueAtIndex(i);
        if (texBind->bindlessOffset == 0xFFFFFFFF)
        {
            Materials::MaterialSetTexture(item->asset.material, texBind->hashedName, itemData->images[i].res);
        }
    }
}

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
        auto itemData = (const MaterialEditorItemData*)item->data;
        // the slot belongs to a template the material isn't edited with anymore
        if (itemData->editTemplate != this->editTemplate)
        {
            return true;
        }
        this->res = Resources::CreateResource(this->asset, "editor", nullptr, nullptr, true, false);
        this->Apply(itemData);
        return true;
    };

    bool Unexecute() override
    {
        assetEditor->Unedit();
        item->grabFocus = true;
        item->editCounter--;
        auto itemData = (const MaterialEditorItemData*)item->data;
        if (itemData->editTemplate != this->editTemplate)
        {
            return true;
        }
        Resources::DiscardResource(this->res);
        this->res = Resources::CreateResource(this->oldAsset, "editor", nullptr, nullptr, true, false);
        this->Apply(itemData);
        return true;
    };

    /// show the texture in its slot, and on the live material while it has the template
    void Apply(const MaterialEditorItemData* itemData)
    {
        ImageHolder* textureInfo = &itemData->images[this->index];
        textureInfo->res = res;
        textureInfo->texture.nebulaHandle = res.resourceId;
        const bool live = EditsLiveMaterial(this->item);
        if (this->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(itemData->constants + this->bindlessOffset, &handle, sizeof(handle));
            if (live)
            {
                Materials::MaterialSetConstant(this->item->asset.material, &handle, sizeof(handle), this->bindlessOffset);
                Materials::MaterialInvalidate(this->item->asset.material);
            }
        }
        else if (live)
        {
            Materials::MaterialSetTexture(this->item->asset.material, this->hash, this->res);
        }
    }

    AssetEditorItem* item;
    AssetEditor* assetEditor;
    const MaterialTemplatesGPULang::Entry* editTemplate;
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
        // the constants are laid out for a template the material isn't edited with anymore
        if (itemData->editTemplate != this->editTemplate)
        {
            return true;
        }

        memcpy(itemData->constants, this->data, this->dataSize);
        SetShownConstants(this->item, this->data, this->dataSize);
        return true;
    };

    bool Unexecute() override
    {
        assetEditor->Unedit();
        item->editCounter--;
        item->grabFocus = true;
        auto itemData = (const MaterialEditorItemData*)item->data;
        if (itemData->editTemplate != this->editTemplate)
        {
            return true;
        }

        memcpy(itemData->constants, this->oldData, this->dataSize);
        SetShownConstants(this->item, this->oldData, this->dataSize);
        return true;
    };

    ubyte* data;
    ubyte* oldData;
    uint dataSize;
    AssetEditorItem* item;
    AssetEditor* assetEditor;
    const MaterialTemplatesGPULang::Entry* editTemplate;
};

//------------------------------------------------------------------------------
/**
    Sets the texture picked for a slot of a material. The material and its slot are looked up again, the material
    can have been closed, or moved by other assets being opened, while the texture was picked
*/
static void
SetPickedTexture(AssetEditor* assetEditor, const IO::Path& material, const Util::String& slot, const IO::Path& texture, const Util::String& oldAsset)
{
    if (!texture.IsFile() || texture.GetType() != "tex")
    {
        n_warning("'%s' is not a texture\n", texture.AsString().AsCharPtr());
        return;
    }
    AssetEditorItem* item = AssetEditor::GetOpenItem(material);
    if (item == nullptr || item->assetType != AssetEditor::AssetType::Material)
    {
        return;
    }

    const MaterialTemplatesGPULang::Entry* materialTemplate = ((const MaterialEditorItemData*)item->data)->editTemplate;
    const IndexT i = FindTemplateEntry(materialTemplate->textures, slot.AsCharPtr());
    if (i != InvalidIndex)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* value = materialTemplate->textures.ValueAtIndex(i);
        auto cmd = new CMDMaterialSetTexture;
        cmd->bindlessOffset = value->bindlessOffset;
        cmd->index = i;
        cmd->hash = value->hashedName;
        cmd->assetEditor = assetEditor;
        cmd->editTemplate = materialTemplate;
        // named after the file it's loaded from, the same name loading the texture by its URN gives it
        cmd->asset = texture.ExportURI().GetHostAndLocalPath();
        cmd->item = item;
        cmd->oldAsset = oldAsset;
        Edit::CommandManager::Execute(cmd);
    }
}

//------------------------------------------------------------------------------
/**
    Shows and edits the material with another template. Textures and values the templates share by name and type
    carry over, the rest start at the new template's defaults. The live material can't change its template, it's
    put back as loaded while another template is edited, and gets the edits again when its own template is picked
*/
static void
SwitchTemplate(AssetEditor* assetEditor, AssetEditorItem* item, const MaterialTemplatesGPULang::Entry* newTemplate)
{
    MaterialEditorItemData* itemData = (MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* oldTemplate = itemData->editTemplate;
    const MaterialTemplatesGPULang::Entry* liveTemplate = Materials::MaterialGetTemplate(item->asset.material);
    if (newTemplate == oldTemplate)
    {
        return;
    }
    if (oldTemplate == liveTemplate)
    {
        RestoreLiveMaterial(item, false);
    }

    ubyte* constants = item->allocator.Alloc<ubyte>(newTemplate->bufferSize);
    Memory::Clear(constants, newTemplate->bufferSize);
    for (IndexT i = 0; i < newTemplate->values.Size(); i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateValue* value = newTemplate->values.ValueAtIndex(i);
        const ubyte* source = (const ubyte*)&value->data;
        const IndexT j = FindTemplateEntry(oldTemplate->values, newTemplate->values.KeyAtIndex(i));
        if (j != InvalidIndex && oldTemplate->values.ValueAtIndex(j)->type == value->type)
        {
            source = itemData->constants + oldTemplate->values.ValueAtIndex(j)->offset;
        }
        memcpy(constants + value->offset, source, value->GetSize());
    }

    // the originals are what the work file has for a slot, which is the live material's texture on its own template
    ImageHolder* images = item->allocator.Alloc<ImageHolder>(newTemplate->numTextures);
    ImageHolder* originalImages = item->allocator.Alloc<ImageHolder>(newTemplate->numTextures);
    for (IndexT i = 0; i < newTemplate->textures.Size(); i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateTexture* texture = newTemplate->textures.ValueAtIndex(i);
        const IndexT j = FindTemplateEntry(oldTemplate->textures, newTemplate->textures.KeyAtIndex(i));
        Resources::ResourceId res, original;
        if (j != InvalidIndex)
        {
            res = itemData->images[j].res;
            original = itemData->originalImages[j].res;
        }
        else
        {
            res = Resources::CreateResource(IO::Path::Parse(texture->resource), "editor");
            original = res;
        }
        if (newTemplate == liveTemplate)
        {
            original = itemData->liveImages[i].res;
        }

        images[i].textureId = Dynui::AllocateImguiTextureId({});
        SetImage(images[i], res);
        originalImages[i] = images[i];
        originalImages[i].res = original;
        if (texture->bindlessOffset != 0xFFFFFFFF)
        {
            CoreGraphics::TextureIdLock _0(res);
            uint handle = CoreGraphics::TextureGetBindlessHandle(res);
            memcpy(constants + texture->bindlessOffset, &handle, sizeof(handle));
        }
    }
    for (IndexT i = 0; i < oldTemplate->textures.Size(); i++)
    {
        Dynui::DeallocateImguiTextureId(itemData->images[i].textureId);
    }

    itemData->editTemplate = newTemplate;
    itemData->images = images;
    itemData->originalImages = originalImages;
    itemData->constants = constants;
    if (newTemplate == liveTemplate)
    {
        ApplyToLiveMaterial(item);
    }
    else
    {
        itemData->shownConstants = item->allocator.Alloc<ubyte>(newTemplate->bufferSize);
        memcpy(itemData->shownConstants, constants, newTemplate->bufferSize);
    }
    assetEditor->Edit();
    item->editCounter++;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    auto itemData = (const MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* materialTemplate = itemData->editTemplate;

    ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
    if (ImGui::BeginCombo("Template", materialTemplate->name))
    {
        for (const auto& it : MaterialTemplatesGPULang::Lookup)
        {
            const MaterialTemplatesGPULang::Entry* entry = it.Value();
            if (ImGui::Selectable(entry->name, entry == materialTemplate) && entry != materialTemplate)
            {
                SwitchTemplate(assetEditor, item, entry);
                materialTemplate = entry;
            }            
        }
        ImGui::EndCombo();
    }
    ImGui::PopFont();
    if (!EditsLiveMaterial(item))
    {
        ImGui::TextDisabled("The preview shows the template '%s' until the saved material is loaded again", Materials::MaterialGetTemplate(item->asset.material)->name);
    }

    ImGui::Separator();

    bool applyChange = false;
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();
    if (ImGui::BeginTable("Material Editor Contents", itemData->renderViewport ? 2 : 1, ImGuiTableFlags_Resizable))
    {
        if (itemData->renderViewport)
        {
            ImGui::TableSetupScrollFreeze(2, 1);
            ImGui::TableNextColumn();
            assetEditor->viewport.RenderInChildWindow();
        }
        else
        {
            ImGui::TableSetupScrollFreeze(1, 1);
        }
        ImGui::TableNextColumn();
        for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
        {
            auto& kvp = materialTemplate->textures.KeyValuePairAtIndex(i);
            const MaterialTemplatesGPULang::MaterialTemplateTexture* value = kvp.Value();

            ImageHolder* textureInfo = &itemData->images[i];
            Util::String name = Editor::PathConverter::MapToCompactPath(texLoader->GetName(textureInfo->res).Value());
            if (!name.IsEmpty())
            {
                ImTextureRef ref {textureInfo->textureId};
                ImGui::Text(kvp.Key());
                ImGui::Image(ref, ImVec2{ 32, 32 });
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Image(ref, ImVec2{ 256, 256 });
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();

                // textures exported from the work folder are shown by their URN, others, like system textures, by their file
                Util::String urn;
                IO::Path texture;
                if (AssetURN(texLoader->GetName(textureInfo->res).Value(), "tex", urn))
                    texture = IO::Path::Parse(urn);
                AssetField(kvp.Key(), urn.IsEmpty() ? name : urn, texture, "tex",
                    [assetEditor, material = item->path, slot = Util::String(kvp.Key()), oldAsset = name](const IO::Path& texture)
                {
                    SetPickedTexture(assetEditor, material, slot, texture, oldAsset);
                });
            }
        }

        uint textureCounter = 0;
        for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
        {
            auto& kvp = materialTemplate->values.KeyValuePairAtIndex(i);
            const MaterialTemplatesGPULang::MaterialTemplateValue* value = kvp.Value();
            ubyte* imguiState = ArrayAllocStack<ubyte>(materialTemplate->bufferSize);
            ubyte* currentState = ShownConstants(item);
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
                    ImGui::DragFloat(kvp.Key(), (float*)(imguiState + value->offset), 0.01f, 0.0f, FLT_MAX);
                    break;
                }
                case MaterialTemplates::MaterialTemplateValue::Type::Vec2:
                {
                    ImGui::DragFloat2(kvp.Key(), (float*)(imguiState + value->offset), 0.01f, 0.0f, FLT_MAX);
                    break;
                }
                case MaterialTemplates::MaterialTemplateValue::Type::Vec3:
                {
                    ImGui::DragFloat3(kvp.Key(), (float*)(imguiState + value->offset), 0.01f, 0.0f, FLT_MAX);
                    break;
                }
                case MaterialTemplates::MaterialTemplateValue::Type::Vec4:
                {
                    ImGui::DragFloat4(kvp.Key(), (float*)(imguiState + value->offset), 0.01f, 0.0f, FLT_MAX);
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
                cmd->editTemplate = materialTemplate;
                cmd->item = item;
                memcpy(cmd->data, imguiState, cmd->dataSize);
                memcpy(cmd->oldData, itemData->constants, cmd->dataSize);
                Edit::CommandManager::Execute(cmd);
            }

            // Update live state
            if (ImGui::IsItemEdited())
            {
                SetShownConstants(item, imguiState, materialTemplate->bufferSize);
            }

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && kvp.Value()->desc != nullptr)
            {
                ImGui::SetTooltip(kvp.Value()->desc);
            }

            ArrayFreeStack(materialTemplate->bufferSize, imguiState);
        }
        ImGui::EndTable();
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
    itemData->editTemplate = materialTemplate;
    itemData->constants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
    itemData->images = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
    itemData->originalConstants = item->allocator.Alloc<ubyte>(materialTemplate->bufferSize);
    itemData->originalImages = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
    itemData->liveImages = item->allocator.Alloc<ImageHolder>(materialTemplate->numTextures);
    item->data = itemData;
    
    Ptr<IO::Stream> assetFileStream = IO::CreateStream(item->path.WorkURI("assets"));
    if (assetFileStream->Open())
    {
        void* data = assetFileStream->MemoryMap();

        Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::MaterialResource>(itemData->asset, (uint8_t*)data);

        assetFileStream->MemoryUnmap();
        assetFileStream->Close();
    }

    if (MaterialSphere == CoreGraphics::InvalidMeshResourceId)
    {
        MaterialSphere = Resources::CreateResource("system:material_knob:msh"_path, "preview", nullptr, nullptr, true, false).resourceId;
    }

    Models::ModelContext::RegisterEntity(item->previewObject);
    Models::ModelContext::Setup(
        item->previewObject,
        Math::mat4(),
        item->asset.material,
        CoreGraphics::MeshResourceGetMesh(MaterialSphere, 0),
        0,
        Graphics::StageMask(1 << 3)
    );
    Models::ModelContext::SetAlwaysVisible(item->previewObject);
    Visibility::ObservableContext::RegisterEntity(item->previewObject);
    Visibility::ObservableContext::Setup(item->previewObject, Visibility::VisibilityEntityType::Model);

    // Copy over material constants
    ubyte* currentData = Materials::MaterialGetConstants(item->asset.material);
    memcpy(itemData->constants, currentData, materialTemplate->bufferSize);
    memcpy(itemData->originalConstants, currentData, materialTemplate->bufferSize);

    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        itemData->images[i].textureId = Dynui::AllocateImguiTextureId({});
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
            Dynui::SetImguiTextureIdData(itemData->images[i].textureId, itemData->images[i].texture);
            memcpy(&itemData->originalImages[i], &itemData->images[i], sizeof(ImageHolder));
            memcpy(&itemData->liveImages[i], &itemData->images[i], sizeof(ImageHolder));
        });

        itemData->images[i].res = res;
        itemData->images[i].texture.layer = 0;
        itemData->images[i].texture.mip = 0;
        itemData->images[i].texture.nebulaHandle = res.resource;
        Dynui::SetImguiTextureIdData(itemData->images[i].textureId, itemData->images[i].texture);
        memcpy(&itemData->originalImages[i], &itemData->images[i], sizeof(ImageHolder));
        memcpy(&itemData->liveImages[i], &itemData->images[i], sizeof(ImageHolder));
    }
}

//------------------------------------------------------------------------------
/**
    Writes the work file, a MaterialResource flatbuffer, and exports it the way the batcher does.
    Textures keep the URN the work file has for them unless they were changed since the last save,
    all constants of the template edited are written.
*/
void
MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item)
{
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();
    MaterialEditorItemData* itemData = (MaterialEditorItemData*)item->data;
    const MaterialTemplatesGPULang::Entry* materialTemplate = itemData->editTemplate;

    ToolkitUtil::MaterialResourceT asset;
    asset.template_name = materialTemplate->name;
    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        const char* name = materialTemplate->textures.KeyAtIndex(i);
        Util::String urn;
        if (itemData->images[i].res != itemData->originalImages[i].res)
        {
            const Util::String resourceName = texLoader->GetName(itemData->images[i].res).Value();
            if (!TextureURN(resourceName, urn))
            {
                n_warning("Texture '%s' isn't an exported texture, '%s' keeps its previous texture\n", resourceName.AsCharPtr(), name);
            }
        }
        if (urn.IsEmpty())
        {
            // textures the work file doesn't have use the template default
            urn = FindMaterialValue(itemData->asset, name);
        }
        if (!urn.IsEmpty())
        {
            asset.value_names.push_back(name);
            asset.values.push_back(urn);
        }
    }
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        const Util::String value = FormatMaterialValue(materialTemplate->values.ValueAtIndex(i), itemData->constants);
        if (!value.IsEmpty())
        {
            asset.value_names.push_back(materialTemplate->values.KeyAtIndex(i));
            asset.values.push_back(value);
        }
    }

    const IO::URI workFile = item->path.WorkURI("assets");
    Ptr<IO::Stream> stream = IO::CreateStream(workFile);
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    if (!stream->Open())
    {
        n_warning("Failed to save material '%s'\n", workFile.LocalPath().AsCharPtr());
        return;
    }
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MaterialResource>(asset);
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();

    // the live material already shows the changes when it has the template edited, so the export isn't reloaded.
    // Saved with another template, the material shows it once it's loaded again
    const IO::URI exportFolder = Util::Format("mat:%s", item->path.GetFolder().AsCharPtr());
    IO::CreateDirectory(exportFolder);
    ToolkitUtil::Logger logger;
    if (!ToolkitUtil::PackageMaterial(&asset, item->path.GetFile(), exportFolder, ToolkitUtil::Platform::Code::Win32, &logger))
    {
        n_warning("Failed to export material '%s'\n", item->path.AsString().AsCharPtr());
    }

    itemData->asset = asset;
    memcpy(itemData->originalImages, itemData->images, sizeof(ImageHolder) * materialTemplate->numTextures);
    if (EditsLiveMaterial(item))
    {
        memcpy(itemData->originalConstants, itemData->constants, materialTemplate->bufferSize);
        memcpy(itemData->liveImages, itemData->images, sizeof(ImageHolder) * materialTemplate->numTextures);
    }
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
    const MaterialTemplatesGPULang::Entry* materialTemplate = itemData->editTemplate;

    // with another template edited, the live material was put back when the template was switched
    if (EditsLiveMaterial(item))
    {
        memcpy(itemData->constants, itemData->originalConstants, materialTemplate->bufferSize);
        RestoreLiveMaterial(item, true);
    }
    for (IndexT i = 0; i < materialTemplate->numTextures; i++)
    {
        Dynui::DeallocateImguiTextureId(itemData->images[i].textureId);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
    Models::ModelContext::SetStageMask(item->previewObject, show ? 1 << 3 : 0x0);
}

//------------------------------------------------------------------------------
/**
    Writes a new material work file with a template (like GLTF, the one the importer gives opaque
    materials), using the template's defaults for all of its values
*/
void
MaterialNew(const Ptr<IO::Stream>& stream, const Util::String& path, const Util::String& templateName)
{
    ToolkitUtil::MaterialResourceT material;
    material.template_name = templateName;
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MaterialResource>(material);
    stream->Write(data.GetPtr(), data.Size());
}

//------------------------------------------------------------------------------
/**
*/
void 
MaterialSerialize(const Ptr<IO::Stream>& stream, const MaterialEditorItemData* data, Materials::MaterialId mat, const MaterialTemplatesGPULang::Entry* materialTemplate)
{
    Ptr<IO::XmlWriter> writer = IO::XmlWriter::Create();
    writer->SetStream(stream);
    bool isOpen = writer->Open();
    n_assert(isOpen);
    CoreGraphics::TextureLoader* texLoader = Resources::GetStreamLoader<CoreGraphics::TextureLoader>();
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

                    ImageHolder* textureInfo = &data->images[i];
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
                    ubyte* currentState = Materials::MaterialGetConstants(mat);

                    writer->BeginNode(kvp.Key());
                    const Util::String formatted = FormatMaterialValue(value, currentState);
                    if (!formatted.IsEmpty())
                    {
                        writer->SetString("value", formatted);
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
}

} // namespace Presentation
