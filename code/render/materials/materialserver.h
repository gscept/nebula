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
#include "materialtype.h"
#include "surfacepool.h"
#include "ids/id.h"
#include "io/uri.h"
namespace Materials
{

class MaterialServer : public Core::RefCounted
{
    __DeclareClass(MaterialServer);
    __DeclareInterfaceSingleton(MaterialServer);
public:

    /// constructor
    MaterialServer();
    /// destructor
    ~MaterialServer();

    /// open server
    void Open();
    /// close server (frees all materials and types)
    void Close();

    /// load material types from file
    bool LoadMaterialTypes(const IO::URI& file);

    /// get material
    MaterialType* GetMaterialType(const Resources::ResourceName& type);
    /// get material types by batch code
    const Util::Array<MaterialType*>* GetMaterialTypesByBatch(CoreGraphics::BatchGroup::Code code);

private:
    friend class SurfacePool;

    Memory::ArenaAllocator<0x400> surfaceAllocator;
    Util::Dictionary<Resources::ResourceName, MaterialType*> materialTypesByName;
    Util::HashTable<CoreGraphics::BatchGroup::Code, Util::Array<MaterialType*>> materialTypesByBatch;
    Util::Array<MaterialType*> materialTypes;
    Ptr<SurfacePool> surfacePool;
    MaterialType* currentType;
    bool isOpen;
};

} // namespace Materials
