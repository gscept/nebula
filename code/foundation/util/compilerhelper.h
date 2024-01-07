#pragma once
//------------------------------------------------------------------------------
/**
    Helper macros for compiler related stuff

    (C) 2024 Individual contributors, see AUTHORS file
*/

#ifdef __clang__
#define UNUSED(x) __attribute__((unused)) x
#else
#define UNUSED(x) x
#endif