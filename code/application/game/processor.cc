//------------------------------------------------------------------------------
//  @file processor.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
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
    name(processorName)
{
    this->filterBuilder = FilterBuilder();
    // empty
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

    // TODO: register to correct event
    info.OnBeginFrame = this->func;
    info.async = this->async;
    info.filter = this->filterBuilder.Build();

    return CreateProcessor(info);
}

} // namespace Game
