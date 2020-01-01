//------------------------------------------------------------------------------
//  messagewriter.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/messagewriter.h"
#include "messaging/message.h"

namespace Messaging
{
__ImplementClass(Messaging::MessageWriter, 'MGWR', IO::StreamWriter);

using namespace IO;
using namespace Util;
using namespace Core;

//------------------------------------------------------------------------------
/**
*/
MessageWriter::MessageWriter()
{
    this->binaryWriter = BinaryWriter::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
MessageWriter::SetStream(const Ptr<IO::Stream>& s)
{
    n_assert(s.isvalid());
    StreamWriter::SetStream(s);
    this->binaryWriter->SetStream(s);
}

//------------------------------------------------------------------------------
/**
    Writes a complete message to the stream. First the FourCC class id of
    the message will be written, then the message will be asked
    to write its own data to the stream.
*/
void
MessageWriter::WriteMessage(const Ptr<Message>& msg)
{
    n_assert(msg.isvalid());
    this->binaryWriter->WriteUInt(msg->GetClassFourCC().AsUInt());
    msg->Encode(this->binaryWriter);
}

} // namespace Messaging