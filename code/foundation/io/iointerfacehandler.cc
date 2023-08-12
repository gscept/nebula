//------------------------------------------------------------------------------
//  iointerface.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/iointerfacehandler.h"

namespace IO
{
__ImplementClass(IO::IoInterfaceHandler, 'IIOH', Interface::InterfaceHandlerBase);

using namespace Interface;
using namespace Messaging;

//------------------------------------------------------------------------------
/**    
*/
IoInterfaceHandler::IoInterfaceHandler()
{
}

//------------------------------------------------------------------------------
/**    
*/
IoInterfaceHandler::~IoInterfaceHandler()
{
    n_assert(!this->IsOpen());
}

//------------------------------------------------------------------------------
/**
    Opens the Interface message handler which does all the interesting stuff.
    This method already runs in the handler thread.
    The method initializes a minimal thread local Nebula runtime, just
    enough to handle the IO messages.
*/
void
IoInterfaceHandler::Open()
{
    InterfaceHandlerBase::Open();
    this->ioServer = IO::IoServer::Create();
}

//------------------------------------------------------------------------------
/**
    Closes the Interface message handler. This will shut down the
    minimal Nebula runtime, the method runs in the handler thread and is
    called just before the thread shuts down.
*/
void
IoInterfaceHandler::Close()
{
    this->ioServer = nullptr;
    InterfaceHandlerBase::Close();
}

//------------------------------------------------------------------------------
/**
    Handles incoming messages. This method runs in the handler thread.
*/
bool
IoInterfaceHandler::HandleMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    if (msg->CheckId(MountArchive::Id))
    {
        this->OnMountArchive(msg.downcast<IO::MountArchive>());
    }
    else if (msg->CheckId(CreateDirectory::Id))
    {
        this->OnCreateDirectory(msg.downcast<IO::CreateDirectory>());
    }
    else if (msg->CheckId(DeleteDirectory::Id))
    {
        this->OnDeleteDirectory(msg.downcast<IO::DeleteDirectory>());
    }
    else if (msg->CheckId(DeleteFile::Id))
    {
        this->OnDeleteFile(msg.downcast<IO::DeleteFile>());
    }
    else if (msg->CheckId(WriteStream::Id))
    {
        this->OnWriteStream(msg.downcast<IO::WriteStream>());
    }
    else if (msg->CheckId(ReadStream::Id))
    {
        this->OnReadStream(msg.downcast<IO::ReadStream>());
    }
    else if (msg->CheckId(CopyFile::Id))
    {
        this->OnCopyFile(msg.downcast<IO::CopyFile>());
    }
    else
    {
        // unknown message
        return false;        
    }
    // fallthrough: message was handled
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnMountArchive(const Ptr<IO::MountArchive>& msg)
{
    //n_printf("IOInterface: MountArchive %s\n", msg->GetURI().AsString().AsCharPtr());
    IO::IoServer::Instance()->MountArchive(msg->GetURI());
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnCreateDirectory(const Ptr<IO::CreateDirectory>& msg)
{
    //n_printf("IOInterface: CreateDirectory %s\n", msg->GetURI().AsString().AsCharPtr());
    msg->SetResult(IO::IoServer::Instance()->CreateDirectory(msg->GetURI()));
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnDeleteDirectory(const Ptr<IO::DeleteDirectory>& msg)
{
    //n_printf("IOInterface: DeleteDirectory %s\n", msg->GetURI().AsString().AsCharPtr());
    msg->SetResult(IO::IoServer::Instance()->DeleteDirectory(msg->GetURI()));
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnCopyFile(const Ptr<IO::CopyFile>& msg)
{
    //n_printf("IOInterface: CopyDirectory %s %s\n", msg->GetFromURI().AsString().AsCharPtr(), msg->GetToURI().AsString().AsCharPtr());
    msg->SetResult(IO::IoServer::Instance()->CopyFile(msg->GetFromURI(), msg->GetToURI()));
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnDeleteFile(const Ptr<IO::DeleteFile>& msg)
{
    //n_printf("IOInterface: DeleteFile %s\n", msg->GetURI().AsString().AsCharPtr());
    msg->SetResult(IO::IoServer::Instance()->DeleteFile(msg->GetURI()));
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnWriteStream(const Ptr<IO::WriteStream>& msg)
{
    //n_printf("IOInterface: WriteStream %s\n", msg->GetURI().AsString().AsCharPtr());

    // create a destination file stream object
    msg->SetResult(false);
    Ptr<Stream> dstStream = IO::IoServer::Instance()->CreateStream(msg->GetURI());
    dstStream->SetAccessMode(Stream::WriteAccess);
    if (dstStream->Open())
    {
        /// @todo handle non-mappable stream
        const Ptr<Stream>& srcStream = msg->GetStream();
        n_assert(srcStream.isvalid());
        n_assert(srcStream->CanBeMapped());
        srcStream->SetAccessMode(Stream::ReadAccess);
        if (srcStream->Open())
        {
            void* ptr = srcStream->Map();
            dstStream->Write(ptr, srcStream->GetSize());
            srcStream->Unmap();
            srcStream->Close();
            msg->SetResult(true);
        }
        dstStream->Close();        
    }
}

//------------------------------------------------------------------------------
/**
*/
void
IoInterfaceHandler::OnReadStream(const Ptr<IO::ReadStream>& msg)
{
    // n_printf("IOInterface: ReadStream %s\n", msg->GetURI().AsString().AsCharPtr());

    // create a file stream which reads in the data from disc
    msg->SetResult(false);
    Ptr<Stream> srcStream = IO::IoServer::Instance()->CreateStream(msg->GetURI());
    srcStream->SetAccessMode(Stream::ReadAccess);
    if (srcStream->Open())
    {
        /// @todo handle non-mappable stream!
        const Ptr<Stream>& dstStream = msg->GetStream();
        n_assert(dstStream.isvalid());
        n_assert(dstStream->CanBeMapped());
        dstStream->SetAccessMode(Stream::WriteAccess);
        Stream::Size srcSize = srcStream->GetSize();
        n_assert(srcSize > 0);
        dstStream->SetURI(msg->GetURI());
        if (dstStream->Open())
        {
            dstStream->SetSize(srcSize);
            void* ptr = dstStream->Map();
            n_assert(0 != ptr);
            srcStream->Read(ptr, srcSize);
            dstStream->Unmap();
            dstStream->Close();
            msg->SetResult(true);
        }
        srcStream->Close();
    }
}

} // namespace IO
