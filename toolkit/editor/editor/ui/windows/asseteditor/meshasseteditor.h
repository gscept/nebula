#pragma once
//------------------------------------------------------------------------------
/**
    Editor for mesh assets

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"

namespace Presentation
{

void MeshEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void MeshSetup(AssetEditorItem* item);
void MeshDiscard(AssetEditor* assetEditor, AssetEditorItem* item);

} // namespace Presentation
