//------------------------------------------------------------------------------
//  @file skeletonasseteditor.cc
//  @copyright (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "skeletonasseteditor.h"

namespace Presentation
{

//------------------------------------------------------------------------------
/**
    Joint names are only kept in debug builds, the others show the index
*/
static Util::String
JointName(const Characters::CharacterJoint& joint, const IndexT index)
{
#if NEBULA_DEBUG
    if (joint.name.IsValid())
        return joint.name.Value();
#endif
    return Util::Format("Joint %d", index);
}

//------------------------------------------------------------------------------
/**
*/
static bool
IsRootJoint(const Util::FixedArray<Characters::CharacterJoint>& joints, const IndexT index)
{
    const IndexT parent = joints[index].parentJointIndex;
    return parent < 0 || parent >= joints.Size();
}

//------------------------------------------------------------------------------
/**
*/
static void
JointTreeNode(const Util::FixedArray<Characters::CharacterJoint>& joints, const IndexT jointIndex, IndexT& hovered)
{
    bool hasChildren = false;
    for (IndexT i = 0; i < joints.Size() && !hasChildren; i++)
        hasChildren = joints[i].parentJointIndex == jointIndex;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    const bool open = ImGui::TreeNodeEx((void*)(intptr_t)jointIndex, flags, "%s", JointName(joints[jointIndex], jointIndex).AsCharPtr());
    if (ImGui::IsItemHovered())
        hovered = jointIndex;
    if (open && hasChildren)
    {
        for (IndexT i = 0; i < joints.Size(); i++)
        {
            if (joints[i].parentJointIndex == jointIndex)
                JointTreeNode(joints, i, hovered);
        }
        ImGui::TreePop();
    }
}

//------------------------------------------------------------------------------
/**
*/
IndexT
SkeletonJointTree(const Characters::SkeletonId skeleton)
{
    IndexT hovered = InvalidIndex;
    const Util::FixedArray<Characters::CharacterJoint>& joints = Characters::SkeletonGetJoints(skeleton);
    ImGui::PushID("joints");
    for (IndexT i = 0; i < joints.Size(); i++)
    {
        if (IsRootJoint(joints, i))
            JointTreeNode(joints, i, hovered);
    }
    ImGui::PopID();
    return hovered;
}

//------------------------------------------------------------------------------
/**
*/
void
SkeletonOverlay(ImDrawList* drawList, const Modules::Viewport& viewport, const Characters::SkeletonId skeleton, const Util::FixedArray<Math::mat4>& joints, const Math::mat4& transform, const IndexT highlight, const Util::FixedArray<float>* weights)
{
    const Util::FixedArray<Characters::CharacterJoint>& skeletonJoints = Characters::SkeletonGetJoints(skeleton);
    if (joints.Size() != skeletonJoints.Size())
        return;

    const Math::mat4 viewProjection = viewport.camera.GetProjectionTransform() * viewport.camera.GetViewTransform() * transform;
    const ImVec2 imagePos(viewport.lastViewportImagePositionAbsolute.x, viewport.lastViewportImagePositionAbsolute.y);
    const ImVec2 imageSize(viewport.lastViewportImageSizeAbsolute.x, viewport.lastViewportImageSizeAbsolute.y);

    // project the joints, the ones behind the camera aren't drawn
    Util::FixedArray<ImVec2> points(joints.Size());
    Util::FixedArray<bool> visible(joints.Size());
    for (IndexT i = 0; i < joints.Size(); i++)
    {
        const Math::vec4 clip = viewProjection * Math::vec4(joints[i].position().vec, 1.0f);
        visible[i] = clip.w > 0.0001f;
        if (!visible[i])
            continue;
        const float x = clip.x / clip.w;
        const float y = clip.y / clip.w;
        points[i] = ImVec2(imagePos.x + (x * 0.5f + 0.5f) * imageSize.x, imagePos.y + (0.5f - y * 0.5f) * imageSize.y);
    }

    const ImU32 boneColor = IM_COL32(255, 200, 60, 220);
    const ImU32 jointColor = IM_COL32(255, 120, 40, 255);
    const ImU32 highlightColor = IM_COL32(80, 220, 255, 255);
    for (IndexT i = 0; i < joints.Size(); i++)
    {
        if (!visible[i] || IsRootJoint(skeletonJoints, i))
            continue;
        const IndexT parent = skeletonJoints[i].parentJointIndex;
        if (visible[parent])
            drawList->AddLine(points[parent], points[i], boneColor, 2.0f);
    }
    // with weights, like a joint mask's, each joint gets a ring filled clockwise from the top by its weight, and a dot
    // from grey at 0 to green at 1
    const float ringRadius = 9.0f;
    const ImU32 ringColor = IM_COL32(20, 20, 20, 160);
    const ImU32 weightColor = IM_COL32(60, 230, 90, 255);
    for (IndexT i = 0; i < joints.Size(); i++)
    {
        if (!visible[i])
            continue;
        if (weights != nullptr && i < weights->Size())
        {
            const float w = Math::clamp((*weights)[i], 0.0f, 1.0f);
            drawList->AddCircle(points[i], ringRadius, ringColor, 0, 4.0f);
            if (w >= 0.999f)
            {
                drawList->AddCircle(points[i], ringRadius, weightColor, 0, 2.5f);
            }
            else if (w > 0.0f)
            {
                const float start = -PI * 0.5f;
                drawList->PathArcTo(points[i], ringRadius, start, start + 2.0f * PI * w);
                drawList->PathStroke(weightColor, 2.5f);
            }
            if (i != highlight)
            {
                const ImU32 color = IM_COL32((int)Math::lerp(110.0f, 60.0f, w), (int)Math::lerp(110.0f, 230.0f, w), (int)Math::lerp(110.0f, 90.0f, w), 255);
                drawList->AddCircleFilled(points[i], 3.0f, color);
            }
        }
        else if (i != highlight)
        {
            drawList->AddCircleFilled(points[i], 3.0f, jointColor);
        }
    }
    if (highlight >= 0 && highlight < joints.Size() && visible[highlight])
    {
        drawList->AddCircleFilled(points[highlight], 6.0f, highlightColor);
        Util::String name = JointName(skeletonJoints[highlight], highlight);
        if (weights != nullptr && highlight < weights->Size())
            name.Append(Util::Format(" (%.2f)", (*weights)[highlight]));
        drawList->AddText(ImVec2(points[highlight].x + 8.0f, points[highlight].y - 8.0f), highlightColor, name.AsCharPtr());
    }
}

} // namespace Presentation
