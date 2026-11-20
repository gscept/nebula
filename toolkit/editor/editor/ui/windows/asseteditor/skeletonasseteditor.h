#pragma once
//------------------------------------------------------------------------------
/**
    Skeleton asset editor

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"
#include "characters/skeleton.h"

namespace Presentation
{

/// draw the joint hierarchy of a skeleton as a tree, returns the joint hovered, InvalidIndex if none is
IndexT SkeletonJointTree(const Characters::SkeletonId skeleton);
/// draw the bones and joints of a pose over the image of a viewport, the joint transforms are in model space and
/// the transform places the model. The highlighted joint is drawn bigger, with its name
void SkeletonOverlay(ImDrawList* drawList, const Modules::Viewport& viewport, const Characters::SkeletonId skeleton, const Util::FixedArray<Math::mat4>& joints, const Math::mat4& transform, const IndexT highlight, const Util::FixedArray<float>* weights = nullptr);

} // namespace Presentation
