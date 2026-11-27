#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::HistoryPanel

    The undo history: what undo would take back, from the oldest to the newest, and after it, greyed
    out, what redo would do again.

    The history shown is the one undo and redo run against, the scene's or that of the asset editor
    tab last worked in, so it follows the work. While the panel itself is the one last worked in it
    keeps the history it showed, and undo and redo still act on it.

    Clicking an entry undoes or redoes until it is the last one done. The first row is the state
    before anything in the history was done.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/core/editorpanel.h"
#include "core/ptr.h"
#include "util/array.h"
#include "util/string.h"

namespace Editor
{

class EditCommandManager;

class HistoryPanel final : public EditorPanel
{
    __DeclareClass(HistoryPanel)
public:
    HistoryPanel();
    virtual ~HistoryPanel();

    void Draw() override;
    /// the history the panel shows, so undo and redo keep running against it while it is in front
    Ptr<EditCommandManager> GetCommandManager() const override;

protected:
    bool OnSetup() override;

private:
    /// undo or redo until this many entries are done. Returns false when the history ran out first
    static bool MoveTo(const Ptr<EditCommandManager>& manager, SizeT done);
    /// undo or redo to this many entries done once the frame has drawn
    void QueueMove(const Ptr<EditCommandManager>& manager, SizeT done);
    void RunPendingMove();
    /// what the history belongs to, for the header
    Util::String DescribeOwner() const;

    /// history [list|goto <n>] - print the history, or undo and redo until n entries are done
    void CommandHistory(const Util::Array<Util::String>& args);

    Ptr<EditCommandManager> shown;
    Ptr<EditCommandManager> pendingManager;
    SizeT pendingDone;
    /// the entries there were at the last draw, the list scrolls to the end when it changes
    SizeT lastCount;
};

} // namespace Editor
