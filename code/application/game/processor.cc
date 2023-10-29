//------------------------------------------------------------------------------
//  @file processor.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "processor.h"
#include "gameserver.h"
#include "basegamefeature/components/basegamefeature.h"

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
ProcessorBuilder::ProcessorBuilder(World* world, Util::StringAtom processorName)
    : world(world),
      name(processorName),
      onEvent("OnBeginFrame")
{
    this->filterBuilder = FilterBuilder();
}

ProcessorBuilder::ProcessorBuilder(Game::World* world, Util::StringAtom processorName)
{
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
ProcessorBuilder&
ProcessorBuilder::Order(int order)
{
    this->order = order;
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

    // clang-format off
    if      (this->onEvent == "OnBeginFrame")  info.OnBeginFrame = this->func;
    else if (this->onEvent == "OnFrame")       info.OnFrame = this->func;
    else if (this->onEvent == "OnEndFrame")    info.OnEndFrame = this->func;
    else if (this->onEvent == "OnSave")        info.OnSave = this->func;
    else if (this->onEvent == "OnLoad")        info.OnLoad = this->func;
    else if (this->onEvent == "OnRenderDebug") info.OnRenderDebug = this->func;
    else if (this->onEvent == "OnActivate")
    {
        info.OnActivate = this->func;
        this->filterBuilder.Excluding<Game::IsActive>();
    }
    else
    {
        n_error("Invalid event name in processor!\n");
        info.OnBeginFrame = this->func;
    }
    // clang-format on

    info.async = this->async;
    info.filter = this->filterBuilder.Build();

    return CreateProcessor(info);
}

Processor*
ProcessorBuilder::BuildP()
{
    Processor* processor = new Processor();
    processor->name = this->name.AsString();
    processor->async = this->async;
    processor->order = this->order;
    processor->filter = this->filterBuilder.Build();
    processor->callback = this->func;
    FrameEvent* frameEvent = world->GetFrameEvent(this->onEvent);
    frameEvent->AddProcessor(processor);
    return processor;
}

} // namespace Game
