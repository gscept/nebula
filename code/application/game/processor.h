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

class ProcessorBuilder;

class Processor
{
public:
    /// name of the processor
    Util::String name;
    /// sorting order within frame event (same as batch order).
    int order = 100;
    /// set if this processor should run as a job.
    bool async = false;
    /// filter used for creating the dataset
    Filter filter;
    /// function that this processor runs
    std::function<void(World*, Dataset::View const&)> callback;
    /// cached tables that fulfill the requirements of the filter
    Util::Array<MemDb::TableId> cache;
    /// set to false if the cache is invalid
    bool cacheValid = false;
    /// set to false if the processor shouldn't execute in the frame.
    bool active = true;
#ifdef WITH_NEBULA_EDITOR
    /// set to true if you want this processor to always run in editor
    bool runInEditor = false;
#endif

private:
    friend ProcessorBuilder;

    template <typename... TYPES, std::size_t... Is>
    static void
    UpdateExpander(World* world, std::function<void(World*, TYPES...)> const& func, Game::Dataset::View const& view, const IndexT instance, uint8_t const bufferStartOffset, std::index_sequence<Is...>)
    {
        func(
            world,
            *((typename std::remove_const<typename std::remove_reference<TYPES>::type>::type*)view.buffers[Is] + instance)...
        );
    }

    template <typename... COMPONENTS>
    static std::function<void(World*, Dataset::View const&)>
    ForEach(std::function<void(World*, COMPONENTS...)> func, uint8_t bufferStartOffset)
    {
        return [func, bufferStartOffset](World* world, Game::Dataset::View const& view)
        {
            uint16_t i = 0;
            while (i < view.numInstances)
            {
                // check validity of instances in sections of 64 instances
                if (!view.validInstances.SectionIsNull(i / 64))
                {
                    uint16_t const end = Math::min<uint16_t>(i + uint16_t(64), view.numInstances);
                    for (uint32_t instance = i; instance < end; ++instance)
                    {
                        // make sure the instance we're processing is valid
                        if (view.validInstances.IsSet(instance))
                        {
                            UpdateExpander<COMPONENTS...>(
                                world,
                                func,
                                view,
                                instance,
                                bufferStartOffset,
                                std::make_index_sequence<sizeof...(COMPONENTS)>()
                            );
                        }
                    }
                }
                // progress 64 instances, which corresponds to 1 section
                i += 64;
            }
        };
    }

    template <typename... COMPONENTS>
    static std::function<void(World*, Dataset::View const&)>
    ForEachModified(std::function<void(World*, COMPONENTS...)> func, uint8_t bufferStartOffset)
    {
        return [func, bufferStartOffset](World* world, Game::Dataset::View const& view)
        {
            uint16_t i = 0;
            while (i < view.numInstances)
            {
                
                // check validity of instances in sections of 64 instances
                uint64_t section = i / 64;
                if (!view.validInstances.SectionIsNull(section) && !view.modifiedInstances.SectionIsNull(section))
                {
                    uint16_t const end = Math::min<uint16_t>(i + uint16_t(64), view.numInstances);
                    for (uint32_t instance = i; instance < end; ++instance)
                    {
                        // make sure the instance we're processing is valid
                        if (view.validInstances.IsSet(instance) && view.modifiedInstances.IsSet(instance))
                        {
                            UpdateExpander<COMPONENTS...>(
                                world,
                                func,
                                view,
                                instance,
                                bufferStartOffset,
                                std::make_index_sequence<sizeof...(COMPONENTS)>()
                            );
                        }
                    }
                }
                // progress 64 instances, which corresponds to 1 section
                i += 64;
            }
        };
    }
};

class ProcessorBuilder
{
public:
    ProcessorBuilder() = delete;
    ProcessorBuilder(Game::World* world, Util::StringAtom processorName);

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
    
    /// entities must be marked as modified for them to actually be processed
    ProcessorBuilder& OnlyModified();

    /// Set the sorting order for the processor
    ProcessorBuilder& Order(int order);

    /// Processor should always run, even in editor; when the game is paused.
    ProcessorBuilder& RunInEditor();

    /// Build the processor and attach it to the world
    Processor* Build();

private:
    World* world;
    Util::StringAtom name;
    Util::StringAtom onEvent;
    std::function<void(World*, Dataset::View const&)> func = nullptr;
    std::function<void(World*, Dataset::View const&)> funcModified = nullptr;
    FilterBuilder filterBuilder;
    bool async = false;
    bool onlyModified = false;
    int order = 100;
#ifdef WITH_NEBULA_EDITOR
    bool runInEditor = false;
#endif
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
    this->func = Processor::ForEach(func, bufferStartOffset);
    this->funcModified = Processor::ForEachModified(func, bufferStartOffset);
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
