#pragma once
//------------------------------------------------------------------------------
/**
    A mutex is a MUTual EXecution primitive, used to synchronize memory access across threads

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Threading
{

class Mutex
{
public:
    /// constructor 
    Mutex();
    /// destructor
    ~Mutex();

    /// acquire mutex
    void Acquire();
    /// release mutex
    void Release();
private:
    void* handle;
};

} // namespace Threading
