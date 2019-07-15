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
    Ptr<FileWatcher> watcher = FileWatcher::Create();

    ioServer->CreateDirectory("temp");
    
    bool fileAdded = false;
    bool fileModified = false;
    bool fileDeleted = false;
    watcher->Watch("temp", false, WatchFlags(WatchFlags::Creation | WatchFlags::Write), [&fileAdded,&fileModified,&fileDeleted](IO::WatchEvent const& ev)
    {
        switch(ev.type)
        {
        case IO::Created: fileAdded = true; break;
        case IO::Modified: fileModified = true; break;
        case IO::Deleted: fileDeleted = true; break;
        }        
    });
    Ptr<Stream> stream = ioServer->Instance()->CreateStream("temp/test.txt");
    Ptr<TextWriter> writer = TextWriter::Create();
    writer->SetStream(stream);
    writer->Open();
    writer->WriteLine("Testline");
    writer->Close();
    watcher->Update();
    watcher->Update();
    watcher->Update();
    VERIFY(fileAdded == true);
    stream = ioServer->Instance()->CreateStream("temp/test.txt");    
    writer->SetStream(stream);
    writer->Open();
    writer->WriteLine("Testline2");
    writer->Close();
    watcher->Update();
    VERIFY(fileModified == true);
    ioServer->Instance()->DeleteFile("temp/test.txt");
    watcher->Update();
    watcher->Update();
    VERIFY(fileDeleted == true);
    watcher->Unwatch("temp");
    ioServer->Instance()->DeleteDirectory("temp");
}
}