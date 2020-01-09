#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::AssignRegistry
    
    Central registry for path assigns. This is a true singleton, visible
    from all threads.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/assign.h"
#include "io/uri.h"
#include "util/hashtable.h"
#include "threading/criticalsection.h"
#include "core/singleton.h"

//------------------------------------------------------------------------------
namespace IO
{
class AssignRegistry : public Core::RefCounted
{
    __DeclareClass(AssignRegistry);
    __DeclareInterfaceSingleton(AssignRegistry);
public:
    /// constructor
    AssignRegistry();
    /// destructor
    virtual ~AssignRegistry();
    
    /// setup the assign registry (may only be called once from the main thread)
    void Setup();
    /// discard the assign registry
    void Discard();
    /// return true if the object has been setup
    bool IsValid() const;

    /// set a new assign
    void SetAssign(const Assign& assign);
    /// return true if an assign exists
    bool HasAssign(const Util::String& assignName) const;
    /// get an assign
    Util::String GetAssign(const Util::String& assignName) const;
    /// clear an assign
    void ClearAssign(const Util::String& assignName);
    /// return an array of all currently defined assigns
    Util::Array<Assign> GetAllAssigns() const;
    /// resolve any assigns in an URI
    URI ResolveAssigns(const URI& uri) const;
    /// resolve any assigns in a string (must have URI form)
    Util::String ResolveAssignsInString(const Util::String& uriString) const;

private:
    /// setup standard system assigns (e.g. home:, etc...)
    void SetupSystemAssigns();
    /// setup standard project assigns (e.g. msh:, model:, etc...)
    void SetupProjectAssigns();

    bool isValid;
    Threading::CriticalSection critSect;
    Util::HashTable<Util::String, Util::String> assignTable;
};

} // namespace IO
//------------------------------------------------------------------------------
