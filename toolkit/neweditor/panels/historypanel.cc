//------------------------------------------------------------------------------
//  @file historypanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/panels/historypanel.h"
#include "neweditor/api/shell.h"
#include "neweditor/console/editorcommands.h"
#include "neweditor/model/editcommandmanager.h"
#include "neweditor/model/scene.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::HistoryPanel, 'HSTP', Editor::EditorPanel);

//------------------------------------------------------------------------------
/**
    The panel starts closed, the panels menu opens it
*/
HistoryPanel::HistoryPanel() :
    lastCount(0),
    pendingDone(0)
{
    this->name = "history";
    this->open = false;
}

//------------------------------------------------------------------------------
/**
*/
HistoryPanel::~HistoryPanel()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
HistoryPanel::OnSetup()
{
    this->core->GetCommands().Register("history", "history [list|goto <n>] - print the undo history, or undo and redo until n entries are done",
        Util::Delegate<void(const Util::Array<Util::String>&)>::FromMethod<HistoryPanel, &HistoryPanel::CommandHistory>(this));
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommandManager>
HistoryPanel::GetCommandManager() const
{
    return this->shown;
}

//------------------------------------------------------------------------------
/**
    A command that fails to undo stays where it is, the loop stops there rather than going around
*/
bool
HistoryPanel::MoveTo(const Ptr<EditCommandManager>& manager, SizeT done)
{
    if (!manager.isvalid())
        return false;
    while ((SizeT)manager->GetUndoList().Size() > done)
    {
        if (!manager->Undo())
            return false;
    }
    while ((SizeT)manager->GetUndoList().Size() < done)
    {
        if (!manager->Redo())
            return false;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
HistoryPanel::DescribeOwner() const
{
    if (!this->shown.isvalid())
        return "Nothing";
    if (this->shown == this->core->GetCommandManager())
    {
        const Ptr<Scene> scene = this->core->GetScene();
        return Util::Format("Scene %s", scene.isvalid() ? scene->GetName().AsCharPtr() : "");
    }
    return "The asset editor";
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryPanel::Draw()
{
    // follow the history undo runs against. While this panel is the one last worked in, the target
    // is what it returns, which keeps it on the history it showed
    this->shown = this->core->GetTargetCommandManager();
    if (!this->shown.isvalid())
    {
        ImGui::TextDisabled("No history");
        return;
    }

    const EditCommandManager::CommandList& undoList = this->shown->GetUndoList();
    const EditCommandManager::CommandList& redoList = this->shown->GetRedoList();
    const SizeT undoCount = undoList.Size();
    const SizeT count = undoCount + redoList.Size();

    ImGui::TextDisabled("%s", this->DescribeOwner().AsCharPtr());
    ImGui::SameLine();
    ImGui::BeginDisabled(undoCount == 0);
    if (ImGui::SmallButton("Undo"))
        this->QueueMove(this->shown, undoCount - 1);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(redoList.IsEmpty());
    if (ImGui::SmallButton("Redo"))
        this->QueueMove(this->shown, undoCount + 1);
    ImGui::EndDisabled();
    ImGui::Separator();

    // the entry to move to is picked while drawing and the move runs after the frame, the commands
    // change the scene the other panels are drawing
    IndexT clicked = InvalidIndex;
    if (ImGui::BeginChild("##entries"))
    {
        if (ImGui::Selectable("(start)", undoCount == 0))
            clicked = 0;

        // the done entries oldest first, then the undone ones in the order redo does them, which is
        // the redo list backwards
        for (IndexT row = 0; row < count; row++)
        {
            const bool done = row < undoCount;
            const EditCommandManager::Entry& entry = done ? undoList[row] : redoList[redoList.Size() - 1 - (row - undoCount)];
            const bool current = row == undoCount - 1;

            ImGui::PushID(row);
            if (!done)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

            const Util::String label = Util::Format("%d  %s", row + 1, entry.name.AsCharPtr());
            if (entry.commands.Size() > 1)
            {
                // a macro opens to the commands it ran
                const bool opened = ImGui::TreeNodeEx("##macro", ImGuiTreeNodeFlags_SpanAvailWidth | (current ? ImGuiTreeNodeFlags_Selected : 0), "%s (%d)", label.AsCharPtr(), entry.commands.Size());
                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
                    clicked = row + 1;
                if (opened)
                {
                    for (IndexT i = 0; i < entry.commands.Size(); i++)
                        ImGui::BulletText("%s", entry.commands[i]->Name());
                    ImGui::TreePop();
                }
            }
            else if (ImGui::Selectable(label.AsCharPtr(), current))
                clicked = row + 1;

            if (!done)
                ImGui::PopStyleColor();
            ImGui::PopID();
        }

        if (this->lastCount != count)
            ImGui::SetScrollHereY(1.0f);
        this->lastCount = count;
    }
    ImGui::EndChild();

    if (clicked != InvalidIndex && clicked != undoCount)
        this->QueueMove(this->shown, clicked);
}

//------------------------------------------------------------------------------
/**
    The move runs after the frame, the commands change the scene the other panels are drawing. One
    move is pending at a time, a second click in the same frame replaces it
*/
void
HistoryPanel::QueueMove(const Ptr<EditCommandManager>& manager, SizeT done)
{
    const bool queued = this->pendingManager.isvalid();
    this->pendingManager = manager;
    this->pendingDone = done;
    if (!queued)
        this->QueueAction(Util::Delegate<void()>::FromMethod<HistoryPanel, &HistoryPanel::RunPendingMove>(this));
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryPanel::RunPendingMove()
{
    const Ptr<EditCommandManager> manager = this->pendingManager;
    this->pendingManager = nullptr;
    MoveTo(manager, this->pendingDone);
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryPanel::CommandHistory(const Util::Array<Util::String>& args)
{
    const Ptr<EditCommandManager> manager = this->core->GetTargetCommandManager();
    if (!manager.isvalid())
    {
        this->core->Log(LogMessageType::N_WARNING, "No history");
        return;
    }

    if (args.Size() == 2 && args[0] == "goto")
    {
        const SizeT count = manager->GetUndoList().Size() + manager->GetRedoList().Size();
        const int done = args[1].AsInt();
        if (done < 0 || done > count)
        {
            this->core->Log(LogMessageType::N_WARNING, Util::Format("history goto: 0 to %d", count));
            return;
        }
        if (!MoveTo(manager, done))
            this->core->Log(LogMessageType::N_WARNING, "history goto: an entry could not be undone or redone");
    }
    else if (!args.IsEmpty() && !(args.Size() == 1 && args[0] == "list"))
    {
        this->core->Log(LogMessageType::N_WARNING, "history [list|goto <n>]");
        return;
    }

    const EditCommandManager::CommandList& undoList = manager->GetUndoList();
    const EditCommandManager::CommandList& redoList = manager->GetRedoList();
    this->core->Log(LogMessageType::N_SYSTEM, Util::Format("history: %d done, %d undone", undoList.Size(), redoList.Size()));
    for (IndexT i = 0; i < undoList.Size(); i++)
        this->core->Log(LogMessageType::N_SYSTEM, Util::Format("  %d %s (%d)", i + 1, undoList[i].name.AsCharPtr(), undoList[i].commands.Size()));
    for (IndexT i = redoList.Size() - 1; i >= 0; i--)
        this->core->Log(LogMessageType::N_SYSTEM, Util::Format("  %d %s (%d) undone", undoList.Size() + (redoList.Size() - i), redoList[i].name.AsCharPtr(), redoList[i].commands.Size()));
}

} // namespace Editor
