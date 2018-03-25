//------------------------------------------------------------------------------
//  MACHINE GENERATED, DON'T EDIT!
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "iointerfaceprotocol.h"


namespace IO
{
    __ImplementClass(IO::CopyFile, 'cofi', Messaging::Message);
    __ImplementMsgId(CopyFile);
    __ImplementClass(IO::IOMessage, 'iomg', Messaging::Message);
    __ImplementMsgId(IOMessage);
    __ImplementClass(IO::CreateDirectory, 'crdi', IOMessage);
    __ImplementMsgId(CreateDirectory);
    __ImplementClass(IO::DeleteDirectory, 'dedi', IOMessage);
    __ImplementMsgId(DeleteDirectory);
    __ImplementClass(IO::DeleteFile, 'defi', IOMessage);
    __ImplementMsgId(DeleteFile);
    __ImplementClass(IO::MountArchive, 'mozi', IOMessage);
    __ImplementMsgId(MountArchive);
    __ImplementClass(IO::ReadStream, 'rest', IOMessage);
    __ImplementMsgId(ReadStream);
    __ImplementClass(IO::WriteStream, 'wrst', IOMessage);
    __ImplementMsgId(WriteStream);
} // IO

namespace Commands
{
} // namespace Commands
//------------------------------------------------------------------------------
