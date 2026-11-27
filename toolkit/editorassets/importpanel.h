#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ImportPanel
    Importing models and textures as assets. Files dropped on the editor window become imports, one
    tab each, with the import settings, a preview for a texture, and the folder in 'work:assets' the
    asset goes to. Six images named after the cube faces import as one cube texture. An asset made
    from a source file can be imported again from it, with the settings it was imported with.

    The window opens itself when something new is to be imported. Closing it keeps the imports, they
    show again with the next one.

    The import writes the work file and batches it, so the asset is ready to be referenced as soon as
    the batcher is done. What the import reports goes to the editor's console.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "io/path.h"
#include "neweditor/core/editorpanel.h"

namespace Editor
{

class ImportPanel final : public EditorPanel
{
    __DeclareClass(ImportPanel)
public:
    ImportPanel();
    virtual ~ImportPanel();

    /// draw the imports
    void Draw() override;

    /// take the files dropped on the window this frame and open the window when there is something
    /// new to import. Called every frame, whether the window is open or not
    static void Update();
    /// import a model (folder:file:mdl) or texture asset (folder:file:tex) again from its source,
    /// returns false and says why if it has no source to import
    static bool Reimport(const IO::Path& asset, Util::String& outError);

private:
    static ImportPanel* instance;
    /// the imports there were last frame, more means something new came in
    SizeT lastPendingImports;
};

} // namespace Editor
//------------------------------------------------------------------------------
