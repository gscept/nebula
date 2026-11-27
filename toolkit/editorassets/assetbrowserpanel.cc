//------------------------------------------------------------------------------
//  @file assetbrowserpanel.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetbrowserpanel.h"
#include "editorassets/assetbrowser.h"
#include "neweditor/api/shell.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::AssetBrowserPanel, 'ASBP', Editor::EditorPanel);

AssetBrowserPanel* AssetBrowserPanel::instance = nullptr;

//------------------------------------------------------------------------------
/**
*/
AssetBrowserPanel::AssetBrowserPanel()
{
    this->name = "assets";
    AssetBrowserPanel::instance = this;
}

//------------------------------------------------------------------------------
/**
*/
AssetBrowserPanel::~AssetBrowserPanel()
{
    if (AssetBrowserPanel::instance == this)
        AssetBrowserPanel::instance = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowserPanel::GetSelectedFolder(IO::Path& outFolder)
{
    AssetBrowserPanel* panel = AssetBrowserPanel::instance;
    return panel != nullptr && panel->browser.isvalid() && panel->browser->GetSelectedFolder(outFolder);
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowserPanel::OnSetup()
{
    this->browser = AssetBrowser::Create();
    this->browser->Setup(this->core);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetBrowserPanel::Draw()
{
    this->browser->Draw();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetBrowserPanel::OnEditAction(EditAction action)
{
    switch (action)
    {
        case EditAction::Rename:
            if (!this->browser->BeginRename())
                this->SetStatus("Select a folder or a file to rename", 3.0f);
            return true;
        case EditAction::Copy:
        {
            const IO::Path asset = this->browser->GetSelectedAsset();
            if (!asset.IsFile() || asset.GetType().IsEmpty())
            {
                this->SetStatus("Select an asset to copy its URN", 3.0f);
                return true;
            }
            const Util::String urn = asset.AsString();
            ImGui::SetClipboardText(urn.AsCharPtr());
            this->SetStatus(Util::Format("Copied %s", urn.AsCharPtr()), 3.0f);
            return true;
        }
        case EditAction::Delete:
        case EditAction::Duplicate:
        case EditAction::Paste:
            // taken so it does not act on the scene, which is not what is being looked at
            this->SetStatus("The asset browser can't do that yet", 3.0f);
            return true;
        case EditAction::Save:
        default:
            // the browser writes every change as it is made, there is nothing to save
            return false;
    }
}

} // namespace Editor
