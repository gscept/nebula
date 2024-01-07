#pragma once
//------------------------------------------------------------------------------
/**
    Stream loader for skeletons

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceloader.h"
#include "util/fixedarray.h"
#include "util/hashtable.h"
#include "skeleton.h"
namespace Characters
{

class SkeletonLoader : public Resources::ResourceLoader
{
    __DeclareClass(SkeletonLoader);
private:

    /// load character definition from stream
    Resources::ResourceUnknownId InitializeResource(const Ids::Id32 entry, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate = false) override;
    /// unload resource
    void Unload(const Resources::ResourceId id) override;

};

} // namespace Characters
