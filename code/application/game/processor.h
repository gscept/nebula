#pragma once
//------------------------------------------------------------------------------
/**
    @file processor.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/stringatom.h"
#include "world.h"
#include "filter.h"

namespace Game
{

/// Opaque processor handle
typedef uint32_t ProcessorHandle;

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
};

/// Create a processor
ProcessorHandle CreateProcessor(ProcessorCreateInfo const& info);
/// register processors to a world
void RegisterProcessors(World*, std::initializer_list<ProcessorHandle>);

class ProcessorBuilder
{
public:
    ProcessorBuilder() = delete;
    ProcessorBuilder(Util::StringAtom processorName);

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

    /// select on which event the processor is executed
    ProcessorBuilder& On(Util::StringAtom eventName);
    
    /// processor should run async
    ProcessorBuilder& Async();
    
    /// create and register the processor
    ProcessorHandle Build();

private:
    template<typename...TYPES, std::size_t...Is>
    static void UpdateExpander(World* world, std::function<void(World*, TYPES...)> const& func, Game::Dataset::EntityTableView const& view, const IndexT instance, std::index_sequence<Is...>)
    {
        // this is a terribly unreadable line. Here's what it does:
        // it unpacks the the index sequence and TYPES into individual parameters for func
        // because we need to cast void pointers (the view buffers), we need to remove any const and reference qualifiers from the type.
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
    this->filterBuilder.Including<COMPONENTS...>();
    this->func = [func](World* world, Game::Dataset data) {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                UpdateExpander<COMPONENTS...>(world, func, view, i, std::make_index_sequence<sizeof...(COMPONENTS)>());
            }
        }
    };
    return *this;
}

template<typename ...COMPONENTS>
inline ProcessorBuilder& ProcessorBuilder::Including()
{
    this->filterBuilder.Including<COMPONENTS...>();
    return *this;
}

template<typename ...COMPONENTS>
inline ProcessorBuilder& ProcessorBuilder::Excluding()
{
    this->filterBuilder.Excluding<COMPONENTS...>();
    return *this;
}

} // namespace Game
