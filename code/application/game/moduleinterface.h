#pragma once
//------------------------------------------------------------------------------
/**
    Runtime shared module ABI definitions.

    These symbols are exported from shared game modules and consumed by the
    runtime module manager. Keep this interface C-compatible and minimal to
    avoid accidental ABI breakage.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include <stdint.h>

#define NEBULA_MODULE_ABI_VERSION 1u
#define NEBULA_MODULE_GET_DESCRIPTOR_EXPORT "NebulaModuleGetDescriptor"
#define NEBULA_MODULE_CREATE_FEATURE_EXPORT "NebulaModuleCreateFeature"
#define NEBULA_MODULE_DESTROY_FEATURE_EXPORT "NebulaModuleDestroyFeature"

#ifdef __cplusplus
extern "C"
{
#endif

struct NebulaModuleDescriptor
{
    uint32_t abiVersion;
    const char* name;
    const char* version;
    uint32_t flags;
};

typedef int (*NebulaModuleGetDescriptorFn)(NebulaModuleDescriptor* outDescriptor);
// Created feature must be a Game::FeatureUnit-compatible Core::RefCounted object
// owned by Nebula's refcounting lifecycle after attachment.
typedef void* (*NebulaModuleCreateFeatureFn)();
typedef void (*NebulaModuleDestroyFeatureFn)(void* feature);

#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------
