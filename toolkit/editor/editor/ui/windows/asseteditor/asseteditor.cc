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

#include "animationasseteditor.h"
#include "materialasseteditor.h"
#include "meshasseteditor.h"
#include "modelasseteditor.h"
#include "skeletonasseteditor.h"
#include "textureasseteditor.h"

#include "materials/material_interfaces.h"

namespace Presentation
{

struct
{
    Util::Array<AssetEditorItem> items;

} assetEditorState;

__ImplementClass(Presentation::AssetEditor, 'PrvW', Presentation::BaseWindow);

//------------------------------------------------------------------------------
/**
*/
AssetEditor::AssetEditor()
{
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

//------------------------------------------------------------------------------
/**
*/
void
AssetEditor::Run(SaveMode save)
{
    using EditorFunc = void(*)(AssetEditor*, AssetEditorItem*);
    static const EditorFunc SavingFunctions[(uint)AssetType::NumAssetTypes] =
    {
        nullptr, // LEAVE THIS ONE AS IT IS
        MaterialSave,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };
    static const EditorFunc RenderFunctions[(uint)AssetType::NumAssetTypes] =
    {
        nullptr, // LEAVE THIS ONE AS IT IS
        MaterialEditor,
        MeshEditor,
        nullptr,
        nullptr,
        nullptr,
        TextureEditor
    };

    static const EditorFunc DiscardFunctions[(uint)AssetType::NumAssetTypes] =
    {
        nullptr, // LEAVE THIS ONE AS IT IS
        MaterialDiscard,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };

    static const char* Labels[(uint)AssetType::NumAssetTypes] =
    {
        "None %s",
        "[Material] %s",
        "[Mesh] %s",
        "[Skeleton] %s",
        "[Model] %s",
        "[Animation] %s",
        "[Texture] %s"
    };

    static const char* PopupText = "Unsaved changes";
    if (!assetEditorState.items.IsEmpty())
    {
        if (ImGui::BeginTabBar("AssetEditor###tabs", ImGuiTabBarFlags_None))
        {
            for (IndexT itemIndex = 0; itemIndex < assetEditorState.items.Size(); itemIndex++)
            {
                AssetEditorItem& item = assetEditorState.items[itemIndex];
                if (save == SaveMode::SaveAll)
                {
                    auto& func = SavingFunctions[(uint)item.assetType];
                    if (func)
                        func(this, &item);
                }

                bool open;
                Util::String assetName = Editor::PathConverter::StripAssetName(item.name.AsString());
                assetName = BaseWindow::FormatName(Util::Format(Labels[(uint)item.assetType], assetName.AsCharPtr()), item.editCounter);
                if (ImGui::BeginTabItem(assetName.AsCharPtr(), &open, item.grabFocus ? ImGuiTabItemFlags_SetSelected : 0x0))
                {
                    ImGui::SetNextWindowSize(ImVec2{ 300, 200 });
                    if (ImGui::BeginPopupModal(PopupText, nullptr, ImGuiWindowFlags_NoResize))
                    {
                        ImGuiStyle& style = ImGui::GetStyle();
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
                                assetEditorState.items.Erase(&item);
                                itemIndex--;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::TableNextColumn();
                            sizeX = ImGui::CalcTextSize("Discard").x + style.FramePadding.x * 2.0f;
                            off = (availX - sizeX) * 0.5f;
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
                            if (ImGui::Button("Discard"))
                            {
                                // TODO: Restore to initial state...
                                auto& discardFunc = DiscardFunctions[(uint)item.assetType];
                                if (discardFunc)
                                    discardFunc(this, &item);
                                this->editCounter -= item.editCounter;
                                item.editCounter = 0;
                                assetEditorState.items.Erase(&item);
                                itemIndex--;
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

                    // If item was closed, remove item from 
                    if (open)
                    {
                        item.grabFocus = false;
                        auto& saveFunc = SavingFunctions[(uint)item.assetType];
                        if (saveFunc && save == SaveMode::SaveActive)
                            saveFunc(this, &item);

                        auto& renderFunc = RenderFunctions[(uint)item.assetType];
                        if (renderFunc)
                            renderFunc(this, &item);
                    }
                    else
                    {
                        if (item.editCounter != 0)
                        {
                            ImGui::OpenPopup(PopupText);
                        }
                        else
                        {
                            assetEditorState.items.Erase(&item);
                            itemIndex--;
                        }
                    }
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
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
Setup(AssetEditorItem* item)
{
    item->allocator.Release();
    item->data = nullptr;
    using SetupFunc = void(*)(AssetEditorItem*);

    static const SetupFunc SetupFuncs[(uint)AssetEditor::AssetType::NumAssetTypes] =
    {
        nullptr, // LEAVE THIS ONE AS IT IS
        MaterialSetup,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        TextureSetup
    };

    const SetupFunc& func = SetupFuncs[(uint)item->assetType];
    if (func)
        func(item);
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditor::Open(const Resources::ResourceName& asset, const AssetType type)
{
    // If we try to load the same item, just focus that one
    for (AssetEditorItem& item : assetEditorState.items)
    {
        if (item.name == asset)
        {
            item.grabFocus = true;
            return;
        }
    }

    // Otherwise, trigger an async load and setup a new item
    Resources::CreateResource(asset, "editor",
        [this, asset, type](Resources::ResourceId id)
        {
            AssetEditorItem& item = assetEditorState.items.Emplace();
            item.asset.id = id.resourceId;
            item.assetType = type;
            item.res = id;
            item.name = asset;
            item.grabFocus = true;
            item.editCounter = 0;
            Setup(&item);
        }
    );
    
}

} // namespace Presentation
