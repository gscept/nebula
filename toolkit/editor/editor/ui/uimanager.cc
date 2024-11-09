//------------------------------------------------------------------------------
//  uimanager.cc
//  (C) 2019-2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "uimanager.h"
#include "graphicsfeature/graphicsfeatureunit.h"
#include "windowserver.h"
#include "windows/console.h"
#include "windows/outline.h"
#include "windows/styleeditor.h"
#include "windows/toolbar.h"
#include "windows/scene.h"
#include "windows/history.h"
#include "windows/environment.h"
#include "windows/inspector.h"
#include "windows/assetbrowser.h"
#include "windows/asseteditor/asseteditor.h"
#include "windows/resourcebrowser.h"
#include "windows/physics.h"
#include "windows/profiler.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "editor/commandmanager.h"
#include "dynui/imguicontext.h"
#include "io/filedialog.h"
#include "window.h"
#include "editor/tools/selectiontool.h"
#include "editor/cmds.h"

#include "frame/default.h"
#include "frame/editorframe.h"

namespace Editor
{

static Ptr<Presentation::WindowServer> windowServer;

namespace UIManager
{

namespace Icons
{
    texturehandle_t play;
    texturehandle_t pause;
    texturehandle_t stop;
    texturehandle_t game;
    texturehandle_t environment;
    texturehandle_t light;
}

Icons::texturehandle_t NLoadIcon(const char* resource)
{
    return Resources::CreateResource(resource, "EditorIcons"_atm, nullptr, nullptr, true).HashCode64();
}

//------------------------------------------------------------------------------
/**
*/
void
OnActivate()
{
    windowServer = Presentation::WindowServer::Create();

    windowServer->RegisterWindow("Presentation::Console", "Console", "Debug");
    windowServer->RegisterWindow("Presentation::Outline", "Outline");
    windowServer->RegisterWindow("Presentation::History", "History", "Editor");
    windowServer->RegisterWindow("Presentation::StyleEditor", "Style Editor", "Editor");
    windowServer->RegisterWindow("Presentation::Toolbar", "Toolbar");
    windowServer->RegisterWindow("Presentation::Environment", "Environment");
    windowServer->RegisterWindow("Presentation::Scene", "Scene View");
    windowServer->RegisterWindow("Presentation::Inspector", "Inspector");
    windowServer->RegisterWindow("Presentation::AssetBrowser", "Asset Browser");
    windowServer->RegisterWindow("Presentation::AssetEditor", "Asset Editor", "Editor");
    windowServer->RegisterWindow("Presentation::ResourceBrowser", "Resource Browser");
    windowServer->RegisterWindow("Presentation::Profiler", "Profiler");
    windowServer->RegisterWindow("Presentation::Physics", "Physics");
    
    Icons::play          = NLoadIcon("systex:icon_play.dds");
    Icons::pause         = NLoadIcon("systex:icon_pause.dds");
    Icons::stop          = NLoadIcon("systex:icon_stop.dds");
    Icons::environment   = NLoadIcon("systex:icon_environment.dds");
    Icons::game          = NLoadIcon("systex:icon_game.dds");
    Icons::light         = NLoadIcon("systex:icon_light.dds");
    
    windowServer->RegisterCommand([](){ Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveActive); }, "Save", "Ctrl+S", "Edit");
    windowServer->RegisterCommand([](){ Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveAll); }, "Save All", "Ctrl+Shift+S", "Edit");
    windowServer->RegisterCommand([](){ Edit::CommandManager::Undo(); }, "Undo", "Ctrl+Z", "Edit");
    windowServer->RegisterCommand([](){ Edit::CommandManager::Redo(); }, "Redo", "Ctrl+Shift+Z", "Edit");
    windowServer->RegisterCommand([]()
    {
        auto selection = Tools::SelectionTool::Selection();
        Edit::CommandManager::BeginMacro("Delete entities", false);
        Util::Array<Editor::Entity> emptySelection;
        Edit::SetSelection(emptySelection);
        for (auto e : selection)
        {
            Edit::DeleteEntity(e);
        }
        Edit::CommandManager::EndMacro();
    }, "Delete", "Delete", "Edit");
    windowServer->RegisterCommand([](){ 
        static Util::String localpath = IO::URI("export:levels").LocalPath();
        Util::String path;
        IO::IoServer::Instance()->CreateDirectory(localpath);
        if (IO::FileDialog::SaveFile("Select location of exported level file", localpath, {"*.nlvl"}, path))
            Editor::state.editorWorld->ExportLevel(path.AsCharPtr());
    }, "Export", "Ctrl+Shift+E", "File");

    Graphics::GraphicsServer::Instance()->AddPostViewCall([](IndexT frameIndex, IndexT bufferIndex)
    {
        ImGui::DockSpaceOverViewport();
        windowServer->RunAll();
        FrameScript_editorframe::Bind_Scene(FrameScript_default::Submission_Scene);
        FrameScript_editorframe::Bind_SceneBuffer(Frame::TextureImport::FromExport(FrameScript_default::Export_ColorBuffer));
        CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(CoreGraphics::CurrentWindow);
        Math::rectangle<int> viewport(0, 0, mode.GetWidth(), mode.GetHeight());
        FrameScript_editorframe::Run(viewport, frameIndex, bufferIndex);

        Graphics::GraphicsServer::SwapInfo swapInfo;
        swapInfo.syncFunc = [](CoreGraphics::CmdBufferId cmdBuf)
    {
            FrameScript_editorframe::Synchronize("Present_Sync", cmdBuf, { { (FrameScript_editorframe::TextureIndex)FrameScript_editorframe::Export_EditorBuffer.index, CoreGraphics::PipelineStage::TransferRead } }, nullptr);
        };
        swapInfo.submission = FrameScript_editorframe::Submission_EditorUI;
        swapInfo.swapSource = FrameScript_editorframe::Export_EditorBuffer.tex;
        Graphics::GraphicsServer::Instance()->SetSwapInfo(swapInfo);
    });
}

//------------------------------------------------------------------------------
/**
*/
void
OnDeactivate()
{
    windowServer = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
OnBeginFrame()
{
    windowServer->Update();
}

//------------------------------------------------------------------------------
/**
*/
Game::ManagerAPI
Create()
{
    Game::ManagerAPI api;
    api.OnActivate = &OnActivate;
    api.OnDeactivate = &OnDeactivate;
    api.OnBeginFrame = &OnBeginFrame;
    return api;
}

} // namespace UIManager

} // namespace Editor
