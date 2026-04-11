#pragma once
//------------------------------------------------------------------------------
/**
    Editor for models

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"

namespace Presentation
{

void ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelSetup(AssetEditorItem* item);
void ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

} // namespace Presentation
