#pragma once
//------------------------------------------------------------------------------
/**
    Asset editor for Materials

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "asseteditor.h"

namespace Presentation
{

void MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item, BaseWindow::SaveMode save);
void MaterialSetup(AssetEditorItem* item);
void MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item);
} // namespace Presentation
