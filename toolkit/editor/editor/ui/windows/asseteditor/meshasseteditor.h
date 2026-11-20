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

/// show the meshes of a mesh resource with their vertex layout and primitive groups
void MeshInfo(const CoreGraphics::MeshResourceId resource);
void MeshEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void MeshSetup(AssetEditorItem* item);
void MeshDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void MeshShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

} // namespace Presentation
