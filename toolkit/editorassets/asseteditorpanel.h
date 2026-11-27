#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetEditorPanel
    The asset editor window: one tab per open asset, each drawn by the document its type made. The
    window knows nothing about the types, a subsystem's open delegate makes the document and hands it
    to Open.

    The tab in front is what the editor's undo, redo and save act on while the window is the panel
    last worked in. Closing a tab with changes asks whether to save them first, and a save that fails
    keeps the tab open.

    Opening is queued and done when the window draws next, so a document is never added while the
    tabs are being drawn. Opening also brings the window up if it was closed.

    The 'asset' command does from the console what the browser and the tabs do: open an asset by its
    URN, close or save one, and list what is open.

    @see
    Editor::AssetDocument
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "io/path.h"
#include "neweditor/core/editorpanel.h"
#include "util/array.h"

namespace Editor
{

class AssetDocument;

class AssetEditorPanel final : public EditorPanel
{
    __DeclareClass(AssetEditorPanel)
public:
    AssetEditorPanel();
    virtual ~AssetEditorPanel();

    /// open a document in the asset editor, or bring its tab to the front when its asset is open
    /// already
    static void Open(const Ptr<AssetDocument>& document);
    /// the document an asset is open in, nullptr when it isn't open
    static AssetDocument* Find(const IO::Path& asset);
    /// close the documents of a type at once, whatever their changes. A subsystem that stops does
    /// this, because its documents hold what it is about to take down
    static void CloseDocumentsOf(const Util::String& type);

    /// open the window. A window that is closed, collapsed or behind another tab has no tab in front,
    /// so a document stops showing what it renders
    bool BeginWindow() override;
    /// draw the tabs
    void Draw() override;
    /// the undo history of the tab in front
    Ptr<EditCommandManager> GetCommandManager() const override;
    /// the tab in front answers the edit keys
    bool OnEditAction(EditAction action) override;

protected:
    bool OnSetup() override;

private:
    /// open what was queued
    void OpenPending();
    /// close a tab, asking first when it has changes
    void RequestClose(AssetDocument* document);
    /// close a tab now, whatever its changes
    void CloseDocument(AssetDocument* document);
    /// make a tab the one in front, telling the documents
    void SetActive(AssetDocument* document);
    /// console command: asset [list|open <urn>|close [urn]|save|reimport <urn>]
    void CommandAsset(const Util::Array<Util::String>& args);
    /// the open assets with changes, for the exit question
    void ListUnsaved(Util::Array<Util::String>& out);
    /// save every open asset with changes, false when one could not be saved
    bool SaveAll();
    /// the answers to the unsaved changes question
    void SaveAndClose();
    void DiscardAndClose();
    void KeepOpen();

    static AssetEditorPanel* instance;

    Util::Array<Ptr<AssetDocument>> documents;
    Util::Array<Ptr<AssetDocument>> pendingOpens;
    /// the tab in front
    AssetDocument* active;
    /// the tab to bring to the front next draw
    AssetDocument* selectRequest;
    /// the tab the unsaved changes question is about
    Ptr<AssetDocument> closeAsked;
};

} // namespace Editor
//------------------------------------------------------------------------------
