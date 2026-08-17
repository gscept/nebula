//------------------------------------------------------------------------------
//  dbattrs.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testaddon/dbattrs.h"

namespace Attr
{
    DefineAttrGuid(GuidValue, 'guid', ReadWrite);
    DefineAttrBool(BoolValue, 'bval', ReadWrite);
    DefineAttrFloat(FloatValue, 'fval', ReadWrite);
    DefineAttrInt(IntValue, 'ival',  ReadWrite);
    DefineAttrFloat4(Float4Value, 'v4vl', ReadWrite);
    DefineAttrMatrix44(Matrix44Value, 'mxvl', ReadWrite);
    DefineAttrString(StringValue, 'sval', ReadWrite);
    DefineAttrBlob(BlobValue, 'blob', ReadWrite);

    DefineAttrString(Name, 'NAME', ReadWrite);
    DefineAttrBool(Male, 'MALE', ReadWrite);
    DefineAttrFloat(IQ, 'IQ__', ReadWrite);
    DefineAttrInt(Age, 'AGE_', ReadWrite);
    DefineAttrFloat4(Velocity, 'VELO', ReadWrite);
    DefineAttrFloat4(Color, 'COLR', ReadWrite);
    DefineAttrMatrix44(Transform, 'TFRM', ReadWrite);
    DefineAttrString(CarModel, 'CARM', ReadWrite);
    DefineAttrString(Street, 'STRT', ReadWrite);
    DefineAttrString(Product, 'PROD', ReadWrite);
    DefineAttrString(City, 'CITY', ReadWrite);
    DefineAttrString(Phone, 'PHON', ReadWrite);
    DefineAttrGuid(GUID, 'GUID', ReadWrite);
    DefineAttrInt(Nr, 'NR__', ReadWrite);
    DefineAttrInt(Price, 'PRCE', ReadWrite);
    DefineAttrInt(Stock, 'STCK', ReadWrite);
    DefineAttrString(Country, 'CTRY', ReadWrite);
};

