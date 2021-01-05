#pragma once
//------------------------------------------------------------------------------
/**
    Test resource loader
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcestreampool.h"
#include "testresource.h"
namespace Test
{
class TestStreamPool : public Resources::ResourceStreamPool
{
    __DeclareClass(TestStreamPool);
public:
    /// constructor
    TestStreamPool();
    /// destructor
    virtual ~TestStreamPool();

    /// setup
    void Setup();
    /// discard
    void Discard();

    /// get resource
    const TestResourceData& GetResource(const Resources::ResourceId id);
private:

    /// load resource
    LoadStatus LoadFromStream(const Ids::Id24 id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream) override;
    /// discard resource
    void Unload(const Ids::Id24 id) override;

    Ids::IdAllocator<TestResourceData> alloc;
    __ImplementResourceAllocatorTyped(alloc, TestResourceIdType)
};
} // namespace Test