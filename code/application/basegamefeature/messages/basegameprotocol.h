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
class SetLocalTransform : public Game::Message<SetLocalTransform, Game::Entity, Math::matrix44 const&>
{
public:
    SetLocalTransform() = delete;
    ~SetLocalTransform() = delete;
    constexpr static const char* GetName() { return "SetLocalTransform"; };
    constexpr static const uint GetFourCC()	{ return 'SLTM'; };
    static void Send(Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, value);
    }
    static void Defer(MessageQueueId qid, Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, value);
    }
};
        

//------------------------------------------------------------------------------
/**
*/
class SetWorldTransform : public Game::Message<SetWorldTransform, Game::Entity, Math::matrix44 const&>
{
public:
    SetWorldTransform() = delete;
    ~SetWorldTransform() = delete;
    constexpr static const char* GetName() { return "SetWorldTransform"; };
    constexpr static const uint GetFourCC()	{ return 'SWTM'; };
    static void Send(Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, value);
    }
    static void Defer(MessageQueueId qid, Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, value);
    }
};
        

//------------------------------------------------------------------------------
/**
*/
class UpdateTransform : public Game::Message<UpdateTransform, Game::Entity, Math::matrix44 const&>
{
public:
    UpdateTransform() = delete;
    ~UpdateTransform() = delete;
    constexpr static const char* GetName() { return "UpdateTransform"; };
    constexpr static const uint GetFourCC()	{ return 'UpdT'; };
    static void Send(Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, value);
    }
    static void Defer(MessageQueueId qid, Game::Entity entity, Math::matrix44 const& value)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, value);
    }
};
        

//------------------------------------------------------------------------------
/**
*/
class SetParent : public Game::Message<SetParent, Game::Entity, Game::Entity>
{
public:
    SetParent() = delete;
    ~SetParent() = delete;
    constexpr static const char* GetName() { return "SetParent"; };
    constexpr static const uint GetFourCC()	{ return 'SetP'; };
    static void Send(Game::Entity entity, Game::Entity parent)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, parent);
    }
    static void Defer(MessageQueueId qid, Game::Entity entity, Game::Entity parent)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, parent);
    }
};
        
} // namespace Msg
