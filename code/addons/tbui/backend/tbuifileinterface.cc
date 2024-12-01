//------------------------------------------------------------------------------
//  backend/tbuifile.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "tbuifileinterface.h"

namespace
{
size_t
allocateFileId()
{
    static size_t s_fileId = 0;
    return ++s_fileId;
}
}

namespace TBUI
{
tb::TBFileHandle
TBUIFileInterface::Open(const char* filename, TBFileMode mode)
{
    if (mode != TBFileMode::MODE_READ)
        return 0;

    Ptr<IO::FileStream> stream = IO::IoServer::Instance()->CreateStream(filename).downcast<IO::FileStream>();
    stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
    if (!stream->Open())
    {
        return 0;
    }

    size_t fileId = allocateFileId();
    openFiles.Add(fileId, stream);
    return static_cast<tb::TBFileHandle>(fileId);
}

void
TBUIFileInterface::Close(tb::TBFileHandle file)
{
    size_t fileId = static_cast<size_t>(file);

    if (openFiles.Contains(fileId))
    {
        openFiles[fileId]->Close();
        openFiles.Erase(fileId);
    }
}

long
TBUIFileInterface::Size(tb::TBFileHandle file)
{
    size_t fileId = static_cast<size_t>(file);

    if (openFiles.Contains(fileId))
    {
        return openFiles[fileId]->GetSize();
    }

    return 0;
}

size_t
TBUIFileInterface::Read(tb::TBFileHandle file, void* buf, size_t elemSize, size_t count)
{
    size_t fileId = static_cast<size_t>(file);

    if (openFiles.Contains(fileId))
    {
        return openFiles[fileId]->Read(buf, count);
    }

    return 0;
}
}
