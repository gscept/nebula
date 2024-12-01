#pragma once
//------------------------------------------------------------------------------
/**
    Turbobadger UI File interface

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "io/filestream.h"
#include "util/string.h"
#include "tb_system.h"

//------------------------------------------------------------------------------
namespace TBUI
{
class TBUIFile : public tb::TBFile
{
public:
    ///
    TBUIFile(const Util::String& filepath, IO::Stream::AccessMode accessMode);
    ///
    ~TBUIFile();

    ///
    long Size() override;
    ///
    size_t Read(void* buf, size_t elemSize, size_t count) override;
    ///
    bool IsOpen() const;

private:
    Ptr<IO::FileStream> fileStream;
};

//------------------------------------------------------------------------------
/*
*/
inline bool
TBUIFile::IsOpen() const
{
    return fileStream && fileStream->IsOpen();
}

} // namespace TBUI
