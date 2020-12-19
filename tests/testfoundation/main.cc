//------------------------------------------------------------------------------
//  testfoundation/main.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "io/gamecontentserver.h"
#include "testbase/testrunner.h"
#include "stringtest.h"
#include "arraytest.h"
#include "arrayallocatortest.h"
#include "stacktest.h"
#include "listtest.h"
#include "dictionarytest.h"
#include "fixedarraytest.h"
#include "fixedtabletest.h"
#include "hashtabletest.h"
#include "queuetest.h"
#include "arrayqueuetest.h"
#include "memorystreamtest.h"
#include "guidtest.h"
#include "fileservertest.h"
#include "filewatchertest.h"
#include "uritest.h"
#include "textreaderwritertest.h"
#include "messagereaderwritertest.h"
#include "xmlreaderwritertest.h"
// #include "jsonreaderwritertest.h"
#include "binaryreaderwritertest.h"
#include "uritest.h"
#include "mediatypetest.h"
#include "varianttest.h"
#include "iointerfacetest.h"
#include "cmdlineargstest.h"
#include "streamservertest.h"
#include "luaservertest.h"
#include "zipfstest.h"
#include "float4test.h"
#include "matrix44test.h"
#include "threadtest.h"
#include "memorypooltest.h"
#include "runlengthcodectest.h"
#include "ringbuffertest.h"
#include "excelxmlreadertest.h"
#include "delegatetest.h"
#include "delegatetabletest.h"
#include "httpclienttest.h"
#include "bxmlreadertest.h"
#include "blobtest.h"
#include "profilingtest.h"
#include "bitfieldtest.h"

using namespace Core;
using namespace Test;

int
__cdecl main()
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Foundation Tests"));
    coreServer->Open();

    Ptr<IO::GameContentServer> gameContentServer = IO::GameContentServer::Create();
    gameContentServer->SetTitle("RL Test Title");
    gameContentServer->SetTitleId("RLTITLEID");
    gameContentServer->SetVersion("1.00");
    gameContentServer->Setup();

    n_printf("NEBULA FOUNDATION TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    //testRunner->AttachTestCase(BXmlReaderTest::Create());
    testRunner->AttachTestCase(HttpClientTest::Create());    
    testRunner->AttachTestCase(DelegateTableTest::Create());
    testRunner->AttachTestCase(DelegateTest::Create());
    testRunner->AttachTestCase(BlobTest::Create());
    testRunner->AttachTestCase(BitFieldTest::Create());
    //testRunner->AttachTestCase(ExcelXmlReaderTest::Create());
    testRunner->AttachTestCase(RingBufferTest::Create());
    testRunner->AttachTestCase(RunLengthCodecTest::Create());
    testRunner->AttachTestCase(MemoryPoolTest::Create());
    testRunner->AttachTestCase(Matrix44Test::Create());
    testRunner->AttachTestCase(Float4Test::Create());
    testRunner->AttachTestCase(ZipFSTest::Create());
    //testRunner->AttachTestCase(FileWatcherTest::Create());
    testRunner->AttachTestCase(LuaServerTest::Create());
    testRunner->AttachTestCase(StreamServerTest::Create());
    testRunner->AttachTestCase(CmdLineArgsTest::Create());
    testRunner->AttachTestCase(MediaTypeTest::Create());
    testRunner->AttachTestCase(URITest::Create());
    testRunner->AttachTestCase(StringTest::Create());   
    testRunner->AttachTestCase(ArrayTest::Create());
    testRunner->AttachTestCase(StackTest::Create());
    testRunner->AttachTestCase(ListTest::Create());
    testRunner->AttachTestCase(DictionaryTest::Create());
    testRunner->AttachTestCase(FixedArrayTest::Create());
    testRunner->AttachTestCase(FixedTableTest::Create());
    testRunner->AttachTestCase(HashTableTest::Create());
    testRunner->AttachTestCase(QueueTest::Create());
    testRunner->AttachTestCase(ArrayQueueTest::Create());
    testRunner->AttachTestCase(MemoryStreamTest::Create());
    testRunner->AttachTestCase(GuidTest::Create());
    testRunner->AttachTestCase(FileServerTest::Create());
    testRunner->AttachTestCase(TextReaderWriterTest::Create());
    testRunner->AttachTestCase(MessageReaderWriterTest::Create());
    testRunner->AttachTestCase(XmlReaderWriterTest::Create());
    // testRunner->AttachTestCase(JSonReaderWriterTest::Create());
    testRunner->AttachTestCase(BinaryReaderWriterTest::Create());
    testRunner->AttachTestCase(VariantTest::Create());
    testRunner->AttachTestCase(IOInterfaceTest::Create());
    testRunner->AttachTestCase(ThreadTest::Create());
    testRunner->AttachTestCase(ArrayAllocatorTest::Create());
    testRunner->AttachTestCase(ProfilingTest::Create());
    bool result = testRunner->Run(); 

    gameContentServer->Discard();
    gameContentServer = nullptr;
    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;
    
    Core::SysFunc::Exit(result ? 0 : -1);
}
