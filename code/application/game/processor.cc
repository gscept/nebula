//------------------------------------------------------------------------------
//  @file processor.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "processor.h"
#include "gameserver.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
ProcessorHandle
CreateProcessor(ProcessorCreateInfo const& info)
{
    return Game::GameServer::Instance()->CreateProcessor(info);
}

//------------------------------------------------------------------------------
/**
*/
ProcessorBuilder::ProcessorBuilder(Util::StringAtom processorName) :
    name(processorName),
    onEvent("OnBeginFrame")
{
    this->filterBuilder = FilterBuilder();
}

//------------------------------------------------------------------------------
/**
*/
ProcessorBuilder&
ProcessorBuilder::Excluding(std::initializer_list<ComponentId> components)
{
    this->filterBuilder.Excluding(components);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
ProcessorBuilder&
ProcessorBuilder::On(Util::StringAtom eventName)
{
    this->onEvent = eventName;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
ProcessorBuilder&
ProcessorBuilder::Async()
{
    this->async = true;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
ProcessorHandle
ProcessorBuilder::Build()
{
    ProcessorCreateInfo info;
    info.name = this->name;

    if      (this->onEvent == "OnBeginFrame")  info.OnBeginFrame = this->func;
    else if (this->onEvent == "OnFrame")       info.OnFrame = this->func;
    else if (this->onEvent == "OnEndFrame")    info.OnEndFrame = this->func;
    else if (this->onEvent == "OnSave")        info.OnSave = this->func;
    else if (this->onEvent == "OnLoad")        info.OnLoad = this->func;
    else if (this->onEvent == "OnRenderDebug") info.OnRenderDebug = this->func;
    else
    {
        n_error("Invalid event name in processor!\n");
        info.OnBeginFrame = this->func;
    }

    info.async = this->async;
    info.filter = this->filterBuilder.Build();

    return CreateProcessor(info);
}

} // namespace Game
