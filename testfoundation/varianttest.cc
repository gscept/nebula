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
    this->Verify(intVar.GetType() == Variant::Int);
    this->Verify(floatVar.GetType() == Variant::Float);
    this->Verify(boolVar.GetType() == Variant::Bool);
    this->Verify(float4Var.GetType() == Variant::Float4);
    this->Verify(matrixVar.GetType() == Variant::Matrix44);
    this->Verify(charPtrVar.GetType() == Variant::String);
    this->Verify(strVar.GetType() == Variant::String);
    this->Verify(blobVar.GetType() == Variant::Blob);


    // test equality operator
    this->Verify(intVar == 123);
    this->Verify(floatVar == 123.4f);
    this->Verify(boolVar == true);
    this->Verify(float4Var == float4(1.0f, 2.0f, 3.0f, 4.0f));
    this->Verify(charPtrVar == "Bla Bla");
    this->Verify(strVar == bla);

    // test inequality operator
    this->Verify(intVar != 124);
    this->Verify(floatVar != 122.3f);
    this->Verify(boolVar != false);
    this->Verify(float4Var != float4(1.0f, 1.0f, 2.0f, 3.0f));
    this->Verify(charPtrVar != "Blob Blob");

    #if __WIN32__
    Guid guid;
    guid.Generate();
    Guid otherGuid;
    otherGuid.Generate();
    Variant guidVar(guid);
    this->Verify(guidVar.GetType() == Variant::Guid);
    this->Verify(guidVar == guid);
    this->Verify(guidVar != otherGuid);
    #endif
}

} // namespace Test
