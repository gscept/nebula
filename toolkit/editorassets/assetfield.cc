//------------------------------------------------------------------------------
//  @file assetfield.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetfield.h"
#include "editorassets/assetbrowser.h"
#include "editorassets/assetpicker.h"
#include "editorassets/assettype.h"
#include "neweditor/base/iconregistry.h"
#include "imgui.h"

namespace Editor
{

//------------------------------------------------------------------------------
/**
*/
void
AssetField(const char* id, const IO::Path& asset, const char* type, const std::function<void(const IO::Path&)>& onChange, float width)
{
    const Util::String typeName = type != nullptr ? type : "";
    const bool isAsset = asset.IsFile() && !asset.GetType().IsEmpty() && (typeName.IsEmpty() || asset.GetType() == typeName);
    // an export like a model's skeleton opens the asset it is exported from
    IO::Path source;
    const AssetType* assetType = isAsset ? AssetTypeRegistry::FindSource(asset, source) : nullptr;
    const bool canOpen = assetType != nullptr && assetType->open.IsValid();

    ImGui::PushID(id);

    // the folder button sits at the end of the line, the name takes the rest
    const char* folderGlyph = IconGlyph("folder-open");
    const char* pickLabel = folderGlyph != nullptr ? folderGlyph : "...";
    const float pickWidth = ImGui::CalcTextSize(pickLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float fieldWidth = width > 0.0f ? width : ImGui::GetContentRegionAvail().x;
    const float nameWidth = Math::max(fieldWidth - pickWidth - ImGui::GetStyle().ItemSpacing.x, 1.0f);

    // a single click does nothing, the folder button picks another asset
    const Util::String label = isAsset ? asset.GetFolderAndFile() : (asset.IsEmpty() ? Util::String("None") : asset.AsString());
    ImGui::Button(Util::Format("%s##name", label.AsCharPtr()).AsCharPtr(), ImVec2(nameWidth, 0.0f));
    if (ImGui::IsItemHovered())
    {
        if (canOpen)
            ImGui::SetItemTooltip("%s\nDouble click to open it, right click to copy it, middle click to paste one", asset.AsString().AsCharPtr());
        else if (isAsset)
            ImGui::SetItemTooltip("%s\nRight click to copy it, middle click to paste one", asset.AsString().AsCharPtr());
        else
            ImGui::SetItemTooltip("Drop an asset here, middle click to paste one");

        if (canOpen && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            assetType->open(source);
        }
        else if (isAsset && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::SetClipboardText(asset.AsString().AsCharPtr());
        }
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
            const char* clipboard = ImGui::GetClipboardText();
            const IO::Path pasted = clipboard != nullptr ? IO::Path::Parse(clipboard) : IO::Path();
            IO::Path taken;
            if (AssetTypeRegistry::As(pasted, typeName, taken))
                onChange(taken);
            else if (pasted.IsFile())
                n_warning("%s can't be used here, the field takes %s\n", pasted.AsString().AsCharPtr(), typeName.IsEmpty() ? "an asset" : typeName.AsCharPtr());
        }
    }
    if (ImGui::BeginDragDropTarget())
    {
        // only assets of the type are accepted, so others don't highlight the field
        const ImGuiPayload* peek = ImGui::GetDragDropPayload();
        if (peek != nullptr && peek->IsDataType(AssetDragPayload))
        {
            const IO::Path dropped = IO::Path::Parse((const char*)peek->Data);
            IO::Path taken;
            if (AssetTypeRegistry::As(dropped, typeName, taken) && ImGui::AcceptDragDropPayload(AssetDragPayload) != nullptr)
                onChange(taken);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button(Util::Format("%s##pick", pickLabel).AsCharPtr()))
    {
        if (AssetPicker::HasInstance())
        {
            // by value, the picker's answer comes after this has returned
            AssetPicker::Instance()->PickFile(asset, typeName, [onChange, typeName](const IO::Path& picked)
            {
                IO::Path taken;
                if (AssetTypeRegistry::As(picked, typeName, taken))
                    onChange(taken);
                else if (picked.IsFile())
                    n_warning("%s has no %s to use here\n", picked.AsString().AsCharPtr(), typeName.AsCharPtr());
            });
        }
    }
    ImGui::SetItemTooltip("Pick another in the asset picker");
    ImGui::PopID();
}

} // namespace Editor
