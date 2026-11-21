//------------------------------------------------------------------------------
//  @file filebrowsermodal.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/filebrowsermodal.h"
#include "neweditor/api/shell.h"
#include "neweditor/widgets/filebrowser.h"
#include "io/ioserver.h"
#include "io/uri.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::FileBrowserModal, 'FBMO', Core::RefCounted);

// The popup size and the room the button row takes. The listing is the child in between
static const float DialogWidth = 620.0f;
static const float DialogHeight = 440.0f;
static const float ButtonsWidth = 170.0f;

//------------------------------------------------------------------------------
/**
*/
FileBrowserModal::FileBrowserModal() :
    core(nullptr),
    mode(ModeSave),
    visible(false),
    openRequested(false),
    closeRequested(false),
    nameFocusRequested(false),
    confirmOverwrite(false),
    acceptedFired(false)
{
    this->nameBuffer[0] = 0;
}

//------------------------------------------------------------------------------
/**
*/
FileBrowserModal::~FileBrowserModal()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The dialog is built with the core, which is what draws it every frame. It is never visible until
    asked.
*/
bool
FileBrowserModal::Setup(Shell* core)
{
    this->core = core;
    this->browser = FileBrowser::Create();
    this->browser->Setup(core);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserModal::Discard()
{
    this->browser = nullptr;
    this->onAccepted = Util::Delegate<void(const Util::String&)>();
    this->onCanceled = Util::Delegate<void()>();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserModal::ShowSave(const Util::String& title, const Util::String& startDir, const Util::String& defaultName,
    const Util::String& filter, Util::Delegate<void(const Util::String&)> onAccepted,
    Util::Delegate<void()> onCanceled)
{
    this->mode = ModeSave;
    this->title = title;
    this->filter = filter;
    this->extension = ExtensionOfFilter(filter);
    this->onAccepted = onAccepted;
    this->onCanceled = onCanceled;
    this->acceptedFired = false;

    defaultName.CopyToBuffer(this->nameBuffer, MaxNameLength);
    this->nameFocusRequested = true;
    this->mirroredSelection.Clear();

    this->confirmOverwrite = false;
    this->confirmDir.Clear();

    this->browser->SetFilter(filter);
    this->browser->Navigate(startDir);

    this->visible = true;
    this->openRequested = true;
    this->closeRequested = false;
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserModal::ShowOpen(const Util::String& title, const Util::String& startDir,
    const Util::String& filter, Util::Delegate<void(const Util::String&)> onAccepted)
{
    this->mode = ModeOpen;
    this->title = title;
    this->filter = filter;
    this->extension = ExtensionOfFilter(filter);
    this->onAccepted = onAccepted;
    this->onCanceled = Util::Delegate<void()>();
    this->acceptedFired = false;

    this->nameBuffer[0] = 0;
    this->nameFocusRequested = false;
    this->mirroredSelection.Clear();

    this->confirmOverwrite = false;
    this->confirmDir.Clear();

    this->browser->SetFilter(filter);
    this->browser->Navigate(startDir);

    this->visible = true;
    this->openRequested = true;
    this->closeRequested = false;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
FileBrowserModal::ExtensionOfFilter(const Util::String& filter)
{
    const IndexT dot = filter.FindCharIndex('.');
    if (dot == InvalidIndex)
        return "";
    return filter.ExtractRange(dot, filter.Length() - dot);
}

//------------------------------------------------------------------------------
/**
    The typed name gets the filter's extension when it carries none, so 'foo' under '*.json' saves
    as 'foo.json' rather than a file without a type.
*/
Util::String
FileBrowserModal::TargetPath() const
{
    Util::String name = this->nameBuffer;
    if (this->mode == ModeSave && !this->extension.IsEmpty() && name.FindCharIndex('.') == InvalidIndex)
        name.Append(this->extension);
    return this->browser->ChildPath(name);
}

//------------------------------------------------------------------------------
/**
    The popup centers on the viewport and takes its size on appearing, so a dialog the user resized
    keeps that shape until the layout is reset. The listing is the child, and the button row stays
    at the bottom.
*/
void
FileBrowserModal::Draw()
{
    if (!this->visible)
        return;

    // A folder change drops an overwrite ask that belongs to the folder it was raised in
    if (this->confirmOverwrite && this->confirmDir != this->browser->GetDir())
        this->confirmOverwrite = false;

    if (this->openRequested)
    {
        // The popup is named by the title, so the open and the begin have to say the same thing
        ImGui::OpenPopup(this->title.AsCharPtr());
        this->openRequested = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(DialogWidth, DialogHeight), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(this->title.AsCharPtr(), &open))
    {
        this->browser->DrawAddressBar();

        const float controls = ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("##listing", ImVec2(0, -controls));
        this->browser->DrawEntryTable();
        ImGui::EndChild();

        // What a double click on a file does is the dialog's business: open mode takes the file,
        // save mode
        // puts its name in the field as the overwrite target
        const Util::String opened = this->browser->ConsumeFileOpen();
        if (!opened.IsEmpty())
        {
            if (this->mode == ModeOpen)
                this->Accept(this->browser->ChildPath(opened));
            else
                opened.CopyToBuffer(this->nameBuffer, MaxNameLength);
        }

        // Open mode mirrors the selection into the field, but only when the selection moved, so a
        // name
        // typed by hand is not overwritten
        if (this->mode == ModeOpen)
        {
            const Util::String& selection = this->browser->GetSelectedName();
            if (selection != this->mirroredSelection)
            {
                this->mirroredSelection = selection;
                selection.CopyToBuffer(this->nameBuffer, MaxNameLength);
            }
        }

        // The button row. While the overwrite ask is up, the row is the question instead of the
        // controls
        if (this->confirmOverwrite)
        {
            ImGui::Text("Overwrite %s?", Util::String(this->nameBuffer).AsCharPtr());
            ImGui::SameLine();
            if (ImGui::Button("Yes"))
                this->Accept(this->TargetPath());
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                this->confirmOverwrite = false;
                this->confirmDir.Clear();
            }
        }
        else
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Name:");
            ImGui::SameLine();

            if (this->nameFocusRequested)
            {
                ImGui::SetKeyboardFocusHere();
                this->nameFocusRequested = false;
            }

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ButtonsWidth);
            const bool committed = ImGui::InputText("##name", this->nameBuffer, MaxNameLength, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();

            // The field is what tells the ask to stand down: a different name is a different target
            if (ImGui::IsItemEdited())
                this->confirmOverwrite = false;

            ImGui::SameLine();
            if (ImGui::Button(this->mode == ModeSave ? "Save" : "Open") || committed)
            {
                const Util::String name = this->nameBuffer;
                if (name.IsEmpty())
                    this->core->SetStatus("The name field is empty", 3.0f);
                else
                {
                    const Util::String path = this->TargetPath();

                    // The ask is raised rather than answered here, so the same press never both
                    // asks and
                    // writes. Whether the file exists is asked of the filesystem rather than of the
                    // listing,
                    // because a save may be pointed at a file the filter does not show
                    if (this->mode == ModeSave && IO::FileExists(IO::URI(path)))
                    {
                        this->confirmOverwrite = true;
                        this->confirmDir = this->browser->GetDir();
                    }
                    else
                    {
                        this->Accept(path);
                    }
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
                this->closeRequested = true;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            this->closeRequested = true;

        if (this->closeRequested)
        {
            this->CancelUnlessAccepted();
            ImGui::CloseCurrentPopup();
            this->closeRequested = false;
            this->visible = false;
        }

        ImGui::EndPopup();
    }

    // ImGui itself closing the popup, Escape through the nav, lands here too
    if (!open)
    {
        this->CancelUnlessAccepted();
        this->visible = false;
    }
}

//------------------------------------------------------------------------------
/**
    The callback does not run here, it is queued: what a dialog accepts changes the model, and the
    core runs model changes after the frame that drew them.
*/
void
FileBrowserModal::Accept(const Util::String& path)
{
    this->acceptedPath = path;
    this->acceptedFired = true;
    this->closeRequested = true;
    this->core->QueueAction(Util::Delegate<void()>::FromMethod<FileBrowserModal, &FileBrowserModal::RunAccepted>(this));
}

//------------------------------------------------------------------------------
/**
    A dialog that closed without an answer tells the caller who was waiting on one: a save that was
    asked for and not given is a no, and the caller's work stays where it was.
*/
void
FileBrowserModal::CancelUnlessAccepted()
{
    if (this->acceptedFired || !this->onCanceled.IsValid() || this->core == nullptr)
        return;
    this->core->QueueAction(Util::Delegate<void()>::FromMethod<FileBrowserModal, &FileBrowserModal::RunCanceled>(this));
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserModal::RunCanceled()
{
    // Both popup close paths can queue in the same frame, so the callback is taken out and cleared
    // before it runs: the dialog is answered, or refused, exactly once
    Util::Delegate<void()> action = this->onCanceled;
    this->onCanceled = Util::Delegate<void()>();
    if (action.IsValid())
        action();
}

//------------------------------------------------------------------------------
/**
*/
void
FileBrowserModal::RunAccepted()
{
    // The callback is taken out and cleared before it runs: an accepted answer can open a fresh
    // dialog
    // (the quit walk saving several untitled scenes does exactly this), and clearing afterward
    // would
    // erase that dialog's callback before it ever drew
    Util::Delegate<void(const Util::String&)> action = this->onAccepted;
    const Util::String path = this->acceptedPath;
    this->onAccepted = Util::Delegate<void(const Util::String&)>();
    if (action.IsValid())
        action(path);
}

} // namespace Editor
