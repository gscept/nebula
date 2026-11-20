//------------------------------------------------------------------------------
//  asseteditor.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "asseteditor.h"
#include "editor/editor.h"
#include "editor/commandmanager.h"
#include "editor/ui/uimanager.h"
#include "graphics/graphicsserver.h"
#include "resources/resourceserver.h"
#include "dynui/imguicontext.h"
#include "tinyfiledialogs.h"
#include "resources/resourceserver.h"
#include "editor/tools/pathconverter.h"
#include "editor/ui/windowserver.h"
#include "editor/ui/windows/assetbrowser.h"
#include "dynui/nebula_icons.h"

#include "animationasseteditor.h"
#include "materialasseteditor.h"
#include "meshasseteditor.h"
#include "modelasseteditor.h"
#include "skeletonasseteditor.h"
#include "textureasseteditor.h"
#include "particleasseteditor.h"

#include "editor/tools/livebatcher.h"
#include "io/ioserver.h"


#include "materials/gpulang/material_interfaces.h"

namespace Presentation
{

struct
{
    Util::Array<AssetEditorItem> items;
    /// assets an asset field asked to open, opened when the editor runs next, opening one while the items are drawn
    /// can add an item and move the one drawing the field
    Util::Array<IO::Path> pendingOpens;

} assetEditorState;


//------------------------------------------------------------------------------
/**
*/
AssetEditor::AssetEditor()
{
    this->viewport.Init(Util::String("AssetEditorViewport"), 1 << 3);

    // Setup URN work mappings
    IO::Path::AddWorkMapping("mdl", "nasset");
    IO::Path::AddWorkMapping("tex", "natex");
    IO::Path::AddWorkMapping("par", "napar");
    IO::Path::AddWorkMapping("mat", "namat");
    
}

//------------------------------------------------------------------------------
/**
*/
AssetEditor::~AssetEditor()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
EmptyEditor()
{
    ImGuiStyle& style = ImGui::GetStyle();

    static const char* EmptyString = "Nothing selected";
    float sizeX = ImGui::CalcTextSize(EmptyString).x + style.FramePadding.x * 2.0f;
    float availX = ImGui::GetContentRegionAvail().x;
    float sizeY = ImGui::CalcTextSize(EmptyString).y + style.FramePadding.y * 2.0f;
    float availY = ImGui::GetContentRegionAvail().y;

    float off = (availX - sizeX) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
    off = (availY - sizeY) * 0.5f;
    if (off > 0.0f)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + off);

    ImGui::Text(EmptyString);
}

using EditorFunc = void(*)(AssetEditor*, AssetEditorItem*);
using ShowFunc = void(*)(AssetEditor*, AssetEditorItem*, bool);
using SetupFunc = void(*)(AssetEditorItem*);

static const SetupFunc SetupFuncs[(uint)AssetEditor::AssetType::NumAssetTypes] =
{
    nullptr, // LEAVE THIS ONE AS IT IS
    MaterialSetup,
    ModelSetup,
    TextureSetup,
    ParticleSetup
};

static const EditorFunc SavingFunctions[(uint)AssetEditor::AssetType::NumAssetTypes] =
{
    nullptr, // LEAVE THIS ONE AS IT IS
    MaterialSave,
    ModelSave,
    nullptr,
    ParticleSave
};
static const EditorFunc RenderFunctions[(uint)AssetEditor::AssetType::NumAssetTypes] =
{
    nullptr, // LEAVE THIS ONE AS IT IS
    MaterialEditor,
    ModelEditor,
    TextureEditor,
    ParticleEditor
};

static const EditorFunc DiscardFunctions[(uint)AssetEditor::AssetType::NumAssetTypes] =
{
    nullptr, // LEAVE THIS ONE AS IT IS
    MaterialDiscard,
    ModelDiscard,
    nullptr,
    ParticleDiscard
};

static const ShowFunc ShowFunctions[(uint)AssetEditor::AssetType::NumAssetTypes] = 
{
    nullptr, // LEAVE THIS ONE AS IT IS
    MaterialShow,
    ModelShow,
    nullptr,
    ParticleShow
};

static const char* Labels[(uint)AssetEditor::AssetType::NumAssetTypes] =
{
    "None %s",
    "[Material] %s",
    "[Model] %s",
    "[Texture] %s",
    "[Particle] %s"
};

//------------------------------------------------------------------------------
/**
    The editor an asset (folder:file:type) opens in, None if there's none for it
*/
static AssetEditor::AssetType
AssetTypeOf(const IO::Path& asset)
{
    if (!asset.IsFile())
        return AssetEditor::AssetType::None;
    const Util::String& type = asset.GetType();
    if (type == "mat")
        return AssetEditor::AssetType::Material;
    if (type == "mdl")
        return AssetEditor::AssetType::Model;
    if (type == "tex")
        return AssetEditor::AssetType::Texture;
    return AssetEditor::AssetType::None;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditor::Run(SaveMode save)
{
    if (!assetEditorState.pendingOpens.IsEmpty())
    {
        const Util::Array<IO::Path> opens = assetEditorState.pendingOpens;
        assetEditorState.pendingOpens.Clear();
        for (const IO::Path& asset : opens)
            this->Open(asset, "", AssetTypeOf(asset));
    }

    static const char* PopupText = "Unsaved changes";
    if (!assetEditorState.items.IsEmpty())
    {
        // the editor style hides the selected tab's close button, but asset tabs have to be closable whichever is selected.
        // The style is read when the tabs are drawn, so it only applies to this tab bar
        ImGuiStyle& style = ImGui::GetStyle();
        const float closeButtonMinWidth = style.TabCloseButtonMinWidthSelected;
        style.TabCloseButtonMinWidthSelected = -1.0f;
        if (ImGui::BeginTabBar("AssetEditor###tabs", ImGuiTabBarFlags_None))
        {
            for (IndexT itemIndex = 0; itemIndex < assetEditorState.items.Size(); itemIndex++)
            {
                AssetEditorItem& item = assetEditorState.items[itemIndex];

                // an item writes back the whole asset as it was loaded, so saving an unedited one would revert
                // changes made to the file since, by a reimport for instance
                if (save == SaveMode::SaveAll && item.editCounter > 0)
                {
                    auto& func = SavingFunctions[(uint)item.assetType];
                    if (func)
                        func(this, &item);
                }

                bool open = true;
                bool closeItem = false;
                auto& hideFunc = ShowFunctions[(uint)item.assetType];
                Util::String assetName = item.path.GetFolderAndFile();
                assetName = BaseWindow::FormatName(Util::Format(Labels[(uint)item.assetType], assetName.AsCharPtr()), item.editCounter);
                if (ImGui::BeginTabItem(assetName.AsCharPtr(), &open, item.grabFocus ? ImGuiTabItemFlags_SetSelected : 0x0))
                {
                    // closing a tab with unsaved changes selects it, and asks here what to do with them
                    if (item.closeRequested)
                    {
                        item.closeRequested = false;
                        ImGui::OpenPopup(PopupText);
                    }

                    ImGui::SetNextWindowSize(ImVec2{ 300, 200 });
                    if (ImGui::BeginPopupModal(PopupText, nullptr, ImGuiWindowFlags_NoResize))
                    {
                        static const char* UnsavedMessage = "You have unsaved changes. Chose what you want to do.";
                        ImGui::TextWrapped(UnsavedMessage);
                        float availY = ImGui::GetContentRegionAvail().y;
                        float sizeY = ImGui::CalcTextSize(UnsavedMessage).y + style.FramePadding.y * 4.0f;
                        float off = (availY - sizeY);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + off);
                        if (ImGui::BeginTable("Columns", 3, ImGuiTableFlags_SizingStretchSame))
                        {
                            ImGui::TableNextColumn();

                            float availX = ImGui::GetContentRegionAvail().x;
                            float sizeX = ImGui::CalcTextSize("Save").x + style.FramePadding.x * 2.0f;
                            float off = (availX - sizeX) * 0.5f;
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                            if (ImGui::Button("Save"))
                            {
                                auto& saveFunc = SavingFunctions[(uint)item.assetType];
                                if (saveFunc)
                                    saveFunc(this, &item);

                                // a failed save keeps the changes, so the tab stays open
                                closeItem = item.editCounter == 0;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::TableNextColumn();
                            sizeX = ImGui::CalcTextSize("Discard").x + style.FramePadding.x * 2.0f;
                            off = (availX - sizeX) * 0.5f;
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                            if (ImGui::Button("Discard"))
                            {
                                auto& discardFunc = DiscardFunctions[(uint)item.assetType];
                                if (discardFunc)
                                    discardFunc(this, &item);
                                item.allocator.Release();
                                this->editCounter -= item.editCounter;
                                item.editCounter = 0;
                                closeItem = true;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::TableNextColumn();
                            sizeX = ImGui::CalcTextSize("Cancel").x + style.FramePadding.x * 2.0f;
                            off = (availX - sizeX) * 0.5f;
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                            if (ImGui::Button("Cancel"))
                            {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndPopup();
                    }

                    if (open && !closeItem)
                    {
                        item.grabFocus = false;
                        auto& saveFunc = SavingFunctions[(uint)item.assetType];
                        if (saveFunc && save == SaveMode::SaveActive && item.editCounter > 0)
                            saveFunc(this, &item);

                        // saves only this asset, the save shortcuts save the level as well
                        ImGui::BeginDisabled(!saveFunc || item.editCounter == 0);
                        if (ImGui::Button("Save") && saveFunc)
                            saveFunc(this, &item);
                        ImGui::EndDisabled();
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                            ImGui::SetTooltip(item.editCounter == 0 ? "No unsaved changes" : "Save this asset");

                        auto& renderFunc = RenderFunctions[(uint)item.assetType];
                        if (renderFunc)
                            renderFunc(this, &item);
                    }
                    ImGui::EndTabItem();

                    if (hideFunc)
                        hideFunc(this, &item, true);
                }
                else
                {
                    if (hideFunc)
                        hideFunc(this, &item, false);
                }

                // the close button works on tabs that aren't selected as well
                if (!open)
                {
                    if (item.editCounter != 0)
                    {
                        item.grabFocus = true;
                        item.closeRequested = true;
                    }
                    else
                    {
                        closeItem = true;
                    }
                }

                if (closeItem)
                {
                    // the preview shares the asset editor's viewport, so it has to stop rendering there
                    if (hideFunc)
                        hideFunc(this, &item, false);
                    assetEditorState.items.EraseIndex(itemIndex);
                    itemIndex--;
                }
            }
            ImGui::EndTabBar();
        }
        style.TabCloseButtonMinWidthSelected = closeButtonMinWidth;
    }
    else
    {
        EmptyEditor();
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AssetEditor::Update()
{
    this->viewport.Update();
}

//------------------------------------------------------------------------------
/**
*/
void
Setup(AssetEditorItem* item)
{
    item->allocator.Release();
    item->data = nullptr;

    item->previewObject = Graphics::CreateEntity();

    const SetupFunc& func = SetupFuncs[(uint)item->assetType];
    if (func)
        func(item);
}

//------------------------------------------------------------------------------
/**
*/
AssetEditorItem*
AssetEditor::GetOpenItem(const IO::Path& asset)
{
    for (AssetEditorItem& item : assetEditorState.items)
    {
        if (item.path == asset)
        {
            return &item;
        }
    }
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditor::Open(const IO::Path& asset, const Util::String root, const AssetType type)
{
    // If we try to load the same item, just focus that one
    for (AssetEditorItem& item : assetEditorState.items)
    {
        if (item.path == asset)
        {
            item.grabFocus = true;
            return;
        }
    }

    // If there is no export, quickly batch it
    if (asset.IsFile())
    {
        Editor::LiveBatcher::BatchFile(asset);
        Editor::LiveBatcher::Wait();
    }

    if (!Resources::ResourceServer::Instance()->HasStreamLoader(asset))
    {
        n_log_warn(Asset Editor, "No loader for '%s', can't open it\n", asset.GetFolderAndFile().AsCharPtr());
        return;
    }

    // Otherwise, trigger an async load and setup a new item
    Resources::CreateResource(asset, "editor",
        [asset, type](Resources::ResourceId id)
        {
            AssetEditorItem& item = assetEditorState.items.Emplace();
            item.asset.id = id.resourceId;
            item.assetType = type;
            item.res = id;
            item.path = asset;
            item.grabFocus = true;
            item.editCounter = 0;

            Setup(&item);
        },
        [asset](Resources::ResourceId id)
        {
            n_log_warn(Asset Editor, "Failed to load '%s', can't open it\n", asset.GetFolderAndFile().AsCharPtr());
        }
    );

}

//------------------------------------------------------------------------------
/**
*/
bool
AssetURN(const Util::String& resourceName, const char* type, Util::String& outURN)
{
    const IO::Path path = IO::Path::Parse(resourceName);
    if (path.IsFile() && path.GetType() == type)
    {
        outURN = resourceName;
        return true;
    }

    Util::String root = IO::URI(Util::Format("%s:", type)).LocalPath();
    root.TrimRight("/\\");
    const Util::String file = IO::URI(resourceName).LocalPath();
    if (file.Length() <= root.Length() + 1 || !file.BeginsWithString(root) || (file[root.Length()] != '/' && file[root.Length()] != '\\'))
        return false;
    outURN = IO::Path::FolderAndFile(type, file.ExtractToEnd(root.Length() + 1)).AsString();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetField(const char* id, const Util::String& label, const IO::Path& asset, const char* type, const std::function<void(const IO::Path&)>& onChange)
{
    const auto isOfType = [typeName = Util::String(type != nullptr ? type : "")](const IO::Path& path)
    {
        return path.IsFile() && (typeName.IsEmpty() || path.GetType() == typeName);
    };
    const bool isAsset = isOfType(asset);
    const bool canOpen = isAsset && AssetTypeOf(asset) != AssetEditor::AssetType::None;

    ImGui::PushID(id);
    // a single click does nothing, the folder button picks another asset
    ImGui::Button(label.IsEmpty() ? "None" : label.AsCharPtr());
    if (ImGui::IsItemHovered())
    {
        if (canOpen)
            ImGui::SetItemTooltip("%s\nDouble click to open it, right click to copy it, middle click to paste one", asset.AsString().AsCharPtr());
        else if (isAsset)
            ImGui::SetItemTooltip("%s\nRight click to copy it, middle click to paste one", asset.AsString().AsCharPtr());
        if (canOpen && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            // the asset editor only runs while it's shown
            assetEditorState.pendingOpens.Append(asset);
            Presentation::AssetEditorWindow->Open() = true;
            Presentation::AssetEditorWindow->Focus();
        }
        else if (isAsset && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImGui::SetClipboardText(asset.AsString().AsCharPtr());
        }
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
        {
            const char* clipboard = ImGui::GetClipboardText();
            const IO::Path pasted = clipboard != nullptr ? IO::Path::Parse(clipboard) : IO::Path();
            if (isOfType(pasted))
                onChange(pasted);
        }
    }
    if (ImGui::BeginDragDropTarget())
    {
        // the asset browser drags URNs, only the ones of the type are accepted, so others don't highlight the field
        const ImGuiPayload* peek = ImGui::GetDragDropPayload();
        if (peek != nullptr && peek->IsDataType("resource"))
        {
            const IO::Path dropped = IO::Path::Parse((const char*)peek->Data);
            if (isOfType(dropped) && ImGui::AcceptDragDropPayload("resource") != nullptr)
                onChange(dropped);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_ttf_FOLDER_OPEN))
    {
        auto browser = (Presentation::AssetBrowser*)Presentation::AssetBrowserPickerWindow;
        browser->PickFile(asset, [onChange, isOfType](const IO::Path& picked)
        {
            if (isOfType(picked))
                onChange(picked);
            else
                n_warning("'%s' can't be used here\n", picked.AsString().AsCharPtr());
        }, type != nullptr ? type : "");
    }
    ImGui::PopID();
}

} // namespace Presentation
