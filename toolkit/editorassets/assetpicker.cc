//------------------------------------------------------------------------------
//  @file assetpicker.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetpicker.h"
#include "editorassets/assetbrowser.h"
#include "neweditor/api/editorfeature.h"
#include "neweditor/api/shell.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::AssetPicker, 'ASPK', Core::RefCounted);
__ImplementInterfaceSingleton(Editor::AssetPicker);

// The popup's id stays the same while its title says what is picked
static const char* const PickFileTitle = "Pick an asset###assetpicker";
static const char* const PickFolderTitle = "Pick a folder###assetpicker";

//------------------------------------------------------------------------------
/**
*/
AssetPicker::AssetPicker() :
    openRequested(false),
    pickingFolder(false)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
AssetPicker::~AssetPicker()
{
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
    A request made while another is open replaces it, the first one never gets an answer.
*/
void
AssetPicker::PickFile(const IO::Path& start, const Util::String& type, const PickCallback& onPick)
{
    if (!this->browser.isvalid())
    {
        this->browser = AssetBrowser::Create();
        this->browser->Setup(EditorFeatureRegistry::GetShell());
    }
    this->browser->StartPickFile(start, type);
    this->onPick = onPick;
    this->pickingFolder = false;
    this->openRequested = true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPicker::PickFolder(const IO::Path& start, const PickCallback& onPick)
{
    if (!this->browser.isvalid())
    {
        this->browser = AssetBrowser::Create();
        this->browser->Setup(EditorFeatureRegistry::GetShell());
    }
    this->browser->StartPickFolder(start);
    this->onPick = onPick;
    this->pickingFolder = true;
    this->openRequested = true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPicker::Draw()
{
    if (!this->browser.isvalid())
        return;

    const char* title = this->pickingFolder ? PickFolderTitle : PickFileTitle;
    if (this->openRequested)
    {
        ImGui::OpenPopup(title);
        this->openRequested = false;
    }

    ImGui::SetNextWindowSize(ImVec2(900.0f, 560.0f), ImGuiCond_Appearing);
    bool keepOpen = true;
    if (ImGui::BeginPopupModal(title, &keepOpen))
    {
        this->browser->Draw();

        IO::Path path;
        if (this->browser->ConsumePick(path))
        {
            this->picked = path;
            Shell* shell = EditorFeatureRegistry::GetShell();
            if (shell != nullptr)
                shell->QueueAction(Util::Delegate<void()>::FromMethod<AssetPicker, &AssetPicker::RunPicked>(this));
            ImGui::CloseCurrentPopup();
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !ImGui::IsAnyItemActive())
        {
            // a name being edited takes Escape for itself
            keepOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (!keepOpen)
        this->Cancel();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPicker::RunPicked()
{
    // taken off first, the callback may ask for another pick
    PickCallback callback = this->onPick;
    this->onPick = nullptr;
    this->browser->StopPicking();
    if (callback)
        callback(this->picked);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetPicker::Cancel()
{
    this->onPick = nullptr;
    this->browser->StopPicking();
}

} // namespace Editor
