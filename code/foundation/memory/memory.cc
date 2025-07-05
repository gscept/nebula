//------------------------------------------------------------------------------
//  memory.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

thread_local ThreadLocalMiniHeap N_ThreadLocalMiniHeap;

//------------------------------------------------------------------------------
/**
*/
ThreadLocalMiniHeap::ThreadLocalMiniHeap() :
    heap(nullptr),
    capacity(0),
    iterator(0)
{
    this->Realloc(16_KB);
}

//------------------------------------------------------------------------------
/**
*/
ThreadLocalMiniHeap::~ThreadLocalMiniHeap()
{
    Memory::Free(Memory::ScratchHeap, this->heap);   
}

//------------------------------------------------------------------------------
/**
*/
void
ThreadLocalMiniHeap::Realloc(size_t numBytes)
{
    if (this->heap != nullptr)
    {
        Memory::Free(Memory::ScratchHeap, this->heap);
        this->heap = nullptr;
    }

    this->capacity = numBytes;
    this->heap = (char*)Memory::Alloc(Memory::ScratchHeap, this->capacity);
    this->iterator = 0;
}
