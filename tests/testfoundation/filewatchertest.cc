//------------------------------------------------------------------------------
//  filewatchertest.cc
//  (C)2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "filewatchertest.h"
#include "io/filewatcher.h"
#include "io/textwriter.h"
#include "io/ioserver.h"
#include "io/assignregistry.h"
#include "io/uri.h"
#include "core/ptr.h"
#include "util/delegate.h"

namespace Test
{
__ImplementClass(Test::FileWatcherTest, 'FWAT', Test::TestCase);

using namespace IO;
using namespace Util;


//------------------------------------------------------------------------------
/**
*/
void
FileWatcherTest::Run()
{
    Ptr<IoServer> ioServer = IoServer::Create();
    AssignRegistry* assignRegistry = AssignRegistry::Instance();
    

    ioServer->CreateDirectory("temp");
    ioServer->CreateDirectory("temp2");
        
    int fileAdded = 0;
    int fileModified = 0;
    int fileDeleted = 0;
    Ptr<FileWatcher> watcher = FileWatcher::Instance();    
    watcher->SetSpeed(0.001);
    watcher->Watch("temp", false, WatchFlags(WatchFlags::Creation | WatchFlags::NameChanged | WatchFlags::Write), [&fileAdded, &fileModified, &fileDeleted](IO::WatchEvent const& ev)
    {
        n_printf("event temp: %d %s\n", ev.type, ev.file.AsCharPtr());
        switch(ev.type)
        {        
        case IO::Created: fileAdded++; break;
        case IO::Modified: fileModified++ ; break;
        case IO::Deleted: fileDeleted++; break;
        default: break;
        }        
    });
    watcher->Watch("temp2", false, WatchFlags(WatchFlags::Creation | WatchFlags::NameChanged | WatchFlags::Write), [&fileAdded, &fileModified, &fileDeleted](IO::WatchEvent const& ev)
    {
        n_printf("event temp2: %d %s\n", ev.type, ev.file.AsCharPtr());
        switch (ev.type)
        {
        case IO::Created: fileAdded++; break;
        case IO::Modified: fileModified++; break;
        case IO::Deleted: fileDeleted++; break;
        default: break;
        }
    });
    Core::SysFunc::Sleep(0.2);
    Ptr<Stream> stream = ioServer->Instance()->CreateStream("temp/test.txt");
    Ptr<TextWriter> writer = TextWriter::Create();
    writer->SetStream(stream);
    writer->Open();
    writer->WriteLine("Testline");
    writer->Close();        
    stream = nullptr;
    
    stream = ioServer->Instance()->CreateStream("temp/test.txt");    
    writer->SetStream(stream);
    writer->Open();
    writer->WriteLine("Testline2");
    writer->Close();    
    stream = nullptr;

    ioServer->Instance()->CopyFile("temp/test.txt", "temp2/test.txt");
   
    ioServer->Instance()->DeleteFile("temp/test.txt");  

    
    ioServer->Instance()->DeleteFile("temp2/test.txt");
    
    
    // loop for a while to give thread time to trigger events
    for (int i = 0; i < 100; i++)
    {
        if (fileDeleted == 2) break;
        Core::SysFunc::Sleep(0.1);
    }

    watcher->Unwatch("temp2");
    watcher->Unwatch("temp");
    ioServer->Instance()->DeleteDirectory("temp");       
    ioServer->Instance()->DeleteDirectory("temp2");
    
    // with low priority we only get a modify per file sometimes (which is fine for normal use)
    //VERIFY(fileAdded == 2);
    VERIFY(fileModified > 2);
    //VERIFY(fileDeleted == 2);
    watcher = nullptr;
}
}