#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetDocument
    One asset open in the asset editor, the tab it is drawn in. A subsystem derives one per asset
    type it can edit and opens it from the type's open delegate, so the asset editor knows nothing
    about what it shows.

    A document has its own undo history, which the editor's undo runs against while its tab is the
    one in front. It knows whether it has changes from that history: what it was at the last save is
    remembered, and a change made outside it is marked with MarkChanged.

    Opening makes sure the asset's export is there and current, then the type loads what it shows,
    usually asynchronously. Until it says it is ready the tab shows that it is loading, and a load
    that fails closes the tab again. Saving writes the work file through the type and batches it, so
    what refers to the asset sees the change.

    @see
    Editor::AssetEditorPanel
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/ptr.h"
#include "io/path.h"
#include "neweditor/api/editaction.h"
#include "util/string.h"

namespace Editor
{

class Shell;
class EditCommand;
class EditCommandManager;

class AssetDocument : public Core::RefCounted
{
    __DeclareClass(AssetDocument)
public:
    AssetDocument();
    virtual ~AssetDocument();

    /// open the asset. Returns false when it can't be edited, and the tab is not made
    bool Open(const IO::Path& asset, Shell* shell);
    /// close it, the changes are gone. Called by the editor after a save or a discard
    void Close();

    /// the asset the document edits
    const IO::Path& GetAsset() const;
    /// the undo history of the document
    Ptr<EditCommandManager> GetCommandManager() const;

    /// draw the document. Only called once it is ready
    virtual void Draw();
    /// the tab came to the front or went behind another one
    virtual void OnShown(bool shown);
    /// answer a shared edit action for what is selected in the document. Returns false by default,
    /// and the editor takes the action without doing anything
    virtual bool OnEditAction(EditAction action);
    /// true when the type writes changes back. A document that only shows its asset leaves this false
    virtual bool CanSave() const;

    /// write the changes to the work file and batch it. Returns false and keeps them when they could not be written
    bool Save();
    /// throw the changes away and show the asset as the work file has it
    void Revert();
    /// true while there are changes that are not saved
    bool IsDirty() const;
    /// true once what the document shows is loaded
    bool IsReady() const;
    /// true when it could not be loaded. The reason says why
    bool HasFailed() const;
    /// why it could not be loaded
    const Util::String& GetFailReason() const;

protected:
    /// load what the document shows. Return false when it can't be loaded at all, call SetReady or
    /// SetFailed once a load started here has finished
    virtual bool OnOpen();
    /// release what the document loaded
    virtual void OnClose();
    /// write the changes to the work file. Return false and say why when they could not be written
    virtual bool OnSave(Util::String& outError);
    /// go back to how the work file has the asset
    virtual void OnRevert();

    /// the document is loaded and can be drawn
    void SetReady();
    /// the document could not be loaded, the editor closes its tab
    void SetFailed(const Util::String& reason);
    /// a change that is not an undoable command, it stays a change until the next save
    void MarkChanged();
    /// true once Close ran, a load finishing after it has to let go of what it loaded
    bool IsClosed() const;

    IO::Path asset;
    Shell* shell;
    Ptr<EditCommandManager> commands;

private:
    /// the last command in the undo history, null when it is empty
    Ptr<EditCommand> TopCommand() const;

    /// the last command at the last save, the document is clean while it is still the last one
    Ptr<EditCommand> savedTop;
    bool changedOutsideCommands;
    bool ready;
    bool failed;
    bool closed;
    Util::String failReason;
};

} // namespace Editor
//------------------------------------------------------------------------------
