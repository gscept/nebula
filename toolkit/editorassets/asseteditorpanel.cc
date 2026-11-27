//------------------------------------------------------------------------------
//  @file asseteditorpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/asseteditorpanel.h"
#include "editorassets/assetdocument.h"
#include "editorassets/assettype.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/widgets/confirmmodal.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::AssetEditorPanel, 'ASEP', Editor::EditorPanel);

AssetEditorPanel* AssetEditorPanel::instance = nullptr;

//------------------------------------------------------------------------------
/**
    The icon of an asset's type, empty for a type without one
*/
static const char*
TypeGlyph(const IO::Path& asset)
{
    const AssetType* type = AssetTypeRegistry::Find(asset);
    const char* glyph = type != nullptr ? IconGlyph(type->icon.AsCharPtr()) : nullptr;
    return glyph != nullptr ? glyph : "";
}

//------------------------------------------------------------------------------
/**
*/
AssetEditorPanel::AssetEditorPanel() :
    active(nullptr),
    selectRequest(nullptr)
{
    this->name = "asset editor";
    AssetEditorPanel::instance = this;
}

//------------------------------------------------------------------------------
/**
    The editor closes before the documents are gone, so they are closed here and let go of what they
    loaded.
*/
AssetEditorPanel::~AssetEditorPanel()
{
    for (IndexT i = 0; i < this->documents.Size(); i++)
        this->documents[i]->Close();
    this->documents.Clear();
    this->pendingOpens.Clear();
    if (AssetEditorPanel::instance == this)
        AssetEditorPanel::instance = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetEditorPanel::OnSetup()
{
    this->core->GetCommands().Register("asset", "asset [list|open <urn>|close [urn]|save|reimport <urn>] - the assets open in the asset editor, and importing one again from its source. A URN is folder:file:type",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<AssetEditorPanel, &AssetEditorPanel::CommandAsset>(this));

    // the exit asks about changed assets along with the scenes
    EditorFeatureRegistry::RegisterUnsavedWork(
        Util::Delegate<void(Util::Array<Util::String>&)>::FromMethod<AssetEditorPanel, &AssetEditorPanel::ListUnsaved>(this),
        Util::Delegate<bool()>::FromMethod<AssetEditorPanel, &AssetEditorPanel::SaveAll>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::ListUnsaved(Util::Array<Util::String>& out)
{
    for (IndexT i = 0; i < this->documents.Size(); i++)
    {
        if (this->documents[i]->IsDirty())
            out.Append(this->documents[i]->GetAsset().GetFolderAndFile());
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetEditorPanel::SaveAll()
{
    bool saved = true;
    for (IndexT i = 0; i < this->documents.Size(); i++)
    {
        if (this->documents[i]->IsDirty() && !this->documents[i]->Save())
            saved = false;
    }
    return saved;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetEditorPanel::BeginWindow()
{
    const bool visible = EditorPanel::BeginWindow();
    if (!visible)
        this->SetActive(nullptr);
    return visible;
}

//------------------------------------------------------------------------------
/**
    Opening goes through the type, the way a double click in the browser does. Closing asks about
    changes the way the tab's close button does.
*/
void
AssetEditorPanel::CommandAsset(const Util::Array<Util::String>& args)
{
    if (args.IsEmpty() || args[0] == "list")
    {
        if (this->documents.IsEmpty())
            this->core->Log(LogMessageType::N_SYSTEM, "No asset is open");
        for (IndexT i = 0; i < this->documents.Size(); i++)
        {
            const AssetDocument* document = this->documents[i];
            this->core->Log(LogMessageType::N_SYSTEM, Util::Format("%s%s%s%s",
                document == this->active ? "* " : "  ",
                document->GetAsset().AsString().AsCharPtr(),
                document->IsDirty() ? " (changed)" : "",
                document->IsReady() ? "" : " (loading)"));
        }
        return;
    }

    const Util::String& verb = args[0];
    if (verb == "open")
    {
        if (args.Size() < 2)
        {
            this->core->Log(LogMessageType::N_WARNING, "asset open <urn> - the asset to open, as folder:file:type");
            return;
        }
        const IO::Path asset = IO::Path::Parse(args[1]);
        const AssetType* type = AssetTypeRegistry::Find(asset);
        if (type == nullptr)
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("'%s' is not an asset of a known type", args[1].AsCharPtr()));
            return;
        }
        if (!type->open.IsValid())
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("No editor opens %s assets yet", type->name.AsCharPtr()));
            return;
        }
        type->open(asset);
        return;
    }

    if (verb == "close")
    {
        AssetDocument* document = args.Size() > 1 ? AssetEditorPanel::Find(IO::Path::Parse(args[1])) : this->active;
        if (document == nullptr)
        {
            this->core->Log(LogMessageType::N_WARNING, "That asset is not open");
            return;
        }
        this->RequestClose(document);
        return;
    }

    if (verb == "reimport")
    {
        if (args.Size() < 2)
        {
            this->core->Log(LogMessageType::N_WARNING, "asset reimport <urn> - the asset to import again, as folder:file:type");
            return;
        }
        const IO::Path asset = IO::Path::Parse(args[1]);
        const AssetType* type = AssetTypeRegistry::Find(asset);
        if (type == nullptr || !type->reimport.IsValid())
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("'%s' isn't imported from a source file", args[1].AsCharPtr()));
            return;
        }
        Util::String error;
        if (!type->reimport(asset, error))
            this->core->Log(LogMessageType::N_WARNING, Util::Format("Could not reimport '%s': %s", args[1].AsCharPtr(), error.AsCharPtr()));
        return;
    }

    if (verb == "save")
    {
        if (this->active == nullptr)
            this->core->Log(LogMessageType::N_WARNING, "No asset is open");
        else
            this->active->Save();
        return;
    }

    this->core->Log(LogMessageType::N_WARNING, Util::Format("asset: unknown '%s', use list, open, close, save or reimport", verb.AsCharPtr()));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::Open(const Ptr<AssetDocument>& document)
{
    AssetEditorPanel* panel = AssetEditorPanel::instance;
    if (panel == nullptr || !document.isvalid())
        return;

    panel->pendingOpens.Append(document);
    panel->SetOpen(true);
    panel->Focus();
}

//------------------------------------------------------------------------------
/**
*/
AssetDocument*
AssetEditorPanel::Find(const IO::Path& asset)
{
    AssetEditorPanel* panel = AssetEditorPanel::instance;
    if (panel == nullptr)
        return nullptr;
    for (IndexT i = 0; i < panel->documents.Size(); i++)
    {
        if (panel->documents[i]->GetAsset() == asset)
            return panel->documents[i];
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
    The editor may still be open when a subsystem stops, the exit stops the features in the order
    they started, so the subsystem closes its documents itself.
*/
void
AssetEditorPanel::CloseDocumentsOf(const Util::String& type)
{
    AssetEditorPanel* panel = AssetEditorPanel::instance;
    if (panel == nullptr)
        return;

    for (IndexT i = panel->pendingOpens.Size() - 1; i >= 0; i--)
    {
        if (panel->pendingOpens[i]->GetAsset().GetType() == type)
            panel->pendingOpens.EraseIndex(i);
    }
    for (IndexT i = panel->documents.Size() - 1; i >= 0; i--)
    {
        if (panel->documents[i]->GetAsset().GetType() == type)
            panel->CloseDocument(panel->documents[i]);
    }
    if (panel->closeAsked.isvalid() && panel->closeAsked->GetAsset().GetType() == type)
        panel->closeAsked = nullptr;
}

//------------------------------------------------------------------------------
/**
    An asset already open only gets its tab brought to the front, its document is not opened again.
*/
void
AssetEditorPanel::OpenPending()
{
    if (this->pendingOpens.IsEmpty())
        return;

    const Util::Array<Ptr<AssetDocument>> opens = this->pendingOpens;
    this->pendingOpens.Clear();
    for (const Ptr<AssetDocument>& document : opens)
    {
        AssetDocument* open = AssetEditorPanel::Find(document->GetAsset());
        if (open != nullptr)
        {
            this->selectRequest = open;
            continue;
        }

        if (!document->Open(document->GetAsset(), this->core))
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("Can't open '%s' in the asset editor", document->GetAsset().GetFolderAndFile().AsCharPtr()));
            document->Close();
            continue;
        }
        this->documents.Append(document);
        this->selectRequest = document;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::SetActive(AssetDocument* document)
{
    if (this->active == document)
        return;
    if (this->active != nullptr)
        this->active->OnShown(false);
    this->active = document;
    if (this->active != nullptr)
        this->active->OnShown(true);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::Draw()
{
    this->OpenPending();

    // a load that failed takes its tab with it, and says why
    for (IndexT i = 0; i < this->documents.Size(); i++)
    {
        AssetDocument* document = this->documents[i];
        if (document->HasFailed())
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("Could not load '%s': %s", document->GetAsset().GetFolderAndFile().AsCharPtr(), document->GetFailReason().AsCharPtr()));
            this->CloseDocument(document);
            i--;
        }
    }

    if (this->documents.IsEmpty())
    {
        this->SetActive(nullptr);
        ImGui::TextDisabled("Double click an asset in the asset browser to open it here");
        return;
    }

    // the editor style hides the selected tab's close button, but asset tabs have to be closable whichever is selected.
    // The style is read when the tabs are drawn, so it only applies to this tab bar
    ImGuiStyle& style = ImGui::GetStyle();
    const float closeButtonMinWidth = style.TabCloseButtonMinWidthSelected;
    style.TabCloseButtonMinWidthSelected = -1.0f;

    AssetDocument* shown = nullptr;
    Util::Array<AssetDocument*> closeRequests;
    if (ImGui::BeginTabBar("assettabs", ImGuiTabBarFlags_Reorderable))
    {
        for (IndexT i = 0; i < this->documents.Size(); i++)
        {
            AssetDocument* document = this->documents[i];
            const IO::Path& asset = document->GetAsset();

            // the id is the URN, so the tab keeps its place while the label changes
            const Util::String label = Util::Format("%s %s###%s", TypeGlyph(asset), asset.GetFolderAndFile().AsCharPtr(), asset.AsString().AsCharPtr());

            ImGuiTabItemFlags flags = ImGuiTabItemFlags_None;
            if (document->IsDirty())
                flags |= ImGuiTabItemFlags_UnsavedDocument;
            if (this->selectRequest == document)
            {
                flags |= ImGuiTabItemFlags_SetSelected;
                this->selectRequest = nullptr;
            }

            bool open = true;
            if (ImGui::BeginTabItem(label.AsCharPtr(), &open, flags))
            {
                shown = document;
                this->SetActive(document);

                // saves only this asset, Ctrl+S does the same while the asset editor is the panel last worked in
                const bool canSave = document->CanSave();
                ImGui::BeginDisabled(!canSave || !document->IsDirty());
                if (IconButton(IconGlyph("save"), "Save"))
                    document->Save();
                ImGui::EndDisabled();
                if (!canSave && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                    ImGui::SetTooltip("This asset is only shown here, it is changed by importing it again");
                ImGui::SameLine();
                ImGui::TextDisabled("%s", asset.AsString().AsCharPtr());
                ImGui::Separator();

                if (document->IsReady())
                    document->Draw();
                else
                    ImGui::TextDisabled("Loading %s...", asset.GetFolderAndFile().AsCharPtr());

                ImGui::EndTabItem();
            }

            // the close button works on tabs that aren't in front as well
            if (!open)
                closeRequests.Append(document);
        }
        ImGui::EndTabBar();
    }
    style.TabCloseButtonMinWidthSelected = closeButtonMinWidth;

    if (shown == nullptr)
        this->SetActive(nullptr);

    // closed after the tabs, as closing changes the list they are drawn from
    for (AssetDocument* document : closeRequests)
        this->RequestClose(document);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::RequestClose(AssetDocument* document)
{
    if (!document->IsDirty())
    {
        this->CloseDocument(document);
        return;
    }

    // one question at a time, a second close while it is open waits for the next click
    if (this->closeAsked.isvalid())
        return;

    this->closeAsked = document;
    this->selectRequest = document;
    this->core->GetConfirmModal()->Show(
        "Unsaved changes",
        Util::Format("'%s' has changes that are not saved.", document->GetAsset().GetFolderAndFile().AsCharPtr()),
        "Save",
        "Discard",
        Util::Delegate<void()>::FromMethod<AssetEditorPanel, &AssetEditorPanel::SaveAndClose>(this),
        Util::Delegate<void()>::FromMethod<AssetEditorPanel, &AssetEditorPanel::DiscardAndClose>(this),
        Util::Delegate<void()>::FromMethod<AssetEditorPanel, &AssetEditorPanel::KeepOpen>(this));
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::CloseDocument(AssetDocument* document)
{
    IndexT index = InvalidIndex;
    for (IndexT i = 0; i < this->documents.Size(); i++)
    {
        if (this->documents[i].get() == document)
        {
            index = i;
            break;
        }
    }
    if (index == InvalidIndex)
        return;

    if (this->active == document)
        this->SetActive(nullptr);
    if (this->selectRequest == document)
        this->selectRequest = nullptr;

    // held until the document is closed, the list entry is the last reference
    Ptr<AssetDocument> closing = this->documents[index];
    this->documents.EraseIndex(index);
    closing->Close();
}

//------------------------------------------------------------------------------
/**
    A save that fails keeps the changes and the tab, the log says why
*/
void
AssetEditorPanel::SaveAndClose()
{
    Ptr<AssetDocument> document = this->closeAsked;
    this->closeAsked = nullptr;
    if (document.isvalid() && document->Save())
        this->CloseDocument(document);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::DiscardAndClose()
{
    Ptr<AssetDocument> document = this->closeAsked;
    this->closeAsked = nullptr;
    if (document.isvalid())
    {
        document->Revert();
        this->CloseDocument(document);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditorPanel::KeepOpen()
{
    this->closeAsked = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommandManager>
AssetEditorPanel::GetCommandManager() const
{
    return this->active != nullptr ? this->active->GetCommandManager() : nullptr;
}

//------------------------------------------------------------------------------
/**
    The keys the tab in front has no answer for are taken all the same, an asset is what is being
    looked at, not the scene.
*/
bool
AssetEditorPanel::OnEditAction(EditAction action)
{
    if (this->active == nullptr)
        return false;

    if (action == EditAction::Save)
    {
        if (!this->active->CanSave())
            this->SetStatus("This asset is only shown here, there is nothing to save", 3.0f);
        else if (!this->active->IsDirty())
            this->SetStatus("No changes to save", 3.0f);
        else if (this->active->Save())
            this->SetStatus(Util::Format("Saved %s", this->active->GetAsset().GetFolderAndFile().AsCharPtr()), 3.0f);
        return true;
    }

    this->active->OnEditAction(action);
    return true;
}

} // namespace Editor
