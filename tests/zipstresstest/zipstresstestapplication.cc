//------------------------------------------------------------------------------
//  zipstresstestapplication.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "zipstresstestapplication.h"
#include "threading/thread.h"
#include "io/stream.h"

namespace App
{
using namespace Util;
using namespace IO;
using namespace Threading;

// thread subclass for file reading
class ReaderThread : public Threading::Thread
{
    __DeclareClass(ReaderThread);
public:
    /// constructor
    ReaderThread() : loopCount(0) {};
    /// setup the directory and file pattern
    void Setup(SizeT loopCount_, const String& path_, const String& pattern_)
    {
        this->loopCount = loopCount_;
        this->path = path_;
        this->pattern = pattern_;
    };

protected:
    /// worker method
    virtual void DoWork();

private:
    SizeT loopCount;
    String path;
    String pattern;
};
__ImplementClass(App::ReaderThread, 'RTHR', Threading::Thread);

//------------------------------------------------------------------------------
/**
*/
void
ReaderThread::DoWork()
{
    // create an IoServer and console for this thread
    Ptr<IoServer> ioServer = IoServer::Create();

    IndexT loopIndex;
    for (loopIndex = 0; loopIndex < this->loopCount; loopIndex++)
    {
        // list files for reading
        Array<String> files = ioServer->ListFiles(this->path, this->pattern);
        IndexT i;
        for (i = 0; i < files.Size(); i++)
        {
            URI fileUri(this->path + "/" + files[i]);
            n_printf("%s: %s\n", Thread::GetMyThreadName(), fileUri.AsString().AsCharPtr());
            Ptr<Stream> stream = ioServer->CreateStream(fileUri);
            if (stream->Open())
            {
                SizeT fileSize = stream->GetSize();
                void* buf = Memory::Alloc(Memory::DefaultHeap, fileSize);
                n_assert(buf);
                SizeT readSize = stream->Read(buf, fileSize);
                n_assert(readSize == fileSize);
                Memory::Free(Memory::DefaultHeap, buf);
                stream->Close();
            }
        }
    }

    // shutdown the thread
}

//------------------------------------------------------------------------------
/**
*/
void
ZipStressTestApplication::Run()
{
    // mount standard zip archives
    IoServer::Instance()->MountStandardArchives();
    const SizeT numThreads = 8;

    // create reader threads
    Array<Ptr<ReaderThread>> threads;
    IndexT i;
    for (i = 0; i < numThreads; i++)
    {
        Ptr<ReaderThread> newThread = (ReaderThread*) ReaderThread::Create();
        threads.Append(newThread);
        String threadName;
        threadName.Format("ReaderThread%d", i);
        threads[i]->SetName(threadName);
    }
    threads[0]->Setup(100, "tex:characters", "*.dds");
    threads[1]->Setup(100, "tex:examples", "*.dds");
    threads[2]->Setup(100, "tex:ground", "*.dds");
    threads[3]->Setup(100, "tex:layered", "*.dds");
    threads[4]->Setup(100, "tex:lighting", "*.dds");
    threads[5]->Setup(100, "tex:materials", "*.dds");
    threads[6]->Setup(100, "tex:mlpaintmaps", "*.dds");
    threads[7]->Setup(100, "tex:system", "*.dds");

    // start threads
    for (i = 0; i < numThreads; i++)
    {
        threads[i]->Start();
    }

    // wait for threads to finish
    bool anyRunning;
    do
    {
        anyRunning = false;
        for (i = 0; i < numThreads; i++)
        {
            if (threads[i]->IsRunning())
            {
                anyRunning = true;
                break;
            }
        }
        n_sleep(0.01);
    }
    while (anyRunning);

    n_printf("DONE.\n");
}

} // namespace App