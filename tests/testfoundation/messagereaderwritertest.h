#ifndef TEST_MESSAGEREADERWRITERTEST_H
#define TEST_MESSAGEREADERWRITERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::MessageReaderWriterTest
    
    Test message reader/writer functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"
#include "messaging/message.h"
#include "math/point.h"

//------------------------------------------------------------------------------
namespace Test
{
class MessageReaderWriterTest : public TestCase
{
    __DeclareClass(MessageReaderWriterTest);
public:
    /// run the test
    virtual void Run();
};

/// a test message for encoding/decoding
class TestMessage : public Messaging::Message
{
    __DeclareClass(TestMessage);
    __DeclareMsgId;
public:
    /// encode message into a stream
    virtual void Encode(const Ptr<IO::BinaryWriter>& writer);
    /// decode message from a stream
    virtual void Decode(const Ptr<IO::BinaryReader>& reader);
    /// set command string
    void SetCommand(const Util::String& s);
    /// get command string
    const Util::String& GetCommand() const;
    /// set position
    void SetPosition(const Math::point& p);
    /// get position
    const Math::point& GetPosition() const;
    /// set velocity
    void SetVelocity(float f);
    /// get velocity
    float GetVelocity() const;
private:
    Util::String command;
    Math::point position;
    float velocity;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
TestMessage::SetCommand(const Util::String& s)
{
    this->command = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Util::String&
TestMessage::GetCommand() const
{
    return this->command;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
TestMessage::SetPosition(const Math::point& p)
{
    this->position = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
const Math::point&
TestMessage::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
TestMessage::SetVelocity(float v)
{
    this->velocity = v;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
TestMessage::GetVelocity() const
{
    return this->velocity;
}

}; // namespace Test
//------------------------------------------------------------------------------
#endif    