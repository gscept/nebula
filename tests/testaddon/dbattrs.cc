//------------------------------------------------------------------------------
//  dbattrs.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "testaddon/dbattrs.h"

namespace Attr
{
    DefineGuid(GuidValue, 'guid', ReadWrite);
    DefineBool(BoolValue, 'bval', ReadWrite);
    DefineFloat(FloatValue, 'fval', ReadWrite);
    DefineInt(IntValue, 'ival',  ReadWrite);
    DefineFloat4(Float4Value, 'v4vl', ReadWrite);
    DefineMatrix44(Matrix44Value, 'mxvl', ReadWrite);
    DefineString(StringValue, 'sval', ReadWrite);
    DefineBlob(BlobValue, 'blob', ReadWrite);

    DefineString(Name, 'NAME', ReadWrite);
    DefineBool(Male, 'MALE', ReadWrite);
    DefineFloat(IQ, 'IQ__', ReadWrite);
    DefineInt(Age, 'AGE_', ReadWrite);
    DefineFloat4(Velocity, 'VELO', ReadWrite);
    DefineFloat4(Color, 'COLR', ReadWrite);
    DefineMatrix44(Transform, 'TFRM', ReadWrite);
    DefineString(CarModel, 'CARM', ReadWrite);
    DefineString(Street, 'STRT', ReadWrite);
    DefineString(Product, 'PROD', ReadWrite);
    DefineString(City, 'CITY', ReadWrite);
    DefineString(Phone, 'PHON', ReadWrite);
    DefineGuid(GUID, 'GUID', ReadWrite);
    DefineInt(Nr, 'NR__', ReadWrite);
    DefineInt(Price, 'PRCE', ReadWrite);
    DefineInt(Stock, 'STCK', ReadWrite);
    DefineString(Country, 'CTRY', ReadWrite);
};

