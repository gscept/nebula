#pragma once

#ifdef _WIN32
#define NEBULA_EXPORT extern "C" __declspec(dllexport)
#else
#define NEBULA_EXPORT extern "C" __attribute__((visibility("default")))
#endif
