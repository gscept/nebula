#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::BatcherPanel
    The batcher's log: what the asset batcher printed for the jobs the editor ran, so a failed export
    can be read where it happened. While jobs run, the status bar says so, which is what tells an
    author an asset is still being exported without the panel open.
    The log is the live batcher's. The panel copies it only when it changed, because the batching
    thread appends to it.
    @see
    Editor::LiveBatcher
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "neweditor/core/editorpanel.h"
#include "util/string.h"

namespace Editor
{

class BatcherPanel final : public EditorPanel
{
    __DeclareClass(BatcherPanel)
public:
    BatcherPanel();
    virtual ~BatcherPanel();

    /// draw the toolbar and the log
    void Draw() override;

private:
    /// the copy of the log the panel draws
    Util::String log;
    /// the log version the copy was taken at
    uint logVersion;
    /// keep the view at the end of the log while it is there
    bool autoScroll;
};

} // namespace Editor
//------------------------------------------------------------------------------
