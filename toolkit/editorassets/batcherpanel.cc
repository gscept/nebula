//------------------------------------------------------------------------------
//  @file batcherpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/batcherpanel.h"
#include "editorassets/livebatcher.h"
#include "neweditor/base/drawsurface.h"
#include "neweditor/base/iconregistry.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::BatcherPanel, 'BATP', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
    The panel starts closed. The status bar says when jobs run, and the Window menu opens the log.
*/
BatcherPanel::BatcherPanel() :
    logVersion(~0u),
    autoScroll(true)
{
    this->name = "batcher";
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
BatcherPanel::~BatcherPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
BatcherPanel::Draw()
{
    const uint version = LiveBatcher::GetLogVersion();
    if (version != this->logVersion)
    {
        this->log = LiveBatcher::GetLog();
        this->logVersion = version;
    }

    if (IconButton(IconGlyph("trash-2"), "Clear the log"))
        LiveBatcher::ClearLog();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &this->autoScroll);
    if (LiveBatcher::IsBusy())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Running the batcher...");
    }
    ImGui::Separator();

    if (this->log.IsEmpty())
    {
        ImGui::TextDisabled("Nothing batched yet");
        return;
    }

    if (ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {
        const char* text = this->log.AsCharPtr();
        ImGui::TextUnformatted(text, text + this->log.Length());

        if (this->autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

} // namespace Editor
