#include "io/ioserver.h"
#include "io/stream.h"
#include "tbuifile.h"
#include "tb/tb_system.h"

namespace tb
{
// static
TBFile*
tb::TBFile::Open(const char* filename, TBFileMode mode)
{
    if (mode != TBFileMode::MODE_READ)
        return nullptr;

    TBUI::TBUIFile* file = new ::TBUI::TBUIFile(filename, IO::Stream::AccessMode::ReadAccess);
    if (!file->IsOpen())
    {
        file = nullptr;
    }

    return file;
}
} // namespace tb

namespace TBUI
{
TBUIFile::TBUIFile(const Util::String& filePath, IO::Stream::AccessMode accessMode)
    : fileStream(nullptr)
{
    fileStream = IO::IoServer::Instance()->CreateStream(filePath).downcast<IO::FileStream>();
    fileStream->SetAccessMode(accessMode);
    if (!fileStream->Open())
    {
        fileStream = nullptr;
    }
}

TBUIFile::~TBUIFile()
{
    if (fileStream->IsOpen())
        fileStream->Close();
}

long
TBUIFile::Size()
{
    return fileStream->GetSize();
}

size_t
TBUIFile::Read(void* buf, size_t elemSize, size_t count)
{
    return fileStream->Read(buf, count);
}
} // namespace TBUI
