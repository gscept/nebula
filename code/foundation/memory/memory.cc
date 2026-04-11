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
    iterator(0),
    capacity(0)
{
    this->Realloc(16_KB);
}

//------------------------------------------------------------------------------
/**
*/
ThreadLocalMiniHeap::~ThreadLocalMiniHeap()
{
    if (this->heap != nullptr)
    {
        free(this->heap);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ThreadLocalMiniHeap::Realloc(size_t numBytes)
{
    if (this->heap != nullptr)
    {
        free(this->heap);
        this->heap = nullptr;
    }

    this->capacity = numBytes;
    this->heap = (char*)malloc(this->capacity);
    this->iterator = 0;
}
