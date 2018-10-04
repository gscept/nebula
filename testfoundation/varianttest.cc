//------------------------------------------------------------------------------
//  varianttest.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "varianttest.h"
#include "util/variant.h"
#include "util/guid.h"

namespace Test
{
__ImplementClass(Test::VariantTest, 'VART', Test::TestCase);

using namespace Util;
using namespace Math;

struct BlobData
{
    int intVal;
    bool boolVal;
    float floatVal;
};

//------------------------------------------------------------------------------
/**
*/
void
VariantTest::Run()
{
    String bla("Bla");

    BlobData blobData;
    blobData.intVal = 123;
    blobData.boolVal = true;
    blobData.floatVal = 10.2f;
    Blob blob(&blobData, sizeof(blobData));

    Variant intVar(123);
    Variant floatVar(123.4f);
    Variant boolVar(true);
    Variant float4Var(float4(1.0f, 2.0f, 3.0f, 4.0f));
    matrix44 m;
    float4 value(1.0f, 2.0f, 3.0f, 1.0f);
    m.setrow3(value);
    Variant matrixVar(m);
    Variant charPtrVar("Bla Bla");
    Variant strVar(bla);
    Variant blobVar(blob);

    // test variant types
    VERIFY(intVar.GetType() == Variant::Int);
    VERIFY(floatVar.GetType() == Variant::Float);
    VERIFY(boolVar.GetType() == Variant::Bool);
    VERIFY(float4Var.GetType() == Variant::Float4);
    VERIFY(matrixVar.GetType() == Variant::Matrix44);
    VERIFY(charPtrVar.GetType() == Variant::String);
    VERIFY(strVar.GetType() == Variant::String);
    VERIFY(blobVar.GetType() == Variant::Blob);


    // test equality operator
    VERIFY(intVar == 123);
    VERIFY(floatVar == 123.4f);
    VERIFY(boolVar == true);
    VERIFY(float4Var == float4(1.0f, 2.0f, 3.0f, 4.0f));
    VERIFY(charPtrVar == "Bla Bla");
    VERIFY(strVar == bla);

    // test inequality operator
    VERIFY(intVar != 124);
    VERIFY(floatVar != 122.3f);
    VERIFY(boolVar != false);
    VERIFY(float4Var != float4(1.0f, 1.0f, 2.0f, 3.0f));
    VERIFY(charPtrVar != "Blob Blob");

    #if __WIN32__
    Guid guid;
    guid.Generate();
    Guid otherGuid;
    otherGuid.Generate();
    Variant guidVar(guid);
    VERIFY(guidVar.GetType() == Variant::Guid);
    VERIFY(guidVar == guid);
    VERIFY(guidVar != otherGuid);
    #endif
}

} // namespace Test
