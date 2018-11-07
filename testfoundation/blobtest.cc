//------------------------------------------------------------------------------
//  blobtest.cc
// (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "blobtest.h"
#include "util/blob.h"

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
    Util::Blob blob;
    blob.Reserve(16 * sizeof(int));
    int * data = (int*)blob.GetPtr();
    for (int i = 0; i < 16; i++)
    {
        data[i] = i;
    }
    Util::Blob blob2 = blob;
    VERIFY(blob2 == blob);
    // check copy constructor
    VERIFY(blob2.GetPtr() != blob.GetPtr());
    VERIFY(blob2.HashCode() == blob2.HashCode());

    // check for move constructor
    void * internalBlob;
    auto blobfill = [&internalBlob]()->Util::Blob
    {
        Util::Blob b;
        b.Reserve(16 * sizeof(int));
        internalBlob = b.GetPtr();
        int * data = (int*)b.GetPtr();
        for (int i = 0; i < 16; i++)
        {
            data[i] = i;
        }
        return b;
    };
    Util::Blob blob3 = blobfill();
    VERIFY(internalBlob == blob3.GetPtr());    
    VERIFY(blob3 == blob);

    void* oldPTr = blob.GetPtr();
    // move assignment
    blob = blobfill();
    VERIFY(oldPTr != blob.GetPtr());
    VERIFY(internalBlob == blob.GetPtr());

    Util::Blob b64 = blob.GetBase64();

    blob3.SetFromBase64(b64.GetPtr(), b64.Size());
    VERIFY(blob3 == blob);
}

}; 