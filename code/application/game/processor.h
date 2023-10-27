#pragma once
//------------------------------------------------------------------------------
/**
    @file processor.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "filter.h"
#include "processorid.h"

namespace Game
{

class World;

/// per frame callback for processors
using ProcessorFrameCallback = std::function<void(World*, Dataset)>;

//------------------------------------------------------------------------------
/**
*/
struct ProcessorCreateInfo
{
    /// name of the processor
    Util::StringAtom name;

    /// set if this processor should run as a job.
    /// TODO: this is currently not used
    bool async = false;
    /// filter used for creating the dataset
    Filter filter;

    /// called when attached to world
    //void(*OnActivate)() = nullptr;
    ///// called when removed from world
    //void(*OnDeactivate)() = nullptr;
    ///// called by Game::Server::Start()
    //void(*OnStart)() = nullptr;

    /// called before frame by the game server
    ProcessorFrameCallback OnBeginFrame = nullptr;
    /// called per-frame by the game server
    ProcessorFrameCallback OnFrame = nullptr;
    /// called after frame by the game server
    ProcessorFrameCallback OnEndFrame = nullptr;
    /// called after loading game state
    ProcessorFrameCallback OnLoad = nullptr;
    /// called before saving game state
    ProcessorFrameCallback OnSave = nullptr;
    /// render a debug visualization 
    ProcessorFrameCallback OnRenderDebug = nullptr;
    /// called on new partitions at beginning of frame
    ProcessorFrameCallback OnActivate = nullptr;
};

/// Create a processor
ProcessorHandle CreateProcessor(ProcessorCreateInfo const& info);

class ProcessorBuilder
{
public:
    ProcessorBuilder() = delete;
    ProcessorBuilder(Util::StringAtom processorName);

    /// which function to run with the processor
    template<typename LAMBDA>
    ProcessorBuilder& Func(LAMBDA);

    /// which function to run with the processor
    template<typename ...COMPONENTS>
    ProcessorBuilder& Func(std::function<void(World*, COMPONENTS...)> func);

    /// entities must have these components
    template<typename ... COMPONENTS>
    ProcessorBuilder& Including();

    /// entities must not have any of these components
    template<typename ... COMPONENTS>
    ProcessorBuilder& Excluding();

    /// entities must not have any of these components
    ProcessorBuilder& Excluding(std::initializer_list<ComponentId>);

    /// select on which event the processor is executed
    ProcessorBuilder& On(Util::StringAtom eventName);

    /// processor should run async
    ProcessorBuilder& Async();
    
    /// create and register the processor
    ProcessorHandle Build();

private:
    template<typename...TYPES, std::size_t...Is>
    static void UpdateExpander(World* world, std::function<void(World*, TYPES...)> const& func, Game::Dataset::EntityTableView const& view, const IndexT instance, uint8_t const bufferStartOffset, std::index_sequence<Is...>)
    {
        func(world, *((typename std::remove_const<typename std::remove_reference<TYPES>::type>::type*)view.buffers[Is] + instance)...);
    }

    Util::StringAtom name;
    Util::StringAtom onEvent;
    ProcessorFrameCallback func = nullptr;
    FilterBuilder filterBuilder;
    bool async = false;
};

//------------------------------------------------------------------------------
/**
*/
template<typename LAMBDA> 
ProcessorBuilder& ProcessorBuilder::Func(LAMBDA lambda)
{
    static_assert(std::is_invocable<LAMBDA()>());
    return this->Func(std::function(lambda));
}

//------------------------------------------------------------------------------
/**
*/
template<typename ...COMPONENTS>
inline ProcessorBuilder&
ProcessorBuilder::Func(std::function<void(World*, COMPONENTS...)> func)
{
    uint8_t const bufferStartOffset = this->filterBuilder.GetNumInclusive();
    this->filterBuilder.Including<COMPONENTS...>();
    this->func = [func, bufferStartOffset](World* world, Game::Dataset data)
    {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];
            
            uint32_t i = 0;
            uint32_t section = 0;
            while (i < view.numInstances)
            {
                // check validity of instances in sections of 64 instances
                if (!view.validInstances.SectionIsNull(section))
                {
                    uint32_t const end = Math::min(i + 64, view.numInstances);
                    for (uint32_t instance = i; instance < end; ++instance)
                    {
                        // make sure the instance we're processing is valid
                        if (view.validInstances.IsSet(instance))
                        {
                            UpdateExpander<COMPONENTS...>(
                                world, func, view, instance, bufferStartOffset, std::make_index_sequence<sizeof...(COMPONENTS)>()
                            );
                        }
                    }
                }
                // progress 64 instances, which corresponds to 1 section
                i += 64;
                section += 1;
            }
        }
    };
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<typename ...COMPONENTS>
inline ProcessorBuilder& ProcessorBuilder::Including()
{
    this->filterBuilder.Including<COMPONENTS...>();
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
template<typename ...COMPONENTS>
inline ProcessorBuilder& ProcessorBuilder::Excluding()
{
    this->filterBuilder.Excluding<COMPONENTS...>();
    return *this;
}

} // namespace Game
