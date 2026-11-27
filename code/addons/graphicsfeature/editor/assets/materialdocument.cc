//------------------------------------------------------------------------------
//  @file materialdocument.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "graphicsfeature/editor/assets/materialdocument.h"

#if WITH_NEBULA_EDITOR
#include "graphicsfeature/editor/assets/assetpreview.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/assetfield.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "coregraphics/texture.h"
#include "materials/gpulang/materialtemplatesgpulang.h"
#include "models/modelcontext.h"
#include "resources/resourceserver.h"
#include "visibility/visibilitycontext.h"
#include "io/ioserver.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/material.h"
#include "imgui.h"

namespace GraphicsFeature
{

__ImplementClass(GraphicsFeature::MaterialDocument, 'GMTD', Editor::AssetDocument);

//------------------------------------------------------------------------------
/**
    A value of the material set to new bytes. A value is at most a vec4.
*/
class SetMaterialValueCommand : public Editor::EditCommand
{
    __DeclareClass(SetMaterialValueCommand)
public:
    const char* Name() const override { return "Set material value"; }
    bool Execute() override
    {
        this->document->ApplyValue(this->offset, this->newBytes, this->size);
        return true;
    }
    bool Unexecute() override
    {
        this->document->ApplyValue(this->offset, this->oldBytes, this->size);
        return true;
    }

    Ptr<MaterialDocument> document;
    uint offset = 0;
    uint size = 0;
    ubyte oldBytes[16] = { 0 };
    ubyte newBytes[16] = { 0 };
};
__ImplementClass(GraphicsFeature::SetMaterialValueCommand, 'GMVC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    A texture slot of the material set to another texture. An empty path is the template's default.
*/
class SetMaterialTextureCommand : public Editor::EditCommand
{
    __DeclareClass(SetMaterialTextureCommand)
public:
    const char* Name() const override { return "Set material texture"; }
    bool Execute() override
    {
        this->document->ApplyTexture(this->slot, this->newTexture);
        return true;
    }
    bool Unexecute() override
    {
        this->document->ApplyTexture(this->slot, this->oldTexture);
        return true;
    }

    Ptr<MaterialDocument> document;
    IndexT slot = InvalidIndex;
    IO::Path oldTexture;
    IO::Path newTexture;
};
__ImplementClass(GraphicsFeature::SetMaterialTextureCommand, 'GMXC', Editor::EditCommand);

//------------------------------------------------------------------------------
/**
    The text a value is written to the work file as, the way the material loader reads it back.
    A vec3 is read as a vec4, so it is written as one.
*/
static Util::String
ValueString(const MaterialTemplatesGPULang::MaterialTemplateValue* value, const ubyte* data)
{
    switch (value->type)
    {
        case MaterialTemplatesGPULang::MaterialTemplateValue::Bool:
            return Util::String::FromBool(*(const bool*)data);
        case MaterialTemplatesGPULang::MaterialTemplateValue::Scalar:
            return Util::String::FromFloat(*(const float*)data);
        case MaterialTemplatesGPULang::MaterialTemplateValue::Int:
            return Util::String::FromInt(*(const int*)data);
        case MaterialTemplatesGPULang::MaterialTemplateValue::Vec2:
            return Util::String::FromFloat2(*(const Math::float2*)data);
        case MaterialTemplatesGPULang::MaterialTemplateValue::Vec3:
        case MaterialTemplatesGPULang::MaterialTemplateValue::Vec4:
        case MaterialTemplatesGPULang::MaterialTemplateValue::Color:
            return Util::String::FromFloat4(*(const Math::float4*)data);
        default:
            return Util::String();
    }
}

//------------------------------------------------------------------------------
/**
    Write a work file, the flatbuffer the batcher exports to a material
*/
static bool
WriteWorkFile(const IO::URI& workFile, const ToolkitUtil::MaterialResourceT& material, Util::String& outError)
{
    Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::MaterialResource>(material);
    Ptr<IO::Stream> stream = IO::CreateStream(workFile);
    stream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    if (!stream->Open())
    {
        outError.Format("could not write '%s'", workFile.LocalPath().AsCharPtr());
        return false;
    }
    stream->Write(data.GetPtr(), data.Size());
    stream->Close();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
MaterialDocument::MaterialDocument() :
    resource(Resources::InvalidResourceId),
    previewObject(Graphics::InvalidGraphicsEntityId),
    pendingSlot(InvalidIndex),
    shown(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
MaterialDocument::~MaterialDocument()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::OpenAsset(const IO::Path& asset)
{
    Ptr<MaterialDocument> document = MaterialDocument::Create();
    document->asset = asset;
    Editor::AssetEditorPanel::Open(document.upcast<Editor::AssetDocument>());
}

//------------------------------------------------------------------------------
/**
    Only the template is written, the material takes the template's defaults for everything else.
    Without a variant the template the importer gives opaque materials is used.
*/
bool
MaterialDocument::CreateAsset(const IO::Path& asset, const IO::URI& workFile, const Util::String& variant, Util::String& outError)
{
    (void)asset;
    ToolkitUtil::MaterialResourceT material;
    material.template_name = variant.IsEmpty() ? Util::String("GLTF") : variant;
    return WriteWorkFile(workFile, material, outError);
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::ListTemplates(Util::Array<Util::String>& out)
{
    for (IndexT i = 0; i < MaterialTemplatesGPULang::Lookup.Size(); i++)
        out.Append(MaterialTemplatesGPULang::Lookup.ValueAtIndex(i)->name);
    out.Sort();
}

//------------------------------------------------------------------------------
/**
*/
bool
MaterialDocument::CanSave() const
{
    return true;
}

//------------------------------------------------------------------------------
/**
    The material is loaded the way the scene loads it, so an asset the level uses is the same
    material, and a change shows in both.
*/
bool
MaterialDocument::OnOpen()
{
    if (!Resources::ResourceServer::Instance()->HasStreamLoader(this->asset))
        return false;

    this->ReadWorkFile();

    Ptr<MaterialDocument> self = this;
    this->resource = Resources::CreateResource(this->asset, "editor",
        [self](Resources::ResourceId id)
        {
            self->Loaded(id);
        },
        [self](Resources::ResourceId id)
        {
            self->SetFailed("the material could not be loaded");
        });
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::ReadWorkFile()
{
    this->workTemplate.Clear();
    this->workValues.Clear();

    Ptr<IO::Stream> stream = IO::CreateStream(this->asset.WorkURI("assets"));
    stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
    if (!stream->Open())
        return;

    ToolkitUtil::MaterialResourceT work;
    const void* data = stream->MemoryMap();
    Flat::FlatbufferInterface::DeserializeFlatbuffer<ToolkitUtil::MaterialResource>(work, (const uint8_t*)data);
    stream->MemoryUnmap();
    stream->Close();

    this->workTemplate = work.template_name;
    const size_t count = Math::min(work.value_names.size(), work.values.size());
    for (size_t i = 0; i < count; i++)
        this->workValues.Emplace(work.value_names[i]) = work.values[i];
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::Loaded(Resources::ResourceId id)
{
    if (this->IsClosed())
        return;

    this->resource = id;
    this->material = Materials::MaterialId(id);
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(this->material);

    this->committed.Resize(materialTemplate->bufferSize);
    if (materialTemplate->bufferSize > 0)
        Memory::Copy(Materials::MaterialGetConstants(this->material), this->committed.Begin(), materialTemplate->bufferSize);

    // a slot shows the texture the work file gives it, by URN, the template's default otherwise
    for (IndexT i = 0; i < materialTemplate->textures.Size(); i++)
    {
        Slot slot;
        const Util::String name = materialTemplate->textures.KeyAtIndex(i);
        IndexT valueIndex = this->workValues.FindIndex(name);
        if (valueIndex != InvalidIndex)
        {
            const IO::Path texture = IO::Path::Parse(this->workValues.ValueAtIndex(valueIndex));
            if (texture.IsFile() && texture.GetType() == "tex")
                slot.texture = slot.original = texture;
        }
        slot.imageId = Dynui::AllocateImguiTextureId(slot.image);
        slot.hasImageId = true;
        this->slots.Append(slot);
    }

    AssetPreview* preview = AssetPreview::Get();
    if (preview != nullptr)
    {
        this->previewObject = Graphics::CreateEntity();
        Models::ModelContext::RegisterEntity(this->previewObject);
        Models::ModelContext::Setup(this->previewObject, Math::mat4(), this->material, preview->GetMaterialKnob(), 0, this->shown ? AssetPreview::Stage : 0);
        Models::ModelContext::SetAlwaysVisible(this->previewObject);
        Visibility::ObservableContext::RegisterEntity(this->previewObject);
        Visibility::ObservableContext::Setup(this->previewObject, Visibility::VisibilityEntityType::Model);
    }

    this->SetReady();
}

//------------------------------------------------------------------------------
/**
    A texture this document bound stays with the material when its change was saved, the scene's
    material shows it. One that is back to what the material was loaded with is released, the
    loader holds that one itself.
*/
void
MaterialDocument::OnClose()
{
    if (this->previewObject != Graphics::InvalidGraphicsEntityId)
    {
        if (Visibility::ObservableContext::IsEntityRegistered(this->previewObject))
            Visibility::ObservableContext::DeregisterEntityImmediate(this->previewObject);
        if (Models::ModelContext::IsEntityRegistered(this->previewObject))
            Models::ModelContext::DeregisterEntityImmediate(this->previewObject);
        Graphics::DestroyEntity(this->previewObject);
        this->previewObject = Graphics::InvalidGraphicsEntityId;
    }

    for (Slot& slot : this->slots)
    {
        if (slot.hasImageId)
            Dynui::DeallocateImguiTextureId(slot.imageId);
        if (slot.held != Resources::InvalidResourceId && slot.texture == slot.original)
            Resources::DiscardResource(slot.held);
    }
    this->slots.Clear();

    if (this->resource != Resources::InvalidResourceId)
    {
        Resources::DiscardResource(this->resource);
        this->resource = Resources::InvalidResourceId;
    }

    if (this->shown)
    {
        AssetPreview* preview = AssetPreview::Peek();
        if (preview != nullptr)
            preview->Hide();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::OnShown(bool shown)
{
    this->shown = shown;
    if (this->previewObject != Graphics::InvalidGraphicsEntityId)
        Models::ModelContext::SetStageMask(this->previewObject, shown ? AssetPreview::Stage : 0);
    if (!shown)
    {
        AssetPreview* preview = AssetPreview::Peek();
        if (preview != nullptr)
            preview->Hide();
    }
}

//------------------------------------------------------------------------------
/**
    The constants are written whole, which is what also updates the copy the bindless templates
    read. They are read back from the material first, so texture handles bound since are kept.
*/
void
MaterialDocument::ApplyValue(uint offset, const void* data, uint size)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(this->material);
    const uint bufferSize = materialTemplate->bufferSize;
    n_assert(offset + size <= bufferSize);

    Util::Array<ubyte> constants;
    constants.Resize(bufferSize);
    Memory::Copy(Materials::MaterialGetConstants(this->material), constants.Begin(), bufferSize);
    Memory::Copy(data, constants.Begin() + offset, size);
    Materials::MaterialSetConstants(this->material, constants.Begin(), bufferSize);

    Memory::Copy(data, this->committed.Begin() + offset, size);
}

//------------------------------------------------------------------------------
/**
    Bound the way the material loader binds it. The constants are written again after, which is
    what hands a bindless handle to the copy the bindless templates read.
*/
void
MaterialDocument::ApplyTexture(IndexT slot, const IO::Path& texture)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(this->material);
    const MaterialTemplatesGPULang::MaterialTemplateTexture* value = materialTemplate->textures.ValueAtIndex(slot);

    const IO::Path source = texture.IsEmpty() ? IO::Path::Parse(value->resource) : texture;
    Resources::ResourceId tex = Resources::CreateResource(source, "editor", nullptr, nullptr, { .immediate = true, .stream = false });
    {
        CoreGraphics::TextureIdLock _0(tex);
        if (value->bindlessOffset != 0xFFFFFFFF)
        {
            uint handle = CoreGraphics::TextureGetBindlessHandle(tex);
            Materials::MaterialSetTextureBindless(this->material, value->hashedName, handle, value->bindlessOffset, tex);
        }
        else
        {
            Materials::MaterialSetTexture(this->material, value->hashedName, tex);
        }
    }

    const uint bufferSize = materialTemplate->bufferSize;
    if (bufferSize > 0)
    {
        Util::Array<ubyte> constants;
        constants.Resize(bufferSize);
        Memory::Copy(Materials::MaterialGetConstants(this->material), constants.Begin(), bufferSize);
        Materials::MaterialSetConstants(this->material, constants.Begin(), bufferSize);
    }

    // the texture the slot had is no longer bound, the one loaded for it here is the slot's now
    Slot& entry = this->slots[slot];
    if (entry.held != Resources::InvalidResourceId)
        Resources::DiscardResource(entry.held);
    entry.held = tex;
    entry.texture = texture;
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::RunPendingTexture()
{
    if (this->pendingSlot == InvalidIndex)
        return;

    const IndexT slot = this->pendingSlot;
    const IO::Path texture = this->pendingTexture;
    this->pendingSlot = InvalidIndex;
    this->pendingTexture = IO::Path();
    if (slot >= this->slots.Size() || this->slots[slot].texture == texture)
        return;

    Ptr<SetMaterialTextureCommand> command = SetMaterialTextureCommand::Create();
    command->document = this;
    command->slot = slot;
    command->oldTexture = this->slots[slot].texture;
    command->newTexture = texture;
    this->commands->Execute(command.upcast<Editor::EditCommand>());
}

//------------------------------------------------------------------------------
/**
*/
void
MaterialDocument::Draw()
{
    this->RunPendingTexture();

    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(this->material);
    ImGui::PushFont(Dynui::ImguiBoldFont, 0.0f);
    ImGui::Text("Template: %s", materialTemplate->name);
    ImGui::PopFont();
    ImGui::SetItemTooltip("A loaded material can't take another template, so it is only shown here");
    ImGui::Separator();

    // a table cell has no height of its own, so both columns get what is left of the window
    const float height = Math::max(ImGui::GetContentRegionAvail().y, 64.0f);
    if (ImGui::BeginTable("material", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableNextColumn();
        AssetPreview* preview = AssetPreview::Get();
        if (preview != nullptr)
            preview->Draw(ImVec2(ImGui::GetContentRegionAvail().x, height));
        else
            ImGui::TextDisabled("The preview could not be made");

        ImGui::TableNextColumn();
        if (ImGui::BeginChild("parameters", ImVec2(0.0f, height)))
        {
            this->DrawTextures(materialTemplate);
            ImGui::Separator();
            this->DrawValues(materialTemplate);
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
}

//------------------------------------------------------------------------------
/**
    A field takes a texture while it draws, or after the frame when the picker answers. The change
    becomes a command when the document draws next.
*/
void
MaterialDocument::DrawTextures(const MaterialTemplatesGPULang::Entry* materialTemplate)
{
    const float thumbnailSize = ImGui::GetFrameHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y;
    for (IndexT i = 0; i < materialTemplate->textures.Size() && i < this->slots.Size(); i++)
    {
        const char* name = materialTemplate->textures.KeyAtIndex(i);
        const MaterialTemplatesGPULang::MaterialTemplateTexture* value = materialTemplate->textures.ValueAtIndex(i);
        Slot& slot = this->slots[i];
        ImGui::PushID(i);

        // the thumbnail is what the material has bound, which is the template's default until a texture is set
        const Resources::ResourceId bound = Materials::MaterialGetTexture(this->material, value->textureIndex);
        if (slot.hasImageId && bound != Resources::InvalidResourceId)
        {
            slot.image.nebulaHandle = bound.resourceId;
            Dynui::SetImguiTextureIdData(slot.imageId, slot.image);
            ImGui::Image(ImTextureRef(slot.imageId), ImVec2(thumbnailSize, thumbnailSize));
            if (ImGui::BeginItemTooltip())
            {
                ImGui::Image(ImTextureRef(slot.imageId), ImVec2(256.0f, 256.0f));
                ImGui::EndTooltip();
            }
        }
        else
        {
            ImGui::Dummy(ImVec2(thumbnailSize, thumbnailSize));
        }

        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextUnformatted(name);
        if (value->desc != nullptr)
            ImGui::SetItemTooltip("%s", value->desc);
        if (slot.texture.IsEmpty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(template default)");
        }

        Ptr<MaterialDocument> self = this;
        const IndexT slotIndex = i;
        Editor::AssetField("texture", slot.texture, "tex", [self, slotIndex](const IO::Path& texture)
        {
            if (self->IsClosed())
                return;
            self->pendingSlot = slotIndex;
            self->pendingTexture = texture;
        });
        ImGui::EndGroup();
        ImGui::PopID();
    }
}

//------------------------------------------------------------------------------
/**
    A value is shown on the material while it is dragged, and becomes a command when the drag ends,
    undone to what it was before the drag.
*/
void
MaterialDocument::DrawValues(const MaterialTemplatesGPULang::Entry* materialTemplate)
{
    ubyte* live = Materials::MaterialGetConstants(this->material);
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        const char* name = materialTemplate->values.KeyAtIndex(i);
        const MaterialTemplatesGPULang::MaterialTemplateValue* value = materialTemplate->values.ValueAtIndex(i);
        const uint size = (uint)value->GetSize();
        if (value->offset + size > materialTemplate->bufferSize)
            continue;

        ubyte edit[16] = { 0 };
        Memory::Copy(live + value->offset, edit, size);

        switch (value->type)
        {
            case MaterialTemplatesGPULang::MaterialTemplateValue::Bool:
                ImGui::Checkbox(name, (bool*)edit);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Scalar:
                ImGui::DragFloat(name, (float*)edit, 0.01f, 0.0f, FLT_MAX);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec2:
                ImGui::DragFloat2(name, (float*)edit, 0.01f, 0.0f, FLT_MAX);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec3:
                ImGui::DragFloat3(name, (float*)edit, 0.01f, 0.0f, FLT_MAX);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Vec4:
                ImGui::DragFloat4(name, (float*)edit, 0.01f, 0.0f, FLT_MAX);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Color:
                ImGui::ColorEdit4(name, (float*)edit, ImGuiColorEditFlags_Float);
                break;
            case MaterialTemplatesGPULang::MaterialTemplateValue::Int:
                // the material loader does not read integers back, so a change would not last
                ImGui::BeginDisabled();
                ImGui::DragInt(name, (int*)edit);
                ImGui::EndDisabled();
                ImGui::SetItemTooltip("Integer values aren't loaded from the material file yet, so they can't be changed here");
                continue;
            default:
                continue;
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && value->desc != nullptr)
            ImGui::SetTooltip("%s", value->desc);

        // shown while it is dragged, the command is made once the drag ends
        if (ImGui::IsItemEdited())
        {
            Util::Array<ubyte> constants;
            constants.Resize(materialTemplate->bufferSize);
            Memory::Copy(live, constants.Begin(), materialTemplate->bufferSize);
            Memory::Copy(edit, constants.Begin() + value->offset, size);
            Materials::MaterialSetConstants(this->material, constants.Begin(), materialTemplate->bufferSize);
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && memcmp(edit, this->committed.Begin() + value->offset, size) != 0)
        {
            Ptr<SetMaterialValueCommand> command = SetMaterialValueCommand::Create();
            command->document = this;
            command->offset = value->offset;
            command->size = size;
            Memory::Copy(this->committed.Begin() + value->offset, command->oldBytes, size);
            Memory::Copy(edit, command->newBytes, size);
            this->commands->Execute(command.upcast<Editor::EditCommand>());
        }
    }
}

//------------------------------------------------------------------------------
/**
    The template's parameters are written from the material as it is now, textures by URN. What the
    work file had that the template does not know is kept as it was.
*/
bool
MaterialDocument::OnSave(Util::String& outError)
{
    const MaterialTemplatesGPULang::Entry* materialTemplate = Materials::MaterialGetTemplate(this->material);
    const ubyte* live = Materials::MaterialGetConstants(this->material);

    ToolkitUtil::MaterialResourceT work;
    work.template_name = materialTemplate->name;
    Util::Dictionary<Util::String, Util::String> written;

    for (IndexT i = 0; i < materialTemplate->textures.Size() && i < this->slots.Size(); i++)
    {
        const Util::String name = materialTemplate->textures.KeyAtIndex(i);
        if (this->slots[i].texture.IsFile())
            written.Emplace(name) = this->slots[i].texture.AsString();
        else if (this->workValues.Contains(name))
            written.Emplace(name) = this->workValues[name];
    }
    for (IndexT i = 0; i < materialTemplate->values.Size(); i++)
    {
        const MaterialTemplatesGPULang::MaterialTemplateValue* value = materialTemplate->values.ValueAtIndex(i);
        const Util::String text = ValueString(value, live + value->offset);
        if (!text.IsEmpty())
            written.Emplace(materialTemplate->values.KeyAtIndex(i)) = text;
    }
    for (IndexT i = 0; i < this->workValues.Size(); i++)
    {
        if (!written.Contains(this->workValues.KeyAtIndex(i)))
            written.Emplace(this->workValues.KeyAtIndex(i)) = this->workValues.ValueAtIndex(i);
    }

    for (IndexT i = 0; i < written.Size(); i++)
    {
        work.value_names.push_back(written.KeyAtIndex(i));
        work.values.push_back(written.ValueAtIndex(i));
    }

    if (!WriteWorkFile(this->asset.WorkURI("assets"), work, outError))
        return false;

    this->workTemplate = work.template_name;
    this->workValues = written;
    return true;
}

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
