#pragma once
//------------------------------------------------------------------------------
/**
    @class Util::StringAtomTableBase
  
    This implements the base class for thread-local and global string atom
    table classes.

    In order to reduce thread-synchronization, there are 2 levels of string
    atom tables in N3. One global string atom table for all threads, and
    one thread-local string atom table in each thread which acts as a
    cache for the global table. If a new string atom is created from a string, the 
    thread-local string atom table will be searched first. If the string has 
    already been registered in the thread-local table, the string atom will
    be setup and no locking at all is necessary. Only if the string is
    not in the thread local table, the global string atom table will
    be consulted (which requires locking). If the string is in the
    global table, the pointer to the string will be sorted into the
    thread-local atom table and the string will be setup. If the 
    string is completely new (not even in the global atom table),
    then the string needs to be sorted both into the global, and
    the thread-local atom table.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "util/array.h"

//------------------------------------------------------------------------------
namespace Util
{
class StringAtomTableBase
{
public:
    /// constructor
    StringAtomTableBase();
    /// destructor
    ~StringAtomTableBase();

protected:
    friend class StringAtom;

    /// find a string pointer in the atom table
    const char* Find(const char* str) const;

    /// a static string class for sorting the array
    struct StaticString
    {
        /// equality operator
        bool operator==(const StaticString& rhs) const;
        /// inequality operator
        bool operator!=(const StaticString& rhs) const;
        /// less-then operator
        bool operator<(const StaticString& rhs) const;
        /// greater-then operator
        bool operator>(const StaticString& rhs) const;

        char* ptr;
    };

    Util::Array<StaticString> table;
};

} // namespace Util
//------------------------------------------------------------------------------

