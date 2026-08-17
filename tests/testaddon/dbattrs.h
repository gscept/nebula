#ifndef TEST_DBATTRS_H
#define TEST_DBATTRS_H
//------------------------------------------------------------------------------
/**
    @file test/dbattrs.h
    
    Declare common attributes for the database tests.
    
    (C) 2006 Radon Labs GmbH
*/
#include "attr/attribute.h"

//------------------------------------------------------------------------------
namespace Attr
{
    DeclareAttrGuid(GuidValue);
    DeclareAttrBool(BoolValue);
    DeclareAttrFloat(FloatValue);
    DeclareAttrInt(IntValue);
    DeclareAttrFloat4(Float4Value);
    DeclareAttrMatrix44(Matrix44Value);
    DeclareAttrString(StringValue);
    DeclareAttrBlob(BlobValue);

    DeclareAttrString(Name);
    DeclareAttrBool(Male);
    DeclareAttrFloat(IQ);
    DeclareAttrInt(Age);
    DeclareAttrFloat4(Velocity);
    DeclareAttrFloat4(Color);
    DeclareAttrMatrix44(Transform);
    DeclareAttrString(CarModel);
    DeclareAttrString(Street);
    DeclareAttrString(Product);
    DeclareAttrString(City);
    DeclareAttrString(Phone);
    DeclareAttrGuid(GUID);
    DeclareAttrInt(Nr);
    DeclareAttrInt(Price);
    DeclareAttrInt(Stock);
    DeclareAttrString(Country);
};
//------------------------------------------------------------------------------
#endif
    