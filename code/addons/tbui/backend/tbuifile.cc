//------------------------------------------------------------------------------
//  backend/tbuifile.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "tbuifile.h"
#include "tb_system.h"

namespace tb
{
//------------------------------------------------------------------------------
/**
*/
TBFile*
TBFile::Open(const char* filename, TBFileMode mode)
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
//------------------------------------------------------------------------------
/**
*/
TBUIFile::TBUIFile(const Util::String& filePath, IO::Stream::AccessMode accessMode)
    : fileStream(nullptr)
{
    this->fileStream = IO::IoServer::Instance()->CreateStream(filePath).downcast<IO::FileStream>();
    this->fileStream->SetAccessMode(accessMode);
    if (!this->fileStream->Open())
    {
        this->fileStream = nullptr;
    }
}

//------------------------------------------------------------------------------
/**
*/
TBUIFile::~TBUIFile()
{
    if (this->fileStream->IsOpen())
    {
        this->fileStream->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
long
TBUIFile::Size()
{
    return this->fileStream->GetSize();
}

//------------------------------------------------------------------------------
/**
*/
size_t
TBUIFile::Read(void* buf, size_t elemSize, size_t count)
{
    return this->fileStream->Read(buf, count);
}
} // namespace TBUI
