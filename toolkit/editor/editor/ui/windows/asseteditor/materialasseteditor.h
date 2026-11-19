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
    bool renderViewport = true;

    /// the template shown and saved. A material can't change its template once loaded, so after switching templates
    /// this differs from the live material's template, which stays as it was loaded until the material is loaded again
    const MaterialTemplatesGPULang::Entry* editTemplate = nullptr;

    /// textures and constants of editTemplate as committed by the editor commands
    ImageHolder* images;
    ubyte* constants;
    /// constants the widgets show and change while dragging, for a template the live material doesn't have
    ubyte* shownConstants = nullptr;

    /// textures the work file has for the slots of editTemplate, which tells the changed ones apart
    ImageHolder* originalImages;
    /// the live material's textures and constants as loaded or last saved, which discarding restores
    ImageHolder* liveImages = nullptr;
    ubyte* originalConstants;

    ToolkitUtil::MaterialResourceT asset;
};

void MaterialEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialSetup(AssetEditorItem* item);
void MaterialSave(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void MaterialShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);
void MaterialNew(const Ptr<IO::Stream>& stream, const Util::String& path, const Util::String& templateName);

void MaterialSerialize(const Ptr<IO::Stream>& stream, const MaterialEditorItemData* data, Materials::MaterialId mat, const MaterialTemplatesGPULang::Entry* materialTemplate);

} // namespace Presentation
