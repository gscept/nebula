#pragma once
//------------------------------------------------------------------------------
/**
    Asset editor for Materials

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"

namespace Presentation
{

struct MaterialEditorItemData
{
    ImageHolder* images;
    ubyte* constants;

    ImageHolder* originalImages;
    ubyte* originalConstants;
};

void MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialSetup(AssetEditorItem* item);
void MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

void MaterialSerialize(const Ptr<IO::Stream>& stream, const MaterialEditorItemData* data, Materials::MaterialId mat, const MaterialTemplatesGPULang::Entry* materialTemplate);

} // namespace Presentation
