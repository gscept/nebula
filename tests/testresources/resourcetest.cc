//------------------------------------------------------------------------------
// blockallocatortest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "resourcetest.h"
#include "memory/sliceallocatorpool.h"
#include "timing/timer.h"
#include "io/console.h"
#include "resources/resourcemanager.h"
#include "teststreampool.h"
#include "testresource.h"
#include "io/ioserver.h"
#include "util/stringatom.h"
#include "ids/id.h"
#include "ids/idpool.h"
#include "testmemorypool.h"

using namespace Timing;
using namespace Resources;
namespace Test
{

__ImplementClass(Test::ResourceTest, 'BLKT', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
ResourceTest::Run()
{
    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

    // create test file
    Ptr<IO::Stream> stream = ioServer->CreateStream("home:testresources.txt");
    stream->SetAccessMode(IO::Stream::WriteAccess);
    n_assert(stream->Open());
    Util::String msg = "Hello, this is a test resource file contents!";
    stream->Write(msg.AsCharPtr(), msg.Length());
    stream->Close();

    // create another test file
    stream = ioServer->CreateStream("home:testresources2.txt");
    stream->SetAccessMode(IO::Stream::WriteAccess);
    n_assert(stream->Open());
    msg = "Hello, eh, this is another one!";
    stream->Write(msg.AsCharPtr(), msg.Length());
    stream->Close();
    stream = nullptr;

    // create resource manager and attach a stream (file) loader and a memory loader
    Ptr<ResourceManager> resMgr = ResourceManager::Create();
    resMgr->Open();

    // all resources loaded with "txt" extension will be directed to the TestStreamPool
    // all resources using Reserve and Update will have to use the TestMemoryPool, however
    resMgr->RegisterStreamPool("txt", TestStreamPool::RTTI);
    resMgr->RegisterMemoryPool(TestMemoryPool::RTTI);

    // test getting a pool (this is useful for using the resources internally at some later point)
    // recommended for all memory pools is to have the pointers to them stored somewhere
    TestMemoryPool* loader = resMgr->GetMemoryPool<TestMemoryPool>();

    // reserve a resource and update it from memory, notice how resMgr->ReserveResource must first find the pool
    Resources::ResourceId tempres = resMgr->ReserveResource("", ""_atm, TestMemoryPool::RTTI);
    resMgr->DiscardResource(tempres);

    // this way, we don't need to find the loader 
    // use this for subsystems which may require resource specific interaction, like vertex buffer binding
    tempres = loader->ReserveResource("", ""_atm);
    TestMemoryPool::UpdateInfo info;
    info.buf = "test";
    info.len = strlen(info.buf);
    resMgr->LoadFromMemory(tempres, &info);

    const TestResourceData& testRes = loader->GetResource(tempres);
    resMgr->DiscardResource(tempres);

    // perform a series of asynchronous loads
    bool mayquit = false;
    bool mayquit2 = false;
    Resources::ResourceId res = resMgr->CreateResource("home:testresources.txt", ""_atm,
        [this, &mayquit](const Resources::ResourceId res)
    {
        n_printf("Resource loaded!\n");
        this->Verify(true);
        mayquit = true;
    },
        [this, &mayquit](const Resources::ResourceId res)
    {
        n_printf("Resource failed!\n");
        this->Verify(false);
        mayquit = true;
    });

    // okay, try to load more of the same resource
    auto g = resMgr->CreateResource("home:testresources.txt", ""_atm,
        [this, &mayquit2](const Resources::ResourceId res)
    {
        n_printf("Resource loaded!\n");
        this->Verify(true);
        mayquit2 = true;
    },
        [this, &mayquit2](const Resources::ResourceId res)
    {
        n_printf("Resource failed!\n");
        this->Verify(false);
        mayquit2 = true;
    }); 

    // create the other resource
    auto g2 = resMgr->CreateResource("home:testresources2.txt", ""_atm);
    resMgr->DiscardResource(g2);

    resMgr->DiscardResource(g);
    resMgr->DiscardResource(res);

    IndexT i = 0;
    while (resMgr->HasPendingResources())
    {
        resMgr->Update(i++);
    }
    n_printf("It took %d loops to complete!\n", i);

    // create yet another resource
    resMgr->Close();
    ioServer = nullptr;
}

} // namespace Test