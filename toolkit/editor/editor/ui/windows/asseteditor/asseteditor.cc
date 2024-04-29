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

#include "materialasseteditor.h"

#include "materials/material_interfaces.h"

namespace Presentation
{

struct
{
    Util::Array<AssetEditorItem> items;

} previewerState;

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
    if (ImGui::BeginTabBar("AssetEditor###tabs", ImGuiTabBarFlags_None))
    {
        for (AssetEditorItem& item : previewerState.items)
        {
            if (save == SaveMode::SaveAll)
            {
                switch (item.assetType)
                {
                    case AssetType::Material:
                        MaterialSave(this, &item);
                        break;
                    case AssetType::Mesh:
                        break;
                    case AssetType::Skeleton:
                        break;
                    case AssetType::Model:
                        break;
                }
            }

            bool open;
            Util::String assetName = Editor::PathConverter::StripAssetName(item.name.AsString());
            assetName = BaseWindow::FormatName(assetName, item.editCounter);
            if (ImGui::BeginTabItem(assetName.AsCharPtr(), &open, item.grabFocus ? ImGuiTabItemFlags_SetSelected : 0x0))
            {
                // If item was closed, remove item from 
                if (open)
                {
                    item.grabFocus = false;
                    switch (item.assetType)
                    {
                        case AssetType::Material:
                        {
                            if (save == SaveMode::SaveActive)
                                MaterialSave(this, &item);
                            MaterialEditor(this, &item, save);
                            break;
                        }
                        case AssetType::Mesh:
                        {
                            break;
                        }
                        case AssetType::Skeleton:
                        {
                            break;
                        }
                        case AssetType::Model:
                        {
                            break;
                        }
                        case AssetEditor::AssetType::None:
                            EmptyEditor();
                            break;
                    }
                    ImGui::EndTabItem();
                }
                else
                {
                    previewerState.items.Erase(&item);
                }
            }
        }
        ImGui::EndTabBar();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Setup(AssetEditorItem* item)
{
    item->allocator.Release();
    item->constants = nullptr;
    item->images.Clear();
    switch (item->assetType)
    {
        case AssetEditor::AssetType::Material:
            MaterialSetup(item);
            break;
        default:
            // TODO: implement other editors
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AssetEditor::Open(const Resources::ResourceName& asset, const AssetType type)
{
    // If we try to load the same item, just focus that one
    for (AssetEditorItem& item : previewerState.items)
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
            AssetEditorItem& item = previewerState.items.Emplace();
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
