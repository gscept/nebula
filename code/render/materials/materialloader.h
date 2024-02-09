#pragma once
//------------------------------------------------------------------------------
/**
    The material pool provides a chunk allocation source for material types and instances

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "materials/shaderconfig.h"
#include "coregraphics/config.h"

namespace IO
{
class BXmlReader;
}

namespace Materials
{

class MaterialLoader : public Resources::ResourceLoader
{
    __DeclareClass(MaterialLoader);
public:

    /// setup resource loader, initiates the placeholder and error resources if valid, so don't forget to run!
    virtual void Setup() override;

    /// update reserved resource, the info struct is loader dependent (overload to implement resource deallocation, remember to set resource state!)
    Resources::ResourceUnknownId InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;

    /// Allocate constant memory explicitly
    static void* AllocateConstantMemory(SizeT size);
private:

    /// unload resource (overload to implement resource deallocation)
    void Unload(const Resources::ResourceId id) override;
    Util::Dictionary<MaterialProperties, void(*)(Ptr<IO::BXmlReader>, Materials::MaterialId, Util::StringAtom)> loaderMap;
};

} // namespace Materials
