#pragma once
//------------------------------------------------------------------------------
/**
    Test resource loader
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourcememorypool.h"
#include "testresource.h"
namespace Test
{
class TestMemoryPool : public Resources::ResourceMemoryPool
{
    __DeclareClass(TestMemoryPool);
public:
    /// constructor
    TestMemoryPool();
    /// destructor
    virtual ~TestMemoryPool();

    /// setup
    void Setup();
    /// discard
    void Discard();

    struct UpdateInfo
    {
        const char* buf;
        size_t len;
    };

    /// get resource
    const TestResourceData& GetResource(const Resources::ResourceId id);
private:

    /// update resource
    LoadStatus LoadFromMemory(const Ids::Id24 id, const void* info) override;
    /// unload resource
    void Unload(const Ids::Id24 id);

    /// implement simple allocator
    //__ResourceAllocator(TestResource);
    Ids::IdAllocator<TestResourceData> alloc;
    __ImplementResourceAllocatorTyped(alloc, TestResourceIdType)
};
} // namespace Test