//------------------------------------------------------------------------------
//  @file modelasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "modelasseteditor.h"
#include "visibility/visibilitycontext.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
*/
void ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item)
{
    assetEditor->viewport.Render();
}

//------------------------------------------------------------------------------
/**
*/
void ModelSetup(AssetEditorItem* item)
{
    Models::ModelContext::RegisterEntity(item->previewObject);
    Models::ModelContext::Setup(
        item->previewObject,
        item->name,
        "preview",
        [gid = item->previewObject]()
        {
            Visibility::ObservableContext::RegisterEntity(gid);
            Visibility::ObservableContext::Setup(gid, Visibility::VisibilityEntityType::Model);
        },
        1 << 3
    );
    Models::ModelContext::SetAlwaysVisible(item->previewObject);
}

//------------------------------------------------------------------------------
/**
*/
void ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item)
{
}

//------------------------------------------------------------------------------
/**
*/
void ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show)
{
    Models::ModelContext::SetStageMask(item->previewObject, show ? 1 << 3 : 0x0);
}

} // namespace Base
