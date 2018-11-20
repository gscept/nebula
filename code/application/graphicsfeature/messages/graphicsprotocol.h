// NIDL #version:57#
#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula's IDL compiler tool.
    DO NOT EDIT
*/
#include "game/entity.h"
#include "game/messaging/message.h"
//------------------------------------------------------------------------------
namespace Msg
{

//------------------------------------------------------------------------------
/**
*/
class SetModel : public Game::Message<SetModel, Game::Entity, Util::String const&>
{
public:
    SetModel() = delete;
    ~SetModel() = delete;
    constexpr static const char* GetName() { return "SetModel"; };
    constexpr static const uint GetFourCC()	{ return 'SMDL'; };
    static void Send(Game::Entity entity, Util::String const& value)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, value);
    }
    static void Defer(MessageQueueId qid, Game::Entity entity, Util::String const& value)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, value);
    }
};
        
} // namespace Msg
