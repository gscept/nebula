#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::AssetBrowserPanel
    The asset browser window: a view of the asset index the whole editor shares. Double clicking an
    asset opens it in the editor its type registered, and an asset is dragged out by its URN.
    The shared edit keys act on the browser while it is the panel last worked in: F2 renames the
    selected folder or file and Ctrl+C copies the selected asset's URN. The keys the browser has no
    use for yet are taken as well, so a delete meant for an asset never removes entities from the
    scene.
    @see
    Editor::AssetBrowser
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "io/path.h"
#include "neweditor/core/editorpanel.h"

namespace Editor
{

class AssetBrowser;

class AssetBrowserPanel final : public EditorPanel
{
    __DeclareClass(AssetBrowserPanel)
public:
    AssetBrowserPanel();
    virtual ~AssetBrowserPanel();

    /// draw the browser
    void Draw() override;
    /// the browser answers the edit keys for the assets
    bool OnEditAction(EditAction action) override;
    /// the folder selected in the browser, false when none is or there is no browser. Where files
    /// dropped on the window are imported to
    static bool GetSelectedFolder(IO::Path& outFolder);

protected:
    bool OnSetup() override;

private:
    static AssetBrowserPanel* instance;
    Ptr<AssetBrowser> browser;
};

} // namespace Editor
//------------------------------------------------------------------------------
