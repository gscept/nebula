//------------------------------------------------------------------------------
//  livebatcher.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "livebatcher.h"
#include "system/process.h"
#include "io/memorystream.h"

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

    bool autoScroll = true;
} livebatcherState;



//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcher::Setup()
{
    livebatcherState.startInfo.workingDir = workPath;
    livebatcherState.startInfo.outputStream = livebatcherState.outputStream.upcast<IO::Stream>();
    livebatcherState.startInfo.consoleWindow = false;
    livebatcherState.startInfo.exePath = Util::String::Sprintf(batcherPath, System::PlatformTypeAsString(System::Platform));
    livebatcherState.outputStream = IO::MemoryStream::Create();

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
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        []() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
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
LiveBatcher::BatchAsset(const IO::URI& assetPath)
{
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [assetPath]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append("-asset " + assetPath.LocalPath());
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
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
LiveBatcher::BatchFile(const IO::URI& filePath)
{
    livebatcherState.batchThread->jobQueue.Enqueue(LiveBatchJob{
        [filePath]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append(" -dir " + filePath.LocalPath().ExtractDirName());
            args.Append(" -file " + filePath.LocalPath().ExtractFileName());
            livebatcherState.startInfo.args = args;
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
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

__ImplementClass(Presentation::LiveBatcherWindow, 'LiBw', Presentation::BaseWindow)

//------------------------------------------------------------------------------
/**
*/
void 
LiveBatcherWindow::Run(SaveMode save)
{
    if (Editor::livebatcherState.outputStream->GetSize() > 0)
    {
        ImGui::Checkbox("Auto-scroll", &Editor::livebatcherState.autoScroll);
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            Editor::livebatcherState.outputStream->SetSize(0);
        }

        ImGui::Separator();

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
}

} // namespace Presentation
