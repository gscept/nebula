//------------------------------------------------------------------------------
//  livebatcher.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "livebatcher.h"
#include "system/process.h"
#include "io/memorystream.h"
#include "io/ioserver.h"
#include "dynui/imguicontext.h"
#include "dynui/nebula_icons.h"
#include "editor/ui/windowserver.h"

const char* batcherPath = NEBULA_BINARY_FOLDER "/assetbatcher.exe";
const char* workPath = "proj:work/";
namespace Editor
{

struct LiveBatchJob
{
    std::function<bool()> func;
};

class LiveBatcherThread : public Threading::Thread
{
    __DeclareClass(LiveBatcherThread);
    void DoWork() override
    {
        while (!this->ThreadStopRequested())
        {
            this->jobQueue.Wait();
            this->waitEvent.Reset();

            this->jobQueue.DequeueAll(this->curWorkRequests);
            for (const auto& job : this->curWorkRequests)
            {
                bool res = job.func();
                n_assert(res);
            }

            this->waitEvent.Signal();
        }
        // empty
    }

public:
    Util::Array<LiveBatchJob> curWorkRequests;

    Threading::SafeQueue<LiveBatchJob> jobQueue;
    Threading::Event waitEvent;
};
__ImplementClass(Editor::LiveBatcherThread, 'LiBt', Threading::Thread);


struct
{
    System::ProcessStartInfo startInfo;
    Ptr<IO::MemoryStream> outputStream;
    Ptr<LiveBatcherThread> batchThread;
    Threading::SafeQueue<Util::String> filesToReload;


    bool autoScroll = true;
} livebatcherState;


Threading::Interlocked::AtomicCounter LiveBatcher::WorkCounter;
//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcher::Setup()
{
    livebatcherState.outputStream = IO::MemoryStream::Create();
    livebatcherState.startInfo.workingDir = workPath;
    livebatcherState.startInfo.outputStream = livebatcherState.outputStream.upcast<IO::Stream>();
    livebatcherState.startInfo.consoleWindow = false;
    livebatcherState.startInfo.exePath = batcherPath;

    livebatcherState.batchThread = LiveBatcherThread::Create();
    livebatcherState.batchThread->Start();
}

//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcher::Discard()
{
    livebatcherState.batchThread->jobQueue.Signal();
    livebatcherState.batchThread->Stop();
    livebatcherState.outputStream = nullptr;
    livebatcherState.batchThread = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcher::BatchAssets()
{
    Presentation::BatcherWindow->Open() = true;
    Presentation::WindowServer* windowServer = Presentation::WindowServer::Instance();
    uint32_t jobMessage = windowServer->PushLoadingMessage("[Packaging] All assets");
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [windowServer, jobMessage]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);

                windowServer->RemoveLoadingMessage(jobMessage);
                return true;
            }
            return false;
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcher::BatchAsset(const IO::Path& assetPath)
{
    Presentation::BatcherWindow->Open() = true;
    IO::IoServer* ioServer = IO::IoServer::Instance();
    WorkCounter.Increment();
    Presentation::WindowServer* windowServer = Presentation::WindowServer::Instance();
    uint32_t jobMessage = windowServer->PushLoadingMessage(Util::Format("[Packaging] %s", assetPath.AsString().AsCharPtr()));
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [assetPath, ioServer, jobMessage, windowServer]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append(" -dir \"" + assetPath.GetFolder() + "\"");
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                n_log(Live Batcher, "%s", (char*)livebatcherState.outputStream->GetRawPointer());

                WorkCounter.Decrement();
                windowServer->RemoveLoadingMessage(jobMessage);

                /// Hmm, maybe it'd be better if the batcher could produce a list of files for us instead...
                static const char* exportFolders[] =
                {
                    "mdl",
                    "msh",
                    "tex",
                    "phys"
                };

                for (SizeT i = 0; i < lengthof(exportFolders); i++)
                {
                    const char* basePath = exportFolders[i];
                    Util::Array<Util::String> files = ioServer->ListFiles(Util::Format("%s:%s", basePath, assetPath.GetFolder().AsCharPtr()), "*");
                    for (const auto& file : files)
                    {
                        livebatcherState.filesToReload.Enqueue(Util::Format("%s:%s/%s", basePath, assetPath.GetFolder().AsCharPtr(), file.AsCharPtr()));
                    }
                }
                

                return true;
            }

            return false;
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcher::BatchFile(const IO::Path& filePath)
{
    Presentation::BatcherWindow->Open() = true;
    WorkCounter.Increment();
    Presentation::WindowServer* windowServer = Presentation::WindowServer::Instance();
    uint32_t jobMessage = windowServer->PushLoadingMessage(Util::Format("[Packaging] %s", filePath.AsString().AsCharPtr()));
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [filePath, jobMessage, windowServer]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append(" -dir \"" + filePath.GetFolder() + "\"");
            args.Append(" -file \"" + filePath.GetWorkFile() + "\"");
            args.Append(" -force");
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                n_log(Live Batcher, "%s", (char*)livebatcherState.outputStream->GetRawPointer());

                WorkCounter.Decrement();
                windowServer->RemoveLoadingMessage(jobMessage);

                livebatcherState.filesToReload.Enqueue(filePath.GetExportFile());

                return true;
            }
            return false;
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcher::BatchModes(Editor::BatchModes modes)
{
    WorkCounter.Increment();
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [modes]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            uint32_t bits = modes;
            uint32_t index = 0;
            while (bits != 0x0)
            {
                if ((bits >> index) & 0x1)
                {
                    switch (1 << index)
                    {
                    case Editor::Meshes:
                        args.Append("-mode fbx");
                        break;
                    case Editor::Models:
                        args.Append("-mode model");
                        break;
                    case Editor::Textures:
                        args.Append("-mode texture");
                        break;
                    default:
                        break;
                    }
                    bits &= ~(1 << index);
                }
                index++;
            }
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);

                WorkCounter.Decrement();
                return true;
            }
            return false;
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcher::Wait()
{
    livebatcherState.batchThread->waitEvent.Wait();
}

} // namespace Editor

namespace Presentation
{


//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcherWindow::Run(SaveMode save)
{

    ImGui::Checkbox("Auto-scroll", &Editor::livebatcherState.autoScroll);
    if (Editor::LiveBatcher::WorkCounter.counter > 0)
    {
        const char* message = "Running batcher...";
        const float diameter = 20.0f;
        const float groupWidth = ImGui::CalcTextSize(message).x + ImGui::GetStyle().ItemSpacing.x + diameter;
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - groupWidth);
        ImGui::TextUnformatted(message);
        ImGui::SameLine();
        Dynui::ImGuiSpinner("Running batcher...");
    }
    ImGui::Separator();
    if (ImGui::Button(ICON_ttf_TRASH))
    {
        Editor::livebatcherState.outputStream->SetSize(0);
    }

    if (Editor::livebatcherState.outputStream->GetSize() > 0)
    {
        if (ImGui::BeginChild("Log", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::TextUnformatted((const char*)Editor::livebatcherState.outputStream->GetRawPointer());

            if (Editor::livebatcherState.autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }
    }
    else
    {
        ImGuiStyle& style = ImGui::GetStyle();

        static const char* EmptyString = "Log empty";
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

}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcherWindow::Update()
{
    // Dequeue pending reloads and run them
    Util::Array<Util::String> filesToReload;
    Editor::livebatcherState.filesToReload.DequeueAll(filesToReload);
    for (const auto& file : filesToReload)
        Resources::ReloadResource(file);
}

} // namespace Presentation
