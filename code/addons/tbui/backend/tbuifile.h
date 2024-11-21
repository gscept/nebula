#pragma once

#include "io/filestream.h"
#include "util/String.h"
#include "tb/tb_system.h"

namespace TBUI
{
class TBUIFile : public tb::TBFile
{
public:
    TBUIFile(const Util::String& filepath, IO::Stream::AccessMode accessMode);

    ~TBUIFile();

    long Size() override;
    size_t Read(void* buf, size_t elemSize, size_t count) override;

    inline bool
    IsOpen() const
    {
        return fileStream->IsOpen();
    }

private:
    Ptr<IO::FileStream> fileStream;
};
} // namespace TBUI
