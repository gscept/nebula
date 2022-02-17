#pragma once
//------------------------------------------------------------------------------
/**
    The material server is the central hub for the material types

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "shaderconfig.h"
#include "materialcache.h"
#include "ids/id.h"
#include "io/uri.h"
namespace Materials
{

class ShaderConfigServer : public Core::RefCounted
{
    __DeclareClass(ShaderConfigServer);
    __DeclareInterfaceSingleton(ShaderConfigServer);
public:

    /// constructor
    ShaderConfigServer();
    /// destructor
    ~ShaderConfigServer();

    /// open server
    void Open();
    /// close server (frees all materials and types)
    void Close();

    /// load material types from file
    bool LoadShaderConfigs(const IO::URI& file);

    /// Allocate some variant memory
    ShaderConfigVariant AllocateVariantMemory(const ShaderConfigVariant::InternalType type);

    /// get material
    ShaderConfig* GetShaderConfig(const Resources::ResourceName& type);
    /// get material types by batch code
    const Util::Array<ShaderConfig*>& GetShaderConfigsByBatch(CoreGraphics::BatchGroup::Code code);

private:
    friend class MaterialCache;

    Threading::CriticalSection variantAllocatorLock;
    Memory::ArenaAllocator<0x10000> shaderConfigVariantAllocator;

    Memory::ArenaAllocator<0x400> surfaceAllocator;
    Util::Dictionary<Resources::ResourceName, ShaderConfig*> shaderConfigsByName;
    Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<ShaderConfig*>> shaderConfigsByBatch;
    Util::Array<ShaderConfig*> shaderConfigs;
    ShaderConfig* currentType;
    bool isOpen;
};

} // namespace Materials
