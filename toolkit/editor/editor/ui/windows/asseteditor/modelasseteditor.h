#pragma once
//------------------------------------------------------------------------------
/**
    Editor for models

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"
#include "coreanimation/animsamplemask.h"
#include "animationasseteditor.h"

namespace Presentation
{


struct ModelEditorItemData
{
    ToolkitUtil::ModelAssetT asset;
    /// the mesh resource packaged from the asset, invalid until it's loaded or if it isn't exported
    CoreGraphics::MeshResourceId mesh = CoreGraphics::InvalidMeshResourceId;
    /// draw the skeleton of a skinned model over the viewport
    bool showSkeleton = true;
    /// how the clips are played from the animation section
    ClipPlayback playback;
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
    /// the joint mask edited in the skeleton section, its weights are shown in the viewport, InvalidIndex if none is
    IndexT editedMask = InvalidIndex;
    /// the joint mask hovered in the list of masks, its weights are shown in the viewport instead of the edited one's
    IndexT hoveredMask = InvalidIndex;
    /// the clip played on the second track, mixed over the one played on the first on the joints of the layer mask
    IndexT layerClip = 0;
    /// the joint mask of the asset the layer is mixed with, InvalidIndex for all joints
    IndexT layerMask = InvalidIndex;
    /// the weight the layer is mixed with
    float layerWeight = 1.0f;
    /// the speed the layer was played at, it follows the speed of the clips
    float layerSpeed = 1.0f;
    /// the mask the layer is played with, a weight per joint of the skeleton. It's sized once when the item is set up,
    /// the character's jobs read it while it plays, so the weights are only changed in place
    CoreAnimation::AnimSampleMask layerSampleMask;
};


void ModelEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelSetup(AssetEditorItem* item);
void ModelDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelSave(AssetEditor* assetEditor, AssetEditorItem* item);
void ModelShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

} // namespace Presentation
