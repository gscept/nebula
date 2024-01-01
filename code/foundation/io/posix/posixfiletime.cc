//------------------------------------------------------------------------------
//  posixfiletime.cc
//  (C 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/posix/posixfiletime.h"

namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
PosixFileTime::PosixFileTime(const Util::String& str)
{
    Util::Array<Util::String> tokens = str.Tokenize("#");
    n_assert(tokens.Size() == 2);
    this->SetBits(tokens[1].AsInt(), tokens[0].AsInt());
}

//------------------------------------------------------------------------------
/**
*/
Util::String
PosixFileTime::AsString() const
{
    Util::String str;
    str.Format("%d#%d", this->GetHighBits(), this->GetLowBits());
    return str;
}

}