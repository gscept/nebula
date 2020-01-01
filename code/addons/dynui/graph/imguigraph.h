#pragma once
//------------------------------------------------------------------------------
/**
    @class Dynui::Graph

    Time Graph with predefined amount of entries

    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "util/ringbuffer.h"
#include "util/string.h"


namespace Dynui
{

class Graph
{
public:
    ///
    Graph(Util::String const& name, SizeT historySize);
            
    ///
    void AddValue(float val);
    ///
    void Draw();    

private:   
    ///
    static float ValueGetter(void* object, int idx);

    float _min, _max;
    float frame_min, frame_max;
    float average;
    float averageSum;
    Util::String name;
    Util::String header;
    Util::RingBuffer<float> buffer;
    bool scroll;
};


//------------------------------------------------------------------------------
/**
*/
inline void
Graph::AddValue(float val)
{
    if(val < FLT_MAX)
        this->buffer.Add(val);
}

//------------------------------------------------------------------------------
/**
*/
inline float
Graph::ValueGetter(void* object, int idx)
{
    Graph * graph = (Graph*)object;
    float val;
    if (graph->scroll)
    {
        val = graph->buffer[idx];
    }
    else
    {
        val = graph->buffer.GetBuffer()[idx];
    }    
    graph->frame_max = graph->frame_max < val ? val : graph->frame_max;
    graph->frame_min = graph->frame_min > val ? val : graph->frame_min;
    graph->averageSum += val;    
    return val;
}

}// namespace Dynui


