// NIDL #version:39#
#pragma once
//------------------------------------------------------------------------------
/**
    This file was generated with Nebula's IDL compiler tool.
    DO NOT EDIT
*/
#include "#include <pybind11/pybind11.h>"
//------------------------------------------------------------------------------
namespace Msg
{

//------------------------------------------------------------------------------
/**
*/
class SetModel : public Game::Message<SetModel, Game::Entity, Util::String>
{
public:
    SetModel() = delete;
    ~SetModel() = delete;
    constexpr static const char* GetName() { return "SetModel"; };
    constexpr static const uint GetFourCC()	{ return 'SMDL'; };
    static void Send(const Game::Entity& entity, const Util::String& value)
    {
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)(entity, value);
    }
    static void Defer(MessageQueueId qid, const Game::Entity& entity, const Util::String& value)
    {
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, entity, value);
    }
};
        
PYBIND11_MODULE(graphicsprotocol, m)
{
    m.doc() = "namespace Msg";
    m.def("SetModel", &SetModel::Send, "Set the model resource.");
}
} // namespace Msg
