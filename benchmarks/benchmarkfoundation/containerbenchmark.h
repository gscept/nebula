#pragma once
//------------------------------------------------------------------------------
/**
    @class Benchmarking::ContainerBench
             
    (C) 2018 Individucal contributors, see AUTHORS file
*/
#include "benchmarkbase/benchmark.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{

template<typename CONT, typename DATA>
class ContainerBenchmark 
{
public:    
    void Setup(DATA d, std::function<void(CONT&, DATA)> _add, std::function<DATA(CONT&)> _remove)
    {
        add = _add;
        remove = _remove;
        data = d;
    }
    void Run(Timing::Timer & timer, SizeT numObjects)
    {                
        n_printf("benchmarking type: %s\n", typeid(CONT).name());
        Time start = timer.GetTime();
        CONT c;
        for (int i = 0; i < numObjects; i++)
        {
            this->add(c, this->data);
        }
        DATA v;
        for (int i = 0; i < numObjects; i++)
        {
            v = this->remove(c);
        }
        Time last = timer.GetTime();
        n_printf("adding all %d items remove all after: %f\n", numObjects, last-start);
        
        c.Clear();
        Time now = timer.GetTime();
        n_printf("clear: %f\n", now - last);
        last = now;

        for (int i = 0; i < numObjects; i++)
        {
            this->add(c, this->data);
            v = this->remove(c);
        }
        now = timer.GetTime();
        n_printf("adding %d at a time remove right after in already allocated: %f\n", numObjects, now - last);
        last = now;
                   
        CONT c2;
        for (int i = 0; i < numObjects; i++)
        {
            this->add(c2, this->data);
            v = this->remove(c2);
        }
        now = timer.GetTime();
        n_printf("adding %d at a time remove right after: %f\n", numObjects, now - last);
        last = now;

        CONT c3;
        for (int i = 0; i < numObjects; i++)
        {
            for(int k = 0 ; k<5;k++)
                this->add(c3, this->data);
            for (int k = 0; k < 5; k++)                
                v = this->remove(c3);
        }
        now = timer.GetTime();
        n_printf("adding %d with 5 at a time remove right after: %f\n", numObjects, now - last);
        last = now;

        n_printf("Total time: %f\n", last - start);
        n_printf("---------------------------------------------------------------\n");
    }
    std::function <void(CONT&,DATA)> add;
    std::function <DATA(CONT&)> remove;
    DATA data;
    const char * Name() 
    {
        return typeid(CONT).name();
    }
    typedef typename CONT ctype ;
    typedef typename DATA dtype;
};



class ContainerBench : public Benchmark
{
    __DeclareClass(ContainerBench);
public:        
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);
};

} // namespace Benchmarking
//------------------------------------------------------------------------------
