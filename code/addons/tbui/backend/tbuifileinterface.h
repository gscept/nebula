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
#include "util/dictionary.h"
#include "platform/tb_file_interface.h"

namespace TBUI
{
class TBUIFileInterface : public tb::TBFileInterface
{
public:
    tb::TBFileHandle Open(const char* filename, TBFileMode mode) override;
    void Close(tb::TBFileHandle file) override;

    long Size(tb::TBFileHandle file) override;
    size_t Read(tb::TBFileHandle file, void* buf, size_t elemSize, size_t count) override;

    bool IsOpen(tb::TBFileHandle file) const;

private:
    Util::Dictionary<size_t, Ptr<IO::FileStream>> openFiles;
};

inline bool
TBUIFileInterface::IsOpen(tb::TBFileHandle file) const
{
    return openFiles.Contains(static_cast<size_t>(file));
}
}
