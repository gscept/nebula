#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula Trifid's idlc compiler tool.
    DO NOT EDIT
*/
#include "messaging/message.h"
#include "http/httprequesthandler.h"

//------------------------------------------------------------------------------
namespace Http
{
//------------------------------------------------------------------------------
class AttachRequestHandler : public Messaging::Message
{
    __DeclareClass(AttachRequestHandler);
    __DeclareMsgId;
public:
    AttachRequestHandler() 
    { };
public:
    void SetRequestHandler(const Ptr<Http::HttpRequestHandler>& val)
    {
        n_assert(!this->handled);
        this->requesthandler = val;
    };
    const Ptr<Http::HttpRequestHandler>& GetRequestHandler() const
    {
        return this->requesthandler;
    };
private:
    Ptr<Http::HttpRequestHandler> requesthandler;
};
//------------------------------------------------------------------------------
class RemoveRequestHandler : public Messaging::Message
{
    __DeclareClass(RemoveRequestHandler);
    __DeclareMsgId;
public:
    RemoveRequestHandler() 
    { };
public:
    void SetRequestHandler(const Ptr<Http::HttpRequestHandler>& val)
    {
        n_assert(!this->handled);
        this->requesthandler = val;
    };
    const Ptr<Http::HttpRequestHandler>& GetRequestHandler() const
    {
        return this->requesthandler;
    };
private:
    Ptr<Http::HttpRequestHandler> requesthandler;
};
} // namespace Http
//------------------------------------------------------------------------------
