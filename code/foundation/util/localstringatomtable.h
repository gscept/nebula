#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::LocalStringAtomTable
  
    Implements a thread-local string atom table which is used as a cache
    to prevent excessive locking when creating string atoms.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/stringatomtablebase.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace Util
{
class LocalStringAtomTable : public StringAtomTableBase
{
    __DeclareSingleton(LocalStringAtomTable);
public:
    /// constructor
    LocalStringAtomTable();
    /// destructor
    ~LocalStringAtomTable();

private:
    friend class StringAtom;

    /// add a string pointer to the atom table
    void Add(const char* str);
};        

} // namespace Util
//------------------------------------------------------------------------------
