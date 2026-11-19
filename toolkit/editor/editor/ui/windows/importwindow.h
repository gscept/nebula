#pragma once
//------------------------------------------------------------------------------
/**
    Window for asset importing

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "io/path.h"

namespace Presentation
{

class AssetImporterWindow : public BaseWindow
{
public:
    /// Constructor
    AssetImporterWindow();
    /// Destructor
    ~AssetImporterWindow();

    /// Render
    void Run(SaveMode save) override;
    ///
    bool ShouldRun() const override;
    /// Opens the window when files are dropped or imports are queued, the imports are its tabs
    void Update() override;

    /// Open the import window for a model (folder:file:mdl) or texture asset (folder:file:tex) to import it again from its source,
    /// returns false if it has no source to import
    static bool Reimport(const IO::Path& asset, Util::String& outError);
private:
    SizeT lastPendingImports = 0;
};


} // namespace Presentation
