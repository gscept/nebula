#pragma once
//------------------------------------------------------------------------------
/**
    @file skeletonview.h
    A skeleton in the asset editor: its joints as a tree, and its bones and joints drawn over the
    asset preview.

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
// the skeleton header needs the allocators declared first
#include "ids/idallocator.h"
#include "characters/skeleton.h"
#include "math/mat4.h"
#include "util/fixedarray.h"
#include "imgui.h"

namespace GraphicsFeature
{

/// draw the joint hierarchy of a skeleton as a tree, returns the joint hovered, InvalidIndex if none is
IndexT SkeletonJointTree(const Characters::SkeletonId skeleton);
/// draw the bones and joints of a pose over the image of a view, the joint transforms are in model space and
/// the transform places the model. The view projection is the camera's the image was rendered with, the image is
/// at min and max. The highlighted joint is drawn bigger, with its name
void SkeletonOverlay(ImDrawList* drawList, const Math::mat4& viewProjection, const ImVec2& imageMin, const ImVec2& imageMax, const Characters::SkeletonId skeleton, const Util::FixedArray<Math::mat4>& joints, const Math::mat4& transform, const IndexT highlight, const Util::FixedArray<float>* weights = nullptr);

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
