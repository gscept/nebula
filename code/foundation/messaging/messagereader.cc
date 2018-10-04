//------------------------------------------------------------------------------
//  messagereader.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "messaging/messagereader.h"
#include "core/factory.h"
#include "messaging/message.h"

namespace Messaging
{
__ImplementClass(Messaging::MessageReader, 'MGRR', IO::StreamReader);

using namespace IO;
using namespace Util;
using namespace Core;

//------------------------------------------------------------------------------
/**
*/
MessageReader::MessageReader()
{
    this->binaryReader = BinaryReader::Create();
}

//------------------------------------------------------------------------------
/**
*/
void
MessageReader::SetStream(const Ptr<IO::Stream>& s)
{
    n_assert(s.isvalid());
    StreamReader::SetStream(s);
    this->binaryReader->SetStream(s);
}

//------------------------------------------------------------------------------
/**
    This constructs a new message from the stream. First the FourCC class id
    of the message will be read, and a new message object constructed from
    it, then the message object will be asked to initialize itself from
    the stream.
*/
Message*
MessageReader::ReadMessage()
{
    // read FourCC from stream and build a new message
    FourCC classFourCC = this->binaryReader->ReadUInt();
    Message* msg = (Message*) Factory::Instance()->Create(classFourCC);

	if (msg == nullptr)
	{
		// object couldn't be created.
		return nullptr;
	}

    // let message initialize itself from the rest of the stream
    msg->Decode(this->binaryReader);
    return msg;
}

} // namespace Messaging