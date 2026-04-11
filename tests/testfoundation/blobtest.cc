//------------------------------------------------------------------------------
//  blobtest.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "blobtest.h"
#include "util/blob.h"
#include <string.h>

namespace Test
{
__ImplementClass(Test::BlobTest, 'BLBT' , Test::TestCase);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
void
BlobTest::Run()
{
    // Default constructor: not valid, size 0, null pointer
    {
        Util::Blob empty;
        VERIFY(!empty.IsValid());
        VERIFY(empty.Size() == 0);
        VERIFY(empty.GetPtr() == nullptr);
    }

    // nullptr_t constructor: same as default
    {
        Util::Blob nullBlob(nullptr);
        VERIFY(!nullBlob.IsValid());
        VERIFY(nullBlob.Size() == 0);
        VERIFY(nullBlob.GetPtr() == nullptr);
    }

    // Blob(const void*, size_t) constructor: copies content
    {
        const char src[] = "hello";
        Util::Blob fromPtr(src, sizeof(src));
        VERIFY(fromPtr.IsValid());
        VERIFY(fromPtr.Size() == sizeof(src));
        VERIFY(memcmp(fromPtr.GetPtr(), src, sizeof(src)) == 0);
    }

    // Blob(size_t) constructor: allocates and exposes writable memory
    {
        Util::Blob sizeBlob(32);
        VERIFY(sizeBlob.IsValid());
        VERIFY(sizeBlob.Size() == 32);
    }

    // SetSize: basic allocation + write + read
    Util::Blob blob;
    blob.SetSize(16 * sizeof(int));
    VERIFY(blob.IsValid());
    VERIFY(blob.Size() == 16 * sizeof(int));
    int* data = (int*)blob.GetPtr();
    for (int i = 0; i < 16; i++)
        data[i] = i;

    // SetSize: grow preserves existing data
    {
        Util::Blob b;
        b.SetSize(4);
        uint8_t* p = (uint8_t*)b.GetPtr();
        p[0] = 0xDE; p[1] = 0xAD; p[2] = 0xBE; p[3] = 0xEF;
        b.SetSize(8);
        VERIFY(b.Size() == 8);
        uint8_t* p2 = (uint8_t*)b.GetPtr();
        VERIFY(p2[0] == 0xDE && p2[1] == 0xAD && p2[2] == 0xBE && p2[3] == 0xEF);
    }

    // SetSize: shrink updates logical size, no reallocation
    {
        Util::Blob b;
        b.SetSize(16);
        void* origPtr = b.GetPtr();
        b.SetSize(8);
        VERIFY(b.Size() == 8);
        VERIFY(b.GetPtr() == origPtr);
    }

    // SetSize: same size is idempotent (no reallocation)
    {
        Util::Blob b;
        b.SetSize(16);
        void* origPtr = b.GetPtr();
        b.SetSize(16);
        VERIFY(b.Size() == 16);
        VERIFY(b.GetPtr() == origPtr);
    }

    // Trim: reduces logical size without reallocation
    {
        Util::Blob b;
        b.SetSize(16);
        void* origPtr = b.GetPtr();
        b.Trim(8);
        VERIFY(b.Size() == 8);
        VERIFY(b.GetPtr() == origPtr);
    }

    // Set: replaces content
    {
        const char src[] = "testdata";
        Util::Blob b;
        b.Set(src, sizeof(src));
        VERIFY(b.IsValid());
        VERIFY(b.Size() == sizeof(src));
        VERIFY(memcmp(b.GetPtr(), src, sizeof(src)) == 0);
    }

    // Copy constructor: deep copy with distinct pointer
    Util::Blob blob2 = blob;
    VERIFY(blob2 == blob);
    VERIFY(blob2.GetPtr() != blob.GetPtr());
    VERIFY(blob2.Size() == blob.Size());
    VERIFY(blob2.HashCode() == blob.HashCode());

    // Copy constructor from empty: destination is also empty
    {
        Util::Blob emptySource;
        Util::Blob emptyDest = emptySource;
        VERIFY(!emptyDest.IsValid());
        VERIFY(emptyDest.Size() == 0);
        VERIFY(emptyDest.GetPtr() == nullptr);
    }

    // Copy assignment from valid: deep copy
    {
        Util::Blob dest;
        dest = blob;
        VERIFY(dest == blob);
        VERIFY(dest.GetPtr() != blob.GetPtr());
        VERIFY(dest.Size() == blob.Size());
    }

    // Copy assignment from invalid: lhs becomes invalid
    {
        Util::Blob dest = blob;
        VERIFY(dest.IsValid());
        Util::Blob emptySource;
        dest = emptySource;
        VERIFY(!dest.IsValid());
        VERIFY(dest.Size() == 0);
        VERIFY(dest.GetPtr() == nullptr);
    }

    // Move constructor via NRVO lambda
    void* internalBlob;
    auto blobfill = [&internalBlob]() -> Util::Blob
    {
        Util::Blob b;
        b.SetSize(16 * sizeof(int));
        internalBlob = b.GetPtr();
        int* data = (int*)b.GetPtr();
        for (int i = 0; i < 16; i++)
            data[i] = i;
        return b;
    };
    Util::Blob blob3 = blobfill();
    VERIFY(internalBlob == blob3.GetPtr());
    VERIFY(blob3 == blob);

    // Move constructor: source becomes invalid
    {
        Util::Blob src;
        src.SetSize(8);
        void* srcPtr = src.GetPtr();
        Util::Blob moved(std::move(src));
        VERIFY(moved.IsValid());
        VERIFY(moved.GetPtr() == srcPtr);
        VERIFY(!src.IsValid());
        VERIFY(src.Size() == 0);
        VERIFY(src.GetPtr() == nullptr);
    }

    // Move assignment: pointer is transferred
    void* oldPtr = blob.GetPtr();
    blob = blobfill();
    VERIFY(oldPtr != blob.GetPtr());
    VERIFY(internalBlob == blob.GetPtr());

    // Move assignment: source becomes invalid
    {
        Util::Blob src;
        src.SetSize(8);
        void* srcPtr = src.GetPtr();
        Util::Blob dest;
        dest = std::move(src);
        VERIFY(dest.IsValid());
        VERIFY(dest.GetPtr() == srcPtr);
        VERIFY(!src.IsValid());
        VERIFY(src.Size() == 0);
        VERIFY(src.GetPtr() == nullptr);
    }

    // SetChunk: fresh blob allocation
    {
        const uint8_t src[] = {1, 2, 3, 4};
        Util::Blob b;
        b.SetChunk(src, 4, 0);
        VERIFY(b.IsValid());
        VERIFY(b.Size() == 4);
        VERIFY(memcmp(b.GetPtr(), src, 4) == 0);
    }

    // SetChunk: write at non-zero offset
    {
        Util::Blob b;
        b.SetSize(8);
        memset(b.GetPtr(), 0, 8);
        const uint8_t chunk[] = {0xAA, 0xBB};
        b.SetChunk(chunk, 2, 4);
        VERIFY(b.Size() == 8);
        const uint8_t* p = (const uint8_t*)b.GetPtr();
        VERIFY(p[4] == 0xAA && p[5] == 0xBB);
    }

    // SetChunk: size updated to max(existing, offset+chunk)
    {
        Util::Blob b;
        b.SetSize(8);
        memset(b.GetPtr(), 0, 8);
        const uint8_t chunk[] = {0xFF};
        b.SetChunk(chunk, 1, 2); // offset 2, chunk 1 → required 3; max(8,3)=8
        VERIFY(b.Size() == 8);
    }

    // SetChunk: growth preserves previously written data
    {
        const uint8_t initial[] = {1, 2, 3, 4};
        Util::Blob b(initial, 4);
        const uint8_t chunk[] = {5, 6, 7, 8};
        b.SetChunk(chunk, 4, 4); // forces reallocation
        VERIFY(b.Size() == 8);
        const uint8_t* p = (const uint8_t*)b.GetPtr();
        VERIFY(p[0] == 1 && p[1] == 2 && p[2] == 3 && p[3] == 4);
        VERIFY(p[4] == 5 && p[5] == 6 && p[6] == 7 && p[7] == 8);
    }

    // Comparisons: equal blobs
    {
        const char src[] = "compare";
        Util::Blob a(src, sizeof(src));
        Util::Blob b(src, sizeof(src));
        VERIFY(a == b);
        VERIFY(!(a != b));
        VERIFY(a >= b);
        VERIFY(a <= b);
        VERIFY(!(a > b));
        VERIFY(!(a < b));
    }

    // Comparisons: size ordering
    {
        const char bigSrc[] = "longerdata";
        const char smallSrc[] = "short";
        Util::Blob big(bigSrc, sizeof(bigSrc));
        Util::Blob small(smallSrc, sizeof(smallSrc));
        VERIFY(big != small);
        VERIFY(big > small);
        VERIFY(big >= small);
        VERIFY(small < big);
        VERIFY(small <= big);
    }

    // Comparisons: same size, different content
    {
        uint8_t a_data[] = {1, 2, 3};
        uint8_t b_data[] = {1, 2, 4};
        Util::Blob a(a_data, 3);
        Util::Blob b(b_data, 3);
        VERIFY(a != b);
        VERIFY(a < b);
        VERIFY(b > a);
    }

    // Comparisons: both empty
    {
        Util::Blob a, b;
        VERIFY(a == b);
        VERIFY(!(a != b));
    }

    // Comparisons: one empty, one valid
    {
        Util::Blob empty;
        Util::Blob valid;
        valid.SetSize(4);
        VERIFY(empty != valid);
        VERIFY(empty < valid);
        VERIFY(valid > empty);
    }

    // HashCode: same data yields same hash
    {
        const char src[] = "hashme";
        Util::Blob a(src, sizeof(src));
        Util::Blob b(src, sizeof(src));
        VERIFY(a.HashCode() == b.HashCode());
    }

    // HashCode: empty blob does not crash (loop skipped for size==0)
    {
        Util::Blob empty;
        (void)empty.HashCode();
    }

    // GetBase64 / SetFromBase64 round-trip
    Util::Blob b64 = blob.GetBase64();
    blob3.SetFromBase64(b64.GetPtr(), b64.Size());
    VERIFY(blob3 == blob);
}

}; 