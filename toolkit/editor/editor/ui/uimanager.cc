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
#include "windows/navigation.h"
#include "windows/settings.h"
#include "windows/profiler.h"
#include "windows/terraineditor/terraineditor.h"
#include "coregraphics/texture.h"
#include "resources/resourceserver.h"
#include "editor/commandmanager.h"
#include "dynui/imguicontext.h"
#include "io/filedialog.h"
#include "io/filestream.h"
#include "window.h"
#include "editor/tools/selectioncontext.h"
#include "editor/cmds.h"
#include "coregraphics/swapchain.h"

#include "frame/default.h"
#include "frame/editorframe.h"

#include "basegamefeature/level.h"

namespace Editor
{

__ImplementClass(Editor::UIManager, 'UiMa', Game::Manager);

static Ptr<Presentation::WindowServer> windowServer;
const char* UIManager::editorUIPath = "user:nebula/editor/editorui.ini";

namespace UI
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

}
//------------------------------------------------------------------------------
/**
*/
UI::Icons::texturehandle_t NLoadIcon(const char* resource)
{
    return Resources::CreateResource(resource, "EditorIcons"_atm, nullptr, nullptr, true).HashCode64();
}

//------------------------------------------------------------------------------
/**
*/
UIManager::UIManager()
{ 
    // empty
}

//------------------------------------------------------------------------------
/**
*/
UIManager::~UIManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
UIManager::OnActivate()
{
    Game::Manager::OnActivate();

    windowServer = Presentation::WindowServer::Create();

    Tools::SelectionContext::Create();

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
    windowServer->RegisterWindow("Presentation::Navigation", "Navigation");
    windowServer->RegisterWindow("Presentation::Settings", "Settings", "Editor");
    windowServer->RegisterWindow("Presentation::TerrainEditor", "Terrain", "Editor");
    windowServer->RegisterWindow("Presentation::LiveBatcherWindow", "Live Batcher", "Editor");

    UI::Icons::play          = NLoadIcon("systex:icon_play.dds");
    UI::Icons::pause         = NLoadIcon("systex:icon_pause.dds");
    UI::Icons::stop          = NLoadIcon("systex:icon_stop.dds");
    UI::Icons::environment   = NLoadIcon("systex:icon_environment.dds");
    UI::Icons::game          = NLoadIcon("systex:icon_game.dds");
    UI::Icons::light         = NLoadIcon("systex:icon_light.dds");
    
    windowServer->RegisterCommand([](){ Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveActive); }, "Save", "Ctrl+S", "Edit");
    windowServer->RegisterCommand([](){ Presentation::WindowServer::Instance()->BroadcastSave(Presentation::BaseWindow::SaveMode::SaveAll); }, "Save All", "Ctrl+Shift+S", "Edit");
    windowServer->RegisterCommand([](){ Edit::CommandManager::Undo(); }, "Undo", "Ctrl+Z", "Edit");
    windowServer->RegisterCommand([](){ Edit::CommandManager::Redo(); }, "Redo", "Ctrl+Shift+Z", "Edit");
    
    windowServer->RegisterCommand([]()
    {
        auto selection = Tools::SelectionContext::Selection();
        Edit::CommandManager::BeginMacro("Delete entities", false);
        Util::Array<Editor::Entity> emptySelection;
        Edit::SetSelection(emptySelection);
        for (auto e : selection)
        {
            Edit::DeleteEntity(e);
        }
        Edit::CommandManager::EndMacro();
    }, "Delete", "Delete", "Edit");
    
    // Import and export is temporary and should be removed later.
    windowServer->RegisterCommand([](){ 
        static Util::String localpath = IO::URI("export:levels").LocalPath();
        Util::String path;
        IO::IoServer::Instance()->CreateDirectory(localpath);
        if (IO::FileDialog::SaveFile("Select location of exported level file", localpath, {"*.nlvl"}, path))
            Editor::state.editorWorld->ExportLevel(path.AsCharPtr());
    }, "Export nlvl", "Ctrl+Shift+E", "File");
    
    windowServer->RegisterCommand([](){ 
        static Util::String localpath = IO::URI("export:levels").LocalPath();
        Util::String path;
        if (IO::FileDialog::OpenFile("Select Nebula Level", localpath, {"*.nlvl"}, path))
        {
            auto gameWorld = Game::GetWorld(WORLD_DEFAULT);
            Game::PackedLevel* pLevel = gameWorld->PreloadLevel(path);
            auto ents = pLevel->Instantiate();
            gameWorld->UnloadLevel(pLevel);
        }
    }, "Import nlvl (game only)", "Ctrl+Shift+I", "File");

    //
    Graphics::GraphicsServer::Instance()->AddPostViewCall([](IndexT frameIndex, IndexT bufferIndex)
    {
        FrameScript_editorframe::Bind_Scene(FrameScript_default::Submission_Scene);
        FrameScript_editorframe::Bind_SceneBuffer(Frame::TextureImport::FromExport(FrameScript_default::Export_ColorBuffer));

        const auto& windows = Graphics::GraphicsServer::Instance()->GetWindows();
        for (const auto& window : windows)
        {
            CoreGraphics::DisplayMode mode = CoreGraphics::WindowGetDisplayMode(window);
            CoreGraphics::SwapchainId swapchain = CoreGraphics::WindowGetSwapchain(window);

            CoreGraphics::UpdatingWindow = window;

            Math::rectangle<int> viewport(0, 0, mode.GetWidth(), mode.GetHeight());
            FrameScript_editorframe::Run(viewport, frameIndex, bufferIndex);

            CoreGraphics::SwapchainSwap(swapchain);
            CoreGraphics::QueueType queue = CoreGraphics::SwapchainGetQueueType(swapchain);

            // Allocate command buffer to run swap
            CoreGraphics::CmdBufferId cmdBuf = CoreGraphics::SwapchainAllocateCmds(swapchain);
            CoreGraphics::CmdBufferBeginInfo beginInfo;
            beginInfo.submitDuringPass = false;
            beginInfo.resubmittable = false;
            beginInfo.submitOnce = true;
            CoreGraphics::CmdBeginRecord(cmdBuf, beginInfo);
            CoreGraphics::CmdBeginMarker(cmdBuf, NEBULA_MARKER_TURQOISE, "Swap");

            FrameScript_editorframe::Synchronize("Present_Sync", cmdBuf, CoreGraphics::GraphicsQueueType, { { (FrameScript_editorframe::TextureIndex)FrameScript_editorframe::Export_EditorBuffer.index, CoreGraphics::PipelineStage::TransferRead } }, nullptr);
            CoreGraphics::SwapchainCopy(swapchain, cmdBuf, FrameScript_editorframe::Export_EditorBuffer.tex);

            CoreGraphics::CmdEndMarker(cmdBuf);
            CoreGraphics::CmdFinishQueries(cmdBuf);
            CoreGraphics::CmdEndRecord(cmdBuf);
            auto submission = CoreGraphics::SubmitCommandBuffers(
                { cmdBuf }
                , queue
                , { FrameScript_editorframe::Submission_EditorUI }
    #if NEBULA_GRAPHICS_DEBUG
                , "Swap"
    #endif

            );
            CoreGraphics::DeferredDestroyCmdBuffer(cmdBuf);

        }
    });
    IO::URI userEditorIni = IO::URI(editorUIPath);
    Util::String path = userEditorIni.LocalPath();
    if (!IO::IoServer::Instance()->FileExists(userEditorIni))
    {
        const Util::String defaultIni = "tool:syswork/data/editor/defaultui.ini";
        IO::IoServer::Instance()->CreateDirectory("user:nebula/editor/");
        if (IO::IoServer::Instance()->FileExists(defaultIni))
        {
            IO::IoServer::Instance()->CopyFile(defaultIni, userEditorIni);
        }
        else
        {
            // Fallback

            ImGui::SaveIniSettingsToDisk(path.c_str());
        }
    }
   
}

//------------------------------------------------------------------------------
/**
*/
void
UIManager::OnDeactivate()
{
    Game::Manager::OnDeactivate();
    windowServer = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
UIManager::OnBeginFrame()
{
    windowServer->Update();
    ImGui::DockSpaceOverViewport();
    windowServer->RunAll();
}

//------------------------------------------------------------------------------
/**
*/
void
UIManager::OnFrame()
{
    if (this->delayedImguiLoad)
    {
        this->delayedImguiLoad = false;
        IO::URI userEditorIni = IO::URI(editorUIPath);
        Util::String path = userEditorIni.LocalPath();
        if (IO::IoServer::Instance()->FileExists(userEditorIni))
        {
            ImGui::LoadIniSettingsFromDisk(path.c_str());
        }
    }

}
//------------------------------------------------------------------------------
/**
*/
const Util::String 
UIManager::GetEditorUIIniPath()
{
    return editorUIPath;
}

} // namespace Editor
