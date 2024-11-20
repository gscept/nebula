//------------------------------------------------------------------------------
//  memory.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

thread_local char ThreadLocalMiniHeap[10_MB];
thread_local size_t ThreadLocalMiniHeapIterator = 0;
