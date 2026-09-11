#pragma once
//------------------------------------------------------------------------------
/**
    Window for asset importing

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"

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
private:
};


} // namespace Presentation
