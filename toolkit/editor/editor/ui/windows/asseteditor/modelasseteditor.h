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


struct ModelEditorItemData
{
    ToolkitUtil::ModelAssetT asset;
    /// the mesh resource packaged from the asset, invalid until it's loaded or if it isn't exported
    CoreGraphics::MeshResourceId mesh = CoreGraphics::InvalidMeshResourceId;
    /// draw the skeleton of a skinned model over the viewport
    bool showSkeleton = true;
    /// loop the clips played from the animation section
    bool loopClips = true;
    /// the clip last played from the animation section
    IndexT clip = 0;
    /// the clip section played, its clip is looped within it, InvalidIndex if none is
    IndexT previewSection = InvalidIndex;
    /// an event the preview fired
    struct FiredEvent
    {
        Util::String name;
        Util::String category;
        Util::String clip;
        double time;        ///< ImGui time it fired at
    };
    /// the events the preview fired last, the latest last
    Util::Array<FiredEvent> firedEvents;
    /// the joint hovered in the skeleton section, highlighted in the viewport
    IndexT hoveredJoint = InvalidIndex;
};


void ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelSetup(AssetEditorItem* item);
void ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelSave(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

} // namespace Presentation
