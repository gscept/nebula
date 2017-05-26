//------------------------------------------------------------------------------
//  delegatetabletest.cc
//  (C) 2008 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#if !__PS3__
#include "delegatetabletest.h"
#include "io/iointerfaceprotocol.h"
#include "messaging/delegatetable.h"

namespace Test
{
__ImplementClass(Test::DelegateTableTest, 'dttt', Test::TestCase);

using namespace Messaging;
using namespace Util;
using namespace IO;

// some test message handler classes
class A
{
public:
    void OnCopyFile(const Ptr<Message>& msg) { n_printf("HandlerClass1::OnCopyFile() called!\n"); };
    void OnCreateDirectory(const Ptr<Message>& msg) { n_printf("HandlerClass1::OnCreateDirectory() called!\n"); };
};

class B
{
public:
    void OnDeleteDirectory(const Ptr<Message>& msg) { n_printf("HandlerClass2::OnDeleteDirectory() called!\n"); };
    void OnCreateDirectory(const Ptr<Message>& msg) { n_printf("HandlerClass2::OnCreateDirectory() called!\n"); };
};

//------------------------------------------------------------------------------
/**
*/
void
DelegateTableTest::Run()
{
    // create 2 message handler objects
    A a;
    B b;

    // setup a DelegateTable
    DelegateTable delTable;
    delTable.Bind<A,&A::OnCopyFile>(IO::CopyFile::Id, &a);
    delTable.Bind<A,&A::OnCreateDirectory>(IO::CreateDirectory::Id, &a);
    delTable.Bind<B,&B::OnDeleteDirectory>(IO::DeleteDirectory::Id, &b);
    delTable.Bind<B,&B::OnCreateDirectory>(IO::CreateDirectory::Id, &b);

    // invoke message handler functions
    Ptr<IO::CopyFile> copyFileMsg = IO::CopyFile::Create();
    Ptr<IO::CreateDirectory> createDirMsg = IO::CreateDirectory::Create();
    Ptr<IO::DeleteDirectory> delDirMsg = IO::DeleteDirectory::Create();
    Ptr<IO::MountArchive> mntZipMsg = IO::MountArchive::Create();

    this->Verify(delTable.Invoke(copyFileMsg.cast<Message>()));
    this->Verify(delTable.Invoke(createDirMsg.cast<Message>()));
    this->Verify(delTable.Invoke(delDirMsg.cast<Message>()));
    this->Verify(!delTable.Invoke(mntZipMsg.cast<Message>()));
}    

} // namespace Test
#endif
