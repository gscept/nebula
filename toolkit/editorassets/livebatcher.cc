//------------------------------------------------------------------------------
//  livebatcher.cc
//  (C) 2025 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/livebatcher.h"
#include "system/process.h"
#include "io/memorystream.h"
#include "io/ioserver.h"
#include "resources/resourceserver.h"
#include "threading/criticalsection.h"
#include "threading/event.h"
#include "threading/safequeue.h"
#include "threading/thread.h"

const char* batcherPath = NEBULA_BINARY_FOLDER "/assetbatcher";
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

            this->curWorkRequests.Reserve(this->jobQueue.Size());
            this->jobQueue.DequeueAll(this->curWorkRequests);
            for (const auto& job : this->curWorkRequests)
            {
                bool res = job.func();
                n_assert(res);

                // Count every job, so LiveBatcher::Wait can tell when the jobs it waits for are done
                this->numCompleted.Increment(1);
                this->completedEvent.Signal();
            }
        }
        // empty
    }

public:
    /// queue a job and count it, so LiveBatcher::Wait knows what it has to wait for
    void Enqueue(LiveBatchJob&& job)
    {
        this->numEnqueued.Increment(1);
        this->jobQueue.Enqueue(std::move(job));
    }

    Util::Array<LiveBatchJob> curWorkRequests;

    Threading::SafeQueue<LiveBatchJob> jobQueue;
    Threading::Interlocked::AtomicInt numEnqueued;
    Threading::Interlocked::AtomicInt numCompleted;
    Threading::Event completedEvent;
};
__ImplementClass(Editor::LiveBatcherThread, 'LiBt', Threading::Thread);


struct
{
    System::ProcessStartInfo startInfo;
    Ptr<IO::MemoryStream> outputStream;
    Ptr<LiveBatcherThread> batchThread;
    Threading::SafeQueue<Util::String> filesToReload;

    /// what the batcher printed, appended by the batching thread and copied by the panel
    Threading::CriticalSection logLock;
    Util::String log;
    Threading::Interlocked::AtomicCounter logVersion;
} livebatcherState;


Threading::Interlocked::AtomicCounter LiveBatcher::WorkCounter;

//------------------------------------------------------------------------------
/**
    The captured output isn't null terminated and stays unallocated if the batcher
    printed nothing. It goes to the engine log and to the batcher's own log, which the panel shows
*/
static void
LogBatcherOutput()
{
    IO::Stream::Size size = livebatcherState.outputStream->GetSize();
    Util::String output;
    if (size > 0)
    {
        n_log(Live Batcher, "%.*s", (int)size, (const char*)livebatcherState.outputStream->GetRawPointer());
        output.Set((const char*)livebatcherState.outputStream->GetRawPointer(), (SizeT)size);
    }
    else
    {
        n_log(Live Batcher, "the batcher printed nothing\n");
        output = "the batcher printed nothing\n";
    }

    Threading::CriticalScope scope(&livebatcherState.logLock);
    livebatcherState.log.Append(output);
    livebatcherState.logVersion.Increment();
}

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
    // The start info holds the stream as well, the process writes into it
    livebatcherState.startInfo.outputStream = nullptr;
    livebatcherState.outputStream = nullptr;
    livebatcherState.batchThread = nullptr;
}

//------------------------------------------------------------------------------
/**
    A resource can only be reloaded on the main thread, so the jobs queue what they exported and
    this reloads it
*/
void
LiveBatcher::Update()
{
    Util::Array<Util::String> filesToReload;
    filesToReload.Reserve(livebatcherState.filesToReload.Size());
    livebatcherState.filesToReload.DequeueAll(filesToReload);
    for (const auto& file : filesToReload)
        Resources::ReloadResource(file);
}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcher::BatchAssets()
{
    WorkCounter.Increment();
    livebatcherState.batchThread->Enqueue(LiveBatchJob{
        []() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            livebatcherState.startInfo.args = args;
            // the process writes from the start of the stream, drop the previous job's output
            livebatcherState.outputStream->SetSize(0);
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                LogBatcherOutput();

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
LiveBatcher::BatchAsset(const IO::Path& assetPath)
{
    WorkCounter.Increment();
    // Through Enqueue, so Wait counts this job as well
    livebatcherState.batchThread->Enqueue(LiveBatchJob{
        [assetPath]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append(" -dir \"" + assetPath.GetFolder() + "\"");
            livebatcherState.startInfo.args = args;
            // the process writes from the start of the stream, drop the previous job's output
            livebatcherState.outputStream->SetSize(0);
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                LogBatcherOutput();

                WorkCounter.Decrement();

                /// Hmm, maybe it'd be better if the batcher could produce a list of files for us instead...
                static const char* exportFolders[] =
                {
                    "mdl",
                    "msh",
                    "tex",
                    "phys",
                    "nav"
                };

                for (SizeT i = 0; i < lengthof(exportFolders); i++)
                {
                    const char* basePath = exportFolders[i];
                    Util::Array<Util::String> files = IO::ListFiles(Util::Format("%s:%s", basePath, assetPath.GetFolder().AsCharPtr()), "*");
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
    WorkCounter.Increment();

    // Loaders look resources up by their resolved path, so the reload needs the full export URI, not just folder/file.ext
    const Util::String exportFile = filePath.ExportURI().AsString();
    // a model asset packages its animation under the model's name, with the clip sections cut from it, which characters
    // set up from the model play
    const Util::String animationFile = filePath.GetType() == "mdl" && IO::Path::ExportExtensions.Contains("ani")
        ? IO::Path::File(filePath.GetFolder(), filePath.GetFile(), "ani").ExportURI().AsString()
        : Util::String();
    livebatcherState.batchThread->Enqueue(LiveBatchJob{
        [filePath, exportFile, animationFile]() -> bool
        {
            Util::String args;
            args.Append("-rawlog ");
            args.Append(" -dir \"" + filePath.GetFolder() + "\"");
            args.Append(" -file \"" + filePath.GetWorkFile() + "\"");
            args.Append(" -force");
            livebatcherState.startInfo.args = args;
            // the process writes from the start of the stream, drop the previous job's output
            livebatcherState.outputStream->SetSize(0);
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                LogBatcherOutput();

                WorkCounter.Decrement();

                livebatcherState.filesToReload.Enqueue(exportFile);
                if (!animationFile.IsEmpty())
                    livebatcherState.filesToReload.Enqueue(animationFile);

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
    livebatcherState.batchThread->Enqueue(LiveBatchJob{
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
            // the process writes from the start of the stream, drop the previous job's output
            livebatcherState.outputStream->SetSize(0);
            System::ProcessId process = System::StartProcess(livebatcherState.startInfo);
            if (process != System::InvalidProcessId)
            {
                System::WaitForProcess(process);
                LogBatcherOutput();

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
    // Wait for every job queued so far. The event can't tell us that on its own: on posix it's a
    // counting semaphore, so it still holds a signal for each earlier job nobody waited for (like saves)
    const Ptr<LiveBatcherThread>& thread = livebatcherState.batchThread;
    const int target = thread->numEnqueued.Add(0);
    while (thread->numCompleted.Add(0) < target)
        thread->completedEvent.Wait();
}

//------------------------------------------------------------------------------
/**
*/
bool
LiveBatcher::IsBusy()
{
    return WorkCounter.counter > 0;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
LiveBatcher::GetLog()
{
    Threading::CriticalScope scope(&livebatcherState.logLock);
    return livebatcherState.log;
}

//------------------------------------------------------------------------------
/**
*/
uint
LiveBatcher::GetLogVersion()
{
    return (uint)livebatcherState.logVersion.counter;
}

//------------------------------------------------------------------------------
/**
*/
void
LiveBatcher::ClearLog()
{
    Threading::CriticalScope scope(&livebatcherState.logLock);
    livebatcherState.log.Clear();
    livebatcherState.logVersion.Increment();
}

} // namespace Editor
